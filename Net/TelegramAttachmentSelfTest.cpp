#include <vcl.h>
#pragma hdrstop

#include "TelegramAttachmentSelfTest.h"

#ifdef _DEBUG

#include "MultiServerManager.h"
#include "TelegramAttachmentClient.h"
#include "utils.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
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

	struct TextPayload
	{
		int telegramId{0};
		int telegramType{0};
		std::string bytes;
	};

	class FakeAttachmentServer
	{
	public:
		FakeAttachmentServer(
			std::string listJson,
			std::vector<AttachmentPayload> payloads,
			std::vector<TextPayload> texts = {})
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, listJson_(std::move(listJson))
			, payloads_(std::move(payloads))
			, texts_(std::move(texts))
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
		bool sawFileText() const { return sawFileText_.load(std::memory_order_acquire); }
		bool sawForbiddenRawDownload() const
		{
			return sawGetBinFile() || sawFileText();
		}

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
			if (body.find("FileName") != std::string::npos)
				return true;
			if (body.find("\"path\"") != std::string::npos)
				return true;
			if (body.find("\"dir\"") != std::string::npos)
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

				if (request.command == COMMAND::GETBINFILE ||
					request.command == COMMAND::FILETEXT) {
					if (request.command == COMMAND::GETBINFILE)
						sawGetBinFile_.store(true, std::memory_order_release);
					else
						sawFileText_.store(true, std::memory_order_release);
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

				if (request.command == COMMAND::GET_TELEGA_TEXT) {
					if (!isScopedListJson(body)) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					int telegramId = 0;
					int telegramType = -1;
					try {
						const auto parsed = nh::json::parse(body);
						telegramId = parsed.at("id").get<int>();
						telegramType = parsed.at("type").get<int>();
					}
					catch (...) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					const TextPayload* payload = nullptr;
					for (const auto& item : texts_) {
						if (item.telegramId == telegramId &&
							item.telegramType == telegramType) {
							payload = &item;
							break;
						}
					}
					if (!payload) {
						failed_.store(true, std::memory_order_release);
						send(socket, COMMAND::SOMEERROR, {});
						return;
					}

					send(socket, request.command, payload->bytes);
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
				if (sawGetBinFile_.load(std::memory_order_acquire) ||
					sawFileText_.load(std::memory_order_acquire))
					failed_.store(true, std::memory_order_release);
			}
		}

		boost::asio::io_context io_;
		tcp::acceptor acceptor_;
		std::unique_ptr<tcp::socket> socket_;
		std::string listJson_;
		std::vector<AttachmentPayload> payloads_;
		std::vector<TextPayload> texts_;
		std::thread worker_;
		mutable std::mutex mutex_;
		std::vector<COMMAND> commands_;
		std::vector<std::string> bodies_;
		std::atomic_bool failed_{false};
		std::atomic_bool sawGetBinFile_{false};
		std::atomic_bool sawFileText_{false};
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

	bool jsonHasNoRawPathFields(const std::string& body)
	{
		try {
			const auto parsed = nh::json::parse(body);
			if (!parsed.is_object())
				return false;
			return !parsed.contains("path") &&
				!parsed.contains("dir") &&
				!parsed.contains("DirectTo") &&
				!parsed.contains("FileName") &&
				!parsed.contains("remotePath");
		}
		catch (...) {
			return false;
		}
	}

	bool sawLegacyRawDownload(const FakeAttachmentServer& server)
	{
		return server.sawForbiddenRawDownload() ||
			server.countCommand(COMMAND::GETBINFILE) != 0 ||
			server.countCommand(COMMAND::FILETEXT) != 0;
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
		std::map<TelegramAttachmentListKey, int> listKeys;
		listKeys.emplace(TelegramAttachmentListKey{"server1", 23, 0}, 1);
		listKeys.emplace(TelegramAttachmentListKey{"server12", 3, 0}, 2);
		if (listKeys.size() != 2 ||
			listKeys.at(TelegramAttachmentListKey{"server1", 23, 0}) ==
				listKeys.at(TelegramAttachmentListKey{"server12", 3, 0})) {
			cleanup();
			return 80;
		}

		std::map<std::pair<std::string, int>, int> archiveKeys;
		archiveKeys.emplace(std::make_pair(std::string{"server1"}, 23), 1);
		archiveKeys.emplace(std::make_pair(std::string{"server12"}, 3), 2);
		if (archiveKeys.size() != 2) {
			cleanup();
			return 81;
		}
	}

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
			server.countCommand(COMMAND::FILETEXT) != 0 ||
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
		const std::string utf8Name = conv::to_string(std::wstring(
			L"\x043F\x0440\x0438\x043B\x043E\x0436\x0435\x043D\x0438\x0435 \x2116" L"1.pdf"));
		const nh::json listJson = nh::json::array({
			{{"name", utf8Name}, {"exists", true}, {"size", 9}}
		});
		FakeAttachmentServer server(
			listJson.dump(),
			{{utf8Name, "UTF8BYTES", PayloadMode::Whole, 0}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		std::vector<TelegramAttachmentInfo> items;
		std::string error;
		if (!requestTelegramAttachmentList(manager, "first", 321, 0, items, error) ||
			items.size() != 1 ||
			items[0].name != utf8Name ||
			findAdvertisedTelegramAttachment(items, utf8Name) == nullptr) {
			cleanup();
			return 15;
		}

		const fs::path out = root / "utf8" / "payload.pdf";
		if (!downloadIfAdvertised(manager, "first", 321, 0, utf8Name, out.string(), error)) {
			cleanup();
			return 16;
		}
		if (readFileBytes(out) != "UTF8BYTES") {
			cleanup();
			return 17;
		}
		if (server.sawGetBinFile() ||
			server.countCommand(COMMAND::GETBINFILE) != 0 ||
			!jsonHasFileName(
				server.lastBodyFor(COMMAND::GET_TELEGA_SINGLE_ATACHMENT), utf8Name)) {
			cleanup();
			return 18;
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
		if (sawLegacyRawDownload(primary) || sawLegacyRawDownload(second) ||
			primary.countCommand(COMMAND::GET_TELEGA_ATACHMENTS) != 0 ||
			primary.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0 ||
			second.countCommand(COMMAND::GET_TELEGA_ATACHMENTS) != 1 ||
			second.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 1) {
			cleanup();
			return 61;
		}
		manager.disconnectAll();
	}

	{
		const std::vector<std::string> badNames = {
			"../456.zip",
			"..\\456.zip",
			"dir/456.zip",
			"C:\\tmp\\456.zip"
		};
		int caseIndex = 0;
		for (const auto& badName : badNames) {
			const nh::json listJson = nh::json::array({
				{{"name", badName}, {"exists", true}, {"size", 8}}
			});
			FakeAttachmentServer server(
				listJson.dump(),
				{{"456.zip", "ZIPBYTES", PayloadMode::Whole, 0}});
			MultiServerManager manager;
			manager.configure({
				{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
			}, "first");
			manager.setUser("self-test-user");

			const fs::path out = root / ("bad" + std::to_string(caseIndex)) / "456.zip";
			std::vector<TelegramAttachmentInfo> items;
			std::string error;
			if (requestTelegramAttachmentList(manager, "first", 456, 1, items, error) ||
				!items.empty()) {
				cleanup();
				return 70 + caseIndex;
			}
			if (downloadIfAdvertised(manager, "first", 456, 1, "456.zip", out.string(), error) ||
				server.countCommand(COMMAND::GET_TELEGA_SINGLE_ATACHMENT) != 0 ||
				fs::exists(out) ||
				server.countCommand(COMMAND::GETBINFILE) != 0 ||
				server.sawGetBinFile()) {
				cleanup();
				return 74 + caseIndex;
			}
			manager.disconnectAll();
			++caseIndex;
		}
	}

	{
		if (static_cast<std::uint64_t>(COMMAND::GET_TELEGA_TEXT) != 32ull ||
			static_cast<std::uint64_t>(COMMAND::FILETEXT) != 2ull ||
			static_cast<std::uint64_t>(COMMAND::GETBINFILE) != 11ull) {
			cleanup();
			return 90;
		}

		FakeAttachmentServer server(
			"[]",
			{},
			{
				{123, 0, "TELEGA-VHOD"},
				{456, 1, "TELEGA-ISHOD"}
			});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		std::string vhod;
		std::string error;
		if (!requestTelegramText(manager, "first", 123, 0, vhod, error) ||
			vhod != "TELEGA-VHOD") {
			cleanup();
			return 91;
		}
		const std::string vhodBody = server.lastBodyFor(COMMAND::GET_TELEGA_TEXT);
		if (server.countCommand(COMMAND::GET_TELEGA_TEXT) != 1 ||
			!jsonHasIdType(vhodBody, 123, 0) ||
			!jsonHasNoRawPathFields(vhodBody) ||
			sawLegacyRawDownload(server) ||
			server.failed()) {
			cleanup();
			return 92;
		}

		std::string ishod;
		if (!requestTelegramText(manager, "first", 456, 1, ishod, error) ||
			ishod != "TELEGA-ISHOD") {
			cleanup();
			return 93;
		}
		const std::string ishodBody = server.lastBodyFor(COMMAND::GET_TELEGA_TEXT);
		if (server.countCommand(COMMAND::GET_TELEGA_TEXT) != 2 ||
			!jsonHasIdType(ishodBody, 456, 1) ||
			!jsonHasNoRawPathFields(ishodBody) ||
			sawLegacyRawDownload(server) ||
			server.countCommand(COMMAND::GETBINFILE) != 0 ||
			server.countCommand(COMMAND::FILETEXT) != 0 ||
			server.failed()) {
			cleanup();
			return 94;
		}
		manager.disconnectAll();
	}

	{
		FakeAttachmentServer primary(
			"[]",
			{},
			{{9, 0, "PRIMARY-TEXT"}});
		FakeAttachmentServer second(
			"[]",
			{},
			{{9, 0, "SECOND-TEXT"}});
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(primary.port()), true},
			{"second", "Second", "127.0.0.1", std::to_string(second.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		std::string text;
		std::string error;
		if (!requestTelegramText(manager, "second", 9, 0, text, error) ||
			text != "SECOND-TEXT") {
			cleanup();
			return 95;
		}
		if (primary.countCommand(COMMAND::GET_TELEGA_TEXT) != 0 ||
			second.countCommand(COMMAND::GET_TELEGA_TEXT) != 1 ||
			sawLegacyRawDownload(primary) ||
			sawLegacyRawDownload(second) ||
			!jsonHasIdType(second.lastBodyFor(COMMAND::GET_TELEGA_TEXT), 9, 0)) {
			cleanup();
			return 96;
		}
		manager.disconnectAll();
	}

	cleanup();
	return 0;
}

#endif
