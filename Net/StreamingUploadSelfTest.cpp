#include <vcl.h>
#pragma hdrstop

#include "StreamingUploadSelfTest.h"

#ifdef _DEBUG

#include "MultiServerManager.h"
#include "ClientFilePublish.h"

#include <nlohmann/json.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <algorithm>
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

	enum class UploadBehavior
	{
		Success,
		ErrorBeforeReady,
		InterruptAfterReady
	};

	class FakeUploadServer
	{
	public:
		explicit FakeUploadServer(UploadBehavior behavior = UploadBehavior::Success)
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, behavior_(behavior)
			, worker_([this]() { run(); })
		{
		}

		~FakeUploadServer()
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
		bool sawBytesBeforeReady() const
		{
			return sawBytesBeforeReady_.load(std::memory_order_acquire);
		}
		std::uint64_t receivedBytes() const
		{
			return receivedBytes_.load(std::memory_order_acquire);
		}
		std::uint64_t receivedChecksum() const
		{
			return receivedChecksum_.load(std::memory_order_acquire);
		}
		std::size_t maxChunk() const
		{
			return maxChunk_.load(std::memory_order_acquire);
		}
		COMMAND lastUploadCommand() const
		{
			return lastUploadCommand_.load(std::memory_order_acquire);
		}
		std::string lastMetadata() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return lastMetadata_;
		}

	private:
		void send(tcp::socket& socket, COMMAND command, const std::string& body)
		{
			header response{};
			response.command = command;
			response.size = static_cast<uint_fast64_t>(body.size());
			boost::asio::write(socket, boost::asio::buffer(&response, sizeof(response)));
			if (!body.empty())
				boost::asio::write(socket, boost::asio::buffer(body.data(), body.size()));
		}

		void sendTypedError(tcp::socket& socket, search_protocol::ErrorCode code)
		{
			search_protocol::ErrorResponseV1 payload{};
			payload.errorCode = static_cast<std::uint32_t>(code);
			header response{};
			response.command = COMMAND::ERROR_RESPONSE;
			response.size = sizeof(payload);
			boost::asio::write(socket, boost::asio::buffer(&response, sizeof(response)));
			boost::asio::write(socket, boost::asio::buffer(&payload, sizeof(payload)));
		}

		void readRequest(tcp::socket& socket, header& request, std::string& body)
		{
			boost::asio::read(socket, boost::asio::buffer(&request, sizeof(request)));
			body.assign(static_cast<std::size_t>(request.size), '\0');
			if (!body.empty())
				boost::asio::read(socket, boost::asio::buffer(&body[0], body.size()));
		}

		static bool metadataLooksExact(const std::string& body, std::string* fileName, std::uint64_t* fileSize)
		{
			try {
				const auto parsed = nh::json::parse(body);
				if (!parsed.is_object() || parsed.size() != 3)
					return false;
				if (!parsed.contains("version") ||
					!parsed.contains("file_name") ||
					!parsed.contains("file_size"))
					return false;
				if (parsed.value("version", 0) != 1)
					return false;
				if (!parsed["file_name"].is_string() ||
					!parsed["file_size"].is_number_integer())
					return false;
				const auto name = parsed["file_name"].get<std::string>();
				if (name.empty() ||
					name.find('/') != std::string::npos ||
					name.find('\\') != std::string::npos ||
					name.find(':') != std::string::npos)
					return false;
				static const char* forbidden[] = {
					"path", "dir", "directory", "user", "operator",
					"target", "destination", "remotePath"};
				for (const char* key : forbidden) {
					if (parsed.contains(key))
						return false;
				}
				if (fileName)
					*fileName = name;
				if (fileSize)
					*fileSize = parsed["file_size"].get<std::uint64_t>();
				return true;
			}
			catch (...) {
				return false;
			}
		}

		void serve(tcp::socket& socket)
		{
			for (;;) {
				header request{};
				std::string body;
				readRequest(socket, request, body);

				if (request.command == COMMAND::USER_REGISTRY) {
					send(socket, request.command, "OK");
					continue;
				}

				if (request.command != COMMAND::UPLOAD_TLG_TO_SEND_V1 &&
					request.command != COMMAND::UPLOAD_RAZN_V1) {
					failed_.store(true, std::memory_order_release);
					send(socket, COMMAND::SOMEERROR, {});
					return;
				}

				lastUploadCommand_.store(request.command, std::memory_order_release);
				{
					std::lock_guard<std::mutex> lock(mutex_);
					lastMetadata_ = body;
				}

				std::string fileName;
				std::uint64_t fileSize = 0;
				if (!metadataLooksExact(body, &fileName, &fileSize)) {
					failed_.store(true, std::memory_order_release);
					sendTypedError(socket, search_protocol::ErrorCode::InvalidRequest);
					return;
				}

				boost::system::error_code availableError;
				if (socket.available(availableError) > 0 && !availableError)
					sawBytesBeforeReady_.store(true, std::memory_order_release);

				if (behavior_ == UploadBehavior::ErrorBeforeReady) {
					sendTypedError(socket, search_protocol::ErrorCode::InvalidRequest);
					return;
				}

				send(socket, request.command, nh::json{
					{"version", 1},
					{"ready", true}}.dump());

				if (behavior_ == UploadBehavior::InterruptAfterReady) {
					if (fileSize > 0) {
						std::array<char, 1024> probe{};
						const std::size_t n = static_cast<std::size_t>(
							(std::min)(fileSize, static_cast<std::uint64_t>(probe.size())));
						boost::asio::read(socket, boost::asio::buffer(probe.data(), n));
						receivedBytes_.store(n, std::memory_order_release);
					}
					boost::system::error_code ignored;
					socket.shutdown(tcp::socket::shutdown_both, ignored);
					socket.close(ignored);
					return;
				}

				std::array<char, 64u * 1024u> chunk{};
				std::uint64_t remaining = fileSize;
				std::uint64_t checksum = 0;
				std::size_t maxChunk = 0;
				while (remaining > 0) {
					const std::size_t n = static_cast<std::size_t>(
						(std::min)(static_cast<std::uint64_t>(chunk.size()), remaining));
					boost::asio::read(socket, boost::asio::buffer(chunk.data(), n));
					maxChunk = (std::max)(maxChunk, n);
					for (std::size_t i = 0; i < n; ++i)
						checksum += static_cast<unsigned char>(chunk[i]);
					remaining -= n;
					receivedBytes_.store(fileSize - remaining, std::memory_order_release);
				}
				receivedChecksum_.store(checksum, std::memory_order_release);
				maxChunk_.store(maxChunk, std::memory_order_release);

				send(socket, request.command, nh::json{
					{"version", 1},
					{"ok", true},
					{"saved_name", fileName}}.dump());
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
				serve(*socket_);
			}
			catch (...) {
				failed_.store(true, std::memory_order_release);
			}
		}

		boost::asio::io_context io_;
		tcp::acceptor acceptor_;
		std::unique_ptr<tcp::socket> socket_;
		UploadBehavior behavior_;
		std::thread worker_;
		mutable std::mutex mutex_;
		std::string lastMetadata_;
		std::atomic<COMMAND> lastUploadCommand_{COMMAND::SOMEERROR};
		std::atomic_bool failed_{false};
		std::atomic_bool sawBytesBeforeReady_{false};
		std::atomic<std::uint64_t> receivedBytes_{0};
		std::atomic<std::uint64_t> receivedChecksum_{0};
		std::atomic<std::size_t> maxChunk_{0};
	};

	bool writePatternFile(const fs::path& path, std::uint64_t size, std::uint64_t* checksum)
	{
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out)
			return false;
		std::array<char, 64u * 1024u> chunk{};
		std::uint64_t offset = 0;
		std::uint64_t sum = 0;
		while (offset < size) {
			const std::size_t n = static_cast<std::size_t>(
				(std::min)(static_cast<std::uint64_t>(chunk.size()), size - offset));
			for (std::size_t i = 0; i < n; ++i) {
				chunk[i] = static_cast<char>((offset + i) & 0xFFu);
				sum += static_cast<unsigned char>(chunk[i]);
			}
			out.write(chunk.data(), static_cast<std::streamsize>(n));
			offset += n;
		}
		if (checksum)
			*checksum = sum;
		return static_cast<bool>(out);
	}

	std::string readTextFile(const fs::path& path)
	{
		std::ifstream in(path, std::ios::binary);
		return {
			std::istreambuf_iterator<char>(in),
			std::istreambuf_iterator<char>()};
	}

	bool metadataHasExactShape(const std::string& body, const std::string& fileName, std::uint64_t size)
	{
		try {
			const auto parsed = nh::json::parse(body);
			return parsed.is_object() &&
				parsed.size() == 3 &&
				parsed.value("version", 0) == 1 &&
				parsed.value("file_name", std::string{}) == fileName &&
				parsed.value("file_size", std::uint64_t{1}) == size &&
				!parsed.contains("user") &&
				!parsed.contains("operator") &&
				!parsed.contains("path") &&
				!parsed.contains("dir");
		}
		catch (...) {
			return false;
		}
	}

	int runOneUpload(
		COMMAND command,
		const fs::path& localFile,
		const std::string& expectedName,
		std::uint64_t expectedSize,
		std::uint64_t expectedChecksum,
		int failBase)
	{
		FakeUploadServer server;
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");

		std::string saved;
		std::string error;
		if (!manager.uploadFileV1("first", localFile.wstring(), command, &saved, &error))
			return failBase;
		if (server.failed() || server.sawBytesBeforeReady())
			return failBase + 1;
		if (server.lastUploadCommand() != command)
			return failBase + 2;
		if (!metadataHasExactShape(server.lastMetadata(), expectedName, expectedSize))
			return failBase + 3;
		if (server.receivedBytes() != expectedSize)
			return failBase + 4;
		if (expectedSize > 0 && server.maxChunk() > 64u * 1024u)
			return failBase + 5;
		if (server.receivedChecksum() != expectedChecksum)
			return failBase + 6;
		if (saved != expectedName)
			return failBase + 7;
		manager.disconnectAll();
		return 0;
	}
}

