#include <vcl.h>
#pragma hdrstop

#include "TelegramAttachmentSelfTest.h"

#ifdef _DEBUG

#include "MultiServerManager.h"
#include "TelegramAttachmentClient.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
	using boost::asio::ip::tcp;
	namespace fs = std::filesystem;
	namespace nh = nlohmann;

	enum class PayloadMode
	{
		Whole,
		Chunked,
		Interrupt
	};

	struct AttachmentPayload
	{
		std::string fileName;
		std::string bytes;
		PayloadMode mode{PayloadMode::Whole};
		std::size_t interruptAfter{0};
	};

	class FakeAttachmentServer
	{
	public:
		FakeAttachmentServer(
			std::string listJson,
			std::vector<AttachmentPayload> payloads)
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, listJson_(std::move(listJson))
			, payloads_(std::move(payloads))
			, worker_([this]() { run(); })
		{
		}

		~FakeAttachmentServer()
		{
			boost::system::error_code ignored;
			acceptor_.close(ignored);
			if (socket_)
				socket_->close(ignored);
			if (worker_.joinable())
				worker_.join();
		}

		unsigned short port() const { return acceptor_.local_endpoint().port(); }
		bool failed() const { return failed_.load(std::memory_order_acquire); }
		bool sawGetBinFile() const { return sawGetBinFile_.load(std::memory_order_acquire); }

		int countCommand(COMMAND command) const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			int count = 0;
			for (const auto seen : commands_) {
				if (seen == command)
					++count;
			}
			return count;
		}

		std::string lastBodyFor(COMMAND command) const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			std::string body;
			for (std::size_t i = 0; i < commands_.size(); ++i) {
				if (commands_[i] == command)
					body = bodies_[i];
			}
			return body;
		}

	private:
		void record(COMMAND command, const std::string& body)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			commands_.push_back(command);
			bodies_.push_back(body);
		}

		void send(tcp::socket& socket, COMMAND command, const std::string& body)
		{
			header response{};
			response.command = command;
			response.size = static_cast<uint_fast64_t>(body.size());
			boost::asio::write(socket, boost::asio::buffer(&response, sizeof(response)));
			if (!body.empty())
				boost::asio::write(socket, boost::asio::buffer(body.data(), body.size()));
		}

		void sendPayload(
			tcp::socket& socket,
			COMMAND command,
			const std::string& body,
			PayloadMode mode,
			std::size_t interruptAfter)
		{
			header response{};
			response.command = command;
			response.size = static_cast<uint_fast64_t>(body.size());
			boost::asio::write(socket, boost::asio::buffer(&response, sizeof(response)));

			if (mode == PayloadMode::Interrupt) {
				const std::size_t n = (std::min)(interruptAfter, body.size());
				if (n > 0)
					boost::asio::write(socket, boost::asio::buffer(body.data(), n));
				boost::system::error_code ignored;
				socket.shutdown(tcp::socket::shutdown_both, ignored);
				socket.close(ignored);
				return;
			}

			if (mode == PayloadMode::Chunked) {
				constexpr std::size_t chunk = 3;
				for (std::size_t offset = 0; offset < body.size(); offset += chunk) {
					const std::size_t n = (std::min)(chunk, body.size() - offset);
					boost::asio::write(socket, boost::asio::buffer(body.data() + offset, n));
				}
				return;
			}

			if (!body.empty())
				boost::asio::write(socket, boost::asio::buffer(body.data(), body.size()));
		}

		void readRequest(tcp::socket& socket, header& request, std::string& body)
		{
			boost::asio::read(socket, boost::asio::buffer(&request, sizeof(request)));
			body.assign(static_cast<std::size_t>(request.size), '\0');
			if (!body.empty())
				boost::asio::read(socket, boost::asio::buffer(&body[0], body.size()));
		}

		static bool looksLikeServerPath(const std::string& body)
		{
			if (body.find("DirectTo") != std::string::npos)
				return true;
			if (body.find("remotePath") != std::string::npos)
				return true;
			if (body.find("D:") != std::string::npos || body.find("d:") != std::string::npos)
				return true;
			if (body.find("\\\\") != std::string::npos)
				return true;
			return false;
		}

		static bool isScopedListJson(const std::string& body)
		{
			try {
				const auto parsed = nh::json::parse(body);
				if (!parsed.is_object() || parsed.size() != 2)
					return false;
				if (!parsed.contains("id") || !parsed.contains("type"))
					return false;
				if (parsed.contains("file_name") || parsed.contains("DirectTo"))
					return false;
				return !looksLikeServerPath(body);
			}
			catch (...) {
				return false;
			}
		}

		static bool isScopedSingleJson(const std::string& body)
		{
			try {
				const auto parsed = nh::json::parse(body);
				if (!parsed.is_object() || parsed.size() != 3)
					return false;
				if (!parsed.contains("id") || !parsed.contains("type") ||
					!parsed.contains("file_name") || !parsed["file_name"].is_string())
					return false;
				const auto name = parsed["file_name"].get<std::string>();
				if (name.empty() ||
					name.find('\\') != std::string::npos ||
					name.find('/') != std::string::npos ||
					name.find(':') != std::string::npos)
					return false;
				return !looksLikeServerPath(body);
			}
			catch (...) {
				return false;
			}
		}

		void serveApplicationRequests(tcp::socket& socket)
		{
			for (;;) {
				header request{};
				std::string body;
				readRequest(socket, request, body);
				record(request.command, body);

				if (request.command == COMMAND::GETBINFILE) {
					sawGetBinFile_.store(true, std::memory_order_release);
					failed_.store(true, std::memory_order_release);
					send(socket, COMMAND::SOMEERROR, {});
					return;
				}

				if (request.command == COMMAND::USER_REGISTRY) {
					send(socket, request.command, "OK");
					continue;
				}

				if (request.command == COMMAND::GET_TELEGA_ATACHMENTS) {
					if (!isScopedListJson(body)) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}
					send(socket, request.command, listJson_);
					continue;
				}

				if (request.command == COMMAND::GET_TELEGA_SINGLE_ATACHMENT) {
					if (!isScopedSingleJson(body)) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					std::string fileName;
					try {
						fileName = nh::json::parse(body).at("file_name").get<std::string>();
					}
					catch (...) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					const AttachmentPayload* payload = nullptr;
					for (const auto& item : payloads_) {
						if (item.fileName == fileName) {
							payload = &item;
							break;
						}
					}
					if (!payload) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					sendPayload(
						socket,
						request.command,
						payload->bytes,
						payload->mode,
						payload->interruptAfter);
					if (payload->mode == PayloadMode::Interrupt)
						return;
					continue;
				}

				failed_.store(true, std::memory_order_release);
				send(socket, COMMAND::SOMEERROR, {});
				return;
			}
		}

		void run()
		{
			try {
				socket_ = std::make_unique<tcp::socket>(io_);
				acceptor_.accept(*socket_);

				header negotiationRequest{};
				std::string negotiationBody;
				readRequest(*socket_, negotiationRequest, negotiationBody);
				if (negotiationRequest.command != COMMAND::NEGOTIATE_PROTOCOL_V1 ||
					!negotiationBody.empty()) {
					failed_.store(true, std::memory_order_release);
					return;
				}

				search_protocol::ProtocolCapabilitiesV1 capabilities{};
				capabilities.capabilities =
					search_protocol::CAPABILITY_TYPED_ERRORS_V1;
				send(
					*socket_,
					COMMAND::NEGOTIATE_PROTOCOL_V1,
					std::string(
						reinterpret_cast<const char*>(&capabilities),
						sizeof(capabilities)));

				serveApplicationRequests(*socket_);
			}
			catch (...) {
				if (sawGetBinFile_.load(std::memory_order_acquire))
					failed_.store(true, std::memory_order_release);
			}
		}

		boost::asio::io_context io_;
		tcp::acceptor acceptor_;
		std::unique_ptr<tcp::socket> socket_;
		std::string listJson_;
		std::vector<AttachmentPayload> payloads_;
		std::thread worker_;
		mutable std::mutex mutex_;
		std::vector<COMMAND> commands_;
		std::vector<std::string> bodies_;
		std::atomic_bool failed_{false};
		std::atomic_bool sawGetBinFile_{false};
	};

	std::string readFileBytes(const fs::path& path)
	{
		std::ifstream in(path, std::ios::binary);
		if (!in)
			return {};
		return std::string(
			(std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());
	}

	bool writeFileBytes(const fs::path& path, const std::string& bytes)
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out)
			return false;
		out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		return static_cast<bool>(out);
	}

	bool jsonHasIdType(const std::string& body, int id, int type)
	{
		try {
			const auto parsed = nh::json::parse(body);
			return parsed.is_object() &&
				parsed.value("id", -1) == id &&
				parsed.value("type", -1) == type;
		}
		catch (...) {
			return false;
		}
	}

	bool jsonHasFileName(const std::string& body, const std::string& fileName)
	{
		try {
			const auto parsed = nh::json::parse(body);
			return parsed.is_object() &&
				parsed.value("file_name", std::string{}) == fileName;
		}
		catch (...) {
			return false;
		}
	}

	bool downloadIfAdvertised(
		MultiServerManager& manager,
		const std::string& serverId,
		int telegramId,
		int telegramType,
		const std::string& fileName,
		const std::string& localPath,
		std::string& error,
		bool replaceAfterSuccess = false)
	{
		std::vector<TelegramAttachmentInfo> items;
		if (!requestTelegramAttachmentList(
				manager, serverId, telegramId, telegramType, items, error))
			return false;
		const auto* advertised = findAdvertisedTelegramAttachment(items, fileName);
		if (!advertised || advertised->name.empty()) {
			error = "unknown";
			return false;
		}
		if (!advertised->exists) {
			error = "missing";
			return false;
		}
		return downloadTelegramAttachment(
			manager,
			serverId,
			telegramId,
			telegramType,
			advertised->name,
			localPath,
			nullptr,
			&error,
			false,
			replaceAfterSuccess);
	}
}