bool IsStreamingUploadSelfTestInvocation()
{
	return ParamCount() >= 1 && SameText(ParamStr(1), L"--streaming-upload-self-test");
}

int RunStreamingUploadSelfTestFromCommandLine()
{
	const fs::path root = fs::temp_directory_path() / "sc_streaming_upload_self_test";
	std::error_code ec;
	fs::remove_all(root, ec);
	fs::create_directories(root, ec);
	if (ec)
		return 1;
	auto cleanup = [&]() { fs::remove_all(root, ec); };

	{
		const fs::path clientCpp = fs::path(__FILE__).parent_path() / "myAsioClient.cpp";
		const fs::path formCpp = fs::path(__FILE__).parent_path().parent_path() / "formMain.cpp";
		const auto clientText = readTextFile(clientCpp);
		const auto formText = readTextFile(formCpp);
		if (clientText.find("bool MyAsioClient::uploadFileV1") == std::string::npos ||
			clientText.find("loadFileToVector2") != std::string::npos ||
			clientText.find("FileData") != std::string::npos ||
			clientText.find("serializeToBytes") != std::string::npos) {
			cleanup();
			return 2;
		}
		if (formText.find("COMMAND::LOAD_TLG_TO_SEND") != std::string::npos ||
			formText.find("COMMAND::LOAD_RAZN") != std::string::npos ||
			formText.find("COMMAND::UPLOAD_TLG_TO_SEND_V1") == std::string::npos ||
			formText.find("COMMAND::UPLOAD_RAZN_V1") == std::string::npos ||
			formText.find("loadFileToVector2") != std::string::npos ||
			formText.find("FileData") != std::string::npos ||
			formText.find("serializeToBytes") != std::string::npos ||
			formText.find("conv::to_wstring(user_) + L\"\\\\\"") != std::string::npos) {
			cleanup();
			return 3;
		}
	}

	{
		const fs::path file = root / "note.txt";
		std::uint64_t checksum = 0;
		if (!writePatternFile(file, 4, &checksum)) {
			cleanup();
			return 10;
		}
		const int rc = runOneUpload(
			COMMAND::UPLOAD_TLG_TO_SEND_V1, file, "note.txt", 4, checksum, 10);
		if (rc != 0) {
			cleanup();
			return rc;
		}
	}

	{
		const fs::path file = root / "razn.dat";
		std::uint64_t checksum = 0;
		if (!writePatternFile(file, 8, &checksum)) {
			cleanup();
			return 20;
		}
		const int rc = runOneUpload(
			COMMAND::UPLOAD_RAZN_V1, file, "razn.dat", 8, checksum, 20);
		if (rc != 0) {
			cleanup();
			return rc;
		}
	}

	{
		const fs::path file = root / L"файл.txt";
		std::uint64_t checksum = 0;
		if (!writePatternFile(file, 3, &checksum)) {
			cleanup();
			return 30;
		}
		const std::string expected = client_file_publish::Narrow(file.filename().wstring());
		const int rc = runOneUpload(
			COMMAND::UPLOAD_RAZN_V1, file, expected, 3, checksum, 30);
		if (rc != 0) {
			cleanup();
			return rc;
		}
	}

	{
		const fs::path file = root / "empty.bin";
		{
			std::ofstream out(file, std::ios::binary | std::ios::trunc);
		}
		const int rc = runOneUpload(
			COMMAND::UPLOAD_RAZN_V1, file, "empty.bin", 0, 0, 40);
		if (rc != 0) {
			cleanup();
			return rc;
		}
	}

	{
		const fs::path file = root / "big.bin";
		std::uint64_t checksum = 0;
		constexpr std::uint64_t kSize = 64ull * 1024ull * 1024ull;
		if (!writePatternFile(file, kSize, &checksum)) {
			cleanup();
			return 50;
		}
		const int rc = runOneUpload(
			COMMAND::UPLOAD_TLG_TO_SEND_V1, file, "big.bin", kSize, checksum, 50);
		if (rc != 0) {
			cleanup();
			return rc;
		}
	}

	{
		const fs::path file = root / "before-ready.bin";
		std::uint64_t checksum = 0;
		if (!writePatternFile(file, 16, &checksum)) {
			cleanup();
			return 60;
		}
		FakeUploadServer server(UploadBehavior::ErrorBeforeReady);
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");
		std::string saved;
		std::string error;
		const bool ok = manager.uploadFileV1(
			"first", file.wstring(), COMMAND::UPLOAD_RAZN_V1, &saved, &error);
		if (ok || saved.size() != 0) {
			cleanup();
			return 61;
		}
		if (server.receivedBytes() != 0 || server.sawBytesBeforeReady()) {
			cleanup();
			return 62;
		}
		manager.disconnectAll();
	}

	{
		const fs::path file = root / "interrupt.bin";
		std::uint64_t checksum = 0;
		if (!writePatternFile(file, 32 * 1024, &checksum)) {
			cleanup();
			return 70;
		}
		FakeUploadServer server(UploadBehavior::InterruptAfterReady);
		MultiServerManager manager;
		manager.configure({
			{"first", "First", "127.0.0.1", std::to_string(server.port()), true}
		}, "first");
		manager.setUser("self-test-user");
		std::string saved;
		std::string error;
		const bool ok = manager.uploadFileV1(
			"first", file.wstring(), COMMAND::UPLOAD_TLG_TO_SEND_V1, &saved, &error);
		if (ok) {
			cleanup();
			return 71;
		}
		if (manager.isConnected("first")) {
			cleanup();
			return 72;
		}
	}

	cleanup();
	return 0;
}

#endif