bool IsTelegramAttachmentSelfTestInvocation()
{
	return ParamCount() >= 1 && SameText(ParamStr(1), L"--telegram-attachment-self-test");
}

int RunTelegramAttachmentSelfTestFromCommandLine()
{
	const fs::path root = fs::temp_directory_path() / "sc_tlg_att_self_test";
	std::error_code ec;
	fs::remove_all(root, ec);
	fs::create_directories(root, ec);
	if (ec)
		return 1;

	auto cleanup = [&]() { fs::remove_all(root, ec); };

	{
		FakeAttachmentServer server(
			R"([{"name":"a.pdf","exists":true,"size":7},{"name":"b.txt","exists":true,"size":3}])",
			{{"a.pdf", "PDFDATA", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "incoming" / "a.pdf";
		std::string error;
		if (!downloadIfAdvertised(manager, "first", 123, 0, "a.pdf", out.string(), error)) {
			cleanup();
			return 10;
		}
		if (readFileBytes(out) != "PDFDATA") {
			cleanup();
			return 11;
		}
		if (server.sawGetBinFile() || server.failed()) {
			cleanup();
			return 12;
		}
		if (server.countCommand(COMMAND::GETBINFILE) != 0 ||
			server.countCommand(COMMAND::GET_TELEGA_ATACHMENTS) != 1 ||
			server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 1) {
			cleanup();
			return 13;
		}
		if (!jsonHasIdType(server.lastBodyFor(COMMAND::GET_TELEGA_ATACHMENTS), 123, 0) ||
			!jsonHasIdType(server.lastBodyFor(COMMAND::GET_TELEGA_SINGLE_ATACHMENT), 123, 0) ||
			!jsonHasFileName(server.lastBodyFor(COMMAND::GET_TELEGA_SINGLE_ATACHMENT), "a.pdf")) {
			cleanup();
			return 14;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer server(
			R"([{"name":"456.zip","exists":true,"size":8}])",
			{{"456.zip", "ZIPBYTES", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "outgoing" / "456.zip";
		std::string error;
		if (!downloadIfAdvertised(manager, "first", 456, 1, "456.zip", out.string(), error)) {
			cleanup();
			return 20;
		}
		if (readFileBytes(out) != "ZIPBYTES") {
			cleanup();
			return 21;
		}
		if (server.sawGetBinFile() || server.failed() ||
			server.countCommand(COMMAND::GETBINFILE) != 0 ||
			!jsonHasIdType(server.lastBodyFor(COMMAND::GET_TELEGA_ATACHMENTS), 456, 1) ||
			!jsonHasFileName(server.lastBodyFor(COMMAND::GET_TELEGA_SINGLE_ATACHMENT), "456.zip")) {
			cleanup();
			return 22;
		}
		manager.disconnectAll();
	}

	{
		const std::string payload = "0123456789ABCDEFGHIJ";
		FakeAttachmentServer server(
			R"([{"name":"chunk.bin","exists":true,"size":20}])",
			{{"chunk.bin", payload, PayloadMode::Chunked, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "chunked" / "chunk.bin";
		std::string error;
		if (!downloadIfAdvertised(manager, "first", 1, 0, "chunk.bin", out.string(), error) ||
			readFileBytes(out) != payload) {
			cleanup();
			return 30;
		}
		if (server.sawGetBinFile() || server.failed()) {
			cleanup();
			return 31;
		}
		manager.disconnectAll();
	}

	{
		const std::string original = "OLD-CONTENT";
		const fs::path existing = root / "interrupt" / "keep.bin";
		fs::create_directories(existing.parent_path(), ec);
		if (!writeFileBytes(existing, original)) {
			cleanup();
			return 40;
		}

		FakeAttachmentServer server(
			R"([{"name":"keep.bin","exists":true,"size":16}])",
			{{"keep.bin", "NEW-CONTENT-XXXX", PayloadMode::Interrupt, 4}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		std::string error;
		if (downloadIfAdvertised(
				manager, "first", 7, 0, "keep.bin", existing.string(), error, true)) {
			cleanup();
			return 41;
		}
		if (readFileBytes(existing) != original) {
			cleanup();
			return 42;
		}

		const fs::path missing = root / "interrupt" / "new.bin";
		FakeAttachmentServer server2(
			R"([{"name":"new.bin","exists":true,"size":16}])",
			{{"new.bin", "NEW-CONTENT-XXXX", PayloadMode::Interrupt, 4}});
		MultiServerManager manager2;
		manager2.configure({
			{"first", "First", "127.0.0.1", std::to_string(server2.port()), true}
		}, "first");
		manager2.setUser("self-test-user");
		if (downloadIfAdvertised(
				manager2, "first", 8, 0, "new.bin", missing.string(), error, false)) {
			cleanup();
			return 43;
		}
		if (fs::exists(missing)) {
			cleanup();
			return 44;
		}
		if (server.sawGetBinFile() || server2.sawGetBinFile()) {
			cleanup();
			return 45;
		}
		manager.disconnectAll();
		manager2.disconnectAll();
	}

	{
		FakeAttachmentServer server(
			R"([{"name":"a.pdf","exists":false,"size":0}])",
			{{"a.pdf", "PDFDATA", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "missing" / "a.pdf";
		std::string error;
		if (downloadIfAdvertised(manager, "first", 123, 0, "a.pdf", out.string(), error) ||
			error != "missing" ||
			server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0 ||
			fs::exists(out)) {
			cleanup();
			return 50;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer server(
			R"([{"name":"a.pdf","exists":true,"size":7}])",
			{{"a.pdf", "PDFDATA", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "unknown" / "c.doc";
		std::string error;
		if (downloadIfAdvertised(manager, "first", 123, 0, "c.doc", out.string(), error) ||
			error != "unknown" ||
			server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0) {
			cleanup();
			return 51;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer server("{not-json", {});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");
		std::vector<TelegramAttachmentInfo> items;
		std::string error;
		if (requestTelegramAttachmentList(manager, "first", 1, 0, items, error) ||
			server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0) {
			cleanup();
			return 52;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer server(R"({"name":"a.pdf","exists":true})", {});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");
		std::vector<TelegramAttachmentInfo> items;
		std::string error;
		if (requestTelegramAttachmentList(manager, "first", 1, 0, items, error) ||
			server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0) {
			cleanup();
			return 53;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer primary(
			R"([{"name":"primary.bin","exists":true,"size":4}])",
			{{"primary.bin", "AAAA", PayloadMode::Whole, 0}});
		FakeAttachmentServer second(
			R"([{"name":"second.bin","exists":true,"size":4}])",
			{{"second.bin", "BBBB", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(primary.port()), true},
			{"second", "Second", "127.0.0.1", std::to_string(second.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		const fs::path out = root / "multi" / "second.bin";
		std::string error;
		if (!downloadIfAdvertised(manager, "second", 9, 0, "second.bin", out.string(), error) ||
			readFileBytes(out) != "BBBB") {
			cleanup();
			return 60;
		}
		if (primary.countCommand(COMMAND::GET_TELEGA_ATACHMENTS) != 0 ||
			primary.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0 ||
			primary.countCommand(COMMAND::GETBINFILE) != 0 ||
			second.countCommand(COMMAND::GET_TELEGA_ATACHMENTS) != 1 ||
			second.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 1 ||
			second.sawGetBinFile() || primary.sawGetBinFile()) {
			cleanup();
			return 61;
		}
		manager.disconnectAll();
	}

	cleanup();
	return 0;
}

#endif
