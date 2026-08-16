//---------------------------------------------------------------------------

#pragma hdrstop

#include "myAsioClient.h"
#include "ClientFilePublish.h"
#include "UserFileReplace.h"
#include <algorithm>
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <vector>
#include <map>
#include <windows.h>
#include <nlohmann/json.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

// Единый idle-таймаут для чтения с сокета (как SocketFileSaver): без новых байт N секунд — ошибка.
constexpr auto kSocketReadIdleTimeout = std::chrono::seconds(10);

// Fail-closed ceiling for in-memory response payloads (requestBinary / requestText).
// Streamed downloads (requestFileToPath) are not bound by this RAM limit.
constexpr std::uint64_t kMaxInMemoryPayloadBytes = 1024ull * 1024ull * 1024ull; // 1 GiB

using ErrorMessages = std::map<std::uint32_t, std::string>;

std::filesystem::path errorMessagesPath()
{
	wchar_t exe[MAX_PATH]{};
	const DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
	if (n == 0 || n >= MAX_PATH) return {};
	const auto exeDir = std::filesystem::path(exe).parent_path();
	const std::filesystem::path candidates[] = {
		exeDir / L"config" / L"server-error-messages.json",
		// IDE Debug/Release under Win32\<Config>\ — project config two levels up.
		exeDir / L".." / L".." / L"config" / L"server-error-messages.json",
		exeDir / L".." / L"config" / L"server-error-messages.json",
	};
	for (const auto& candidate : candidates) {
		std::error_code ec;
		const auto canonical = std::filesystem::weakly_canonical(candidate, ec);
		if (ec) continue;
		if (std::filesystem::is_regular_file(canonical, ec) && !ec)
			return canonical;
	}
	return exeDir / L"config" / L"server-error-messages.json";
}

const ErrorMessages& loadedErrorMessages() noexcept
{
	static const ErrorMessages messages = [] {
		ErrorMessages result;
		static const char* const expectedNames[44] = { nullptr,
			"PayloadTooLarge", "InvalidCommand", "CommandNotRegistered", "ServerBusy",
			"ServerStopping", "InvalidRequest", "InvalidJson", "InvalidBinaryPayload",
			"FileNotFound", "FileMetadataFailed", "FileOpenFailed", "FileReadFailed",
			"FileWriteFailed", "DirectoryCreateFailed", "ConfigurationError",
			"DatabaseOpenFailed", "DatabaseBusy", "DatabaseQueryFailed", "DatabaseSchemaFailed",
			"SerializationFailed", "IndexUnavailable", "IndexDataInconsistent",
			"IndexUpdateInProgress", "IndexUpdateFailed", "OperatorNotRegistered",
			"AttachmentNotFound", "MessageNotFound", "MessageSaveFailed", "MessageReadFailed",
			"ResponseQueueFull", "CommandExecutionFailed", "InternalError",
			"AuthFailed", "AuthClientDisabled", "AuthRequired",
			"AuthClientIdMissing", "AuthClientNameMissing", "AuthFlashSerialMissing",
			"AuthSignatureMissing", "AuthClientIdNotFound", "AuthClientNameMismatch",
			"AuthFlashSerialMismatch", "AuthSignatureInvalid" };
		try {
			std::ifstream input(errorMessagesPath(), std::ios::binary);
			if (!input) return result;
			nlohmann::json document = nlohmann::json::parse(input);
			if (document.at("formatVersion").get<int>() != 1) return ErrorMessages{};
			const auto& errors = document.at("errors");
			if (!errors.is_array()) return ErrorMessages{};
			for (const auto& item : errors) {
				const auto code = item.at("code").get<std::uint32_t>();
				const auto name = item.at("name").get<std::string>();
				const auto message = item.at("message").get<std::string>();
				if (code < 1 || code > 43 || name != expectedNames[code] ||
					message.empty() || !result.emplace(code, message).second) return ErrorMessages{};
			}
			if (result.size() != 43) return ErrorMessages{};
		} catch (...) { return ErrorMessages{}; }
		return result;
	}();
	return messages;
}

} // namespace

std::string serverErrorUserMessage(std::uint32_t rawCode)
{
	// Keep these as explicit UTF-8 bytes. Ordinary Cyrillic literals may be
	// compiled as CP1251 on this toolchain, while JSON messages are UTF-8.
	static const char kInternal[] =
		"\xD0\x9D\xD0\xB0\x20\xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB5\x20"
		"\xD0\xBF\xD1\x80\xD0\xBE\xD0\xB8\xD0\xB7\xD0\xBE\xD1\x88\xD0\xBB\xD0\xB0\x20"
		"\xD0\xB2\xD0\xBD\xD1\x83\xD1\x82\xD1\x80\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8F\xD1\x8F\x20"
		"\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB0\x2E\x20"
		"\xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB8\x3A\x20";
	static const char kUnknown[] =
		"\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\x20"
		"\xD0\xB2\xD0\xB5\xD1\x80\xD0\xBD\xD1\x83\xD0\xBB\x20"
		"\xD0\xBD\xD0\xB5\xD0\xB8\xD0\xB7\xD0\xB2\xD0\xB5\xD1\x81\xD1\x82\xD0\xBD\xD1\x83\xD1\x8E\x20"
		"\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD1\x83\x2E\x20"
		"\xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB8\x3A\x20";
	static const char kCodeSuffix[] =
		"\x20\xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB8\x3A\x20";

	const auto& messages = loadedErrorMessages();
	if (messages.empty())
		return std::string(kInternal) + std::to_string(rawCode) + ".";
	const auto it = messages.find(rawCode);
	if (it == messages.end())
		return std::string(kUnknown) + std::to_string(rawCode) + ".";
	return it->second + std::string(kCodeSuffix) + std::to_string(rawCode) + ".";
}

std::string serverErrorCodeName(std::uint32_t rawCode)
{
	using search_protocol::ErrorCode;
	if (rawCode < 1 ||
		rawCode > static_cast<std::uint32_t>(ErrorCode::AuthSignatureInvalid))
		return "UnknownErrorCode";

	switch (static_cast<ErrorCode>(rawCode)) {
	case ErrorCode::PayloadTooLarge: return "PayloadTooLarge";
	case ErrorCode::InvalidCommand: return "InvalidCommand";
	case ErrorCode::CommandNotRegistered: return "CommandNotRegistered";
	case ErrorCode::ServerBusy: return "ServerBusy";
	case ErrorCode::ServerStopping: return "ServerStopping";
	case ErrorCode::InvalidRequest: return "InvalidRequest";
	case ErrorCode::InvalidJson: return "InvalidJson";
	case ErrorCode::InvalidBinaryPayload: return "InvalidBinaryPayload";
	case ErrorCode::FileNotFound: return "FileNotFound";
	case ErrorCode::FileMetadataFailed: return "FileMetadataFailed";
	case ErrorCode::FileOpenFailed: return "FileOpenFailed";
	case ErrorCode::FileReadFailed: return "FileReadFailed";
	case ErrorCode::FileWriteFailed: return "FileWriteFailed";
	case ErrorCode::DirectoryCreateFailed: return "DirectoryCreateFailed";
	case ErrorCode::ConfigurationError: return "ConfigurationError";
	case ErrorCode::DatabaseOpenFailed: return "DatabaseOpenFailed";
	case ErrorCode::DatabaseBusy: return "DatabaseBusy";
	case ErrorCode::DatabaseQueryFailed: return "DatabaseQueryFailed";
	case ErrorCode::DatabaseSchemaFailed: return "DatabaseSchemaFailed";
	case ErrorCode::SerializationFailed: return "SerializationFailed";
	case ErrorCode::IndexUnavailable: return "IndexUnavailable";
	case ErrorCode::IndexDataInconsistent: return "IndexDataInconsistent";
	case ErrorCode::IndexUpdateInProgress: return "IndexUpdateInProgress";
	case ErrorCode::IndexUpdateFailed: return "IndexUpdateFailed";
	case ErrorCode::OperatorNotRegistered: return "OperatorNotRegistered";
	case ErrorCode::AttachmentNotFound: return "AttachmentNotFound";
	case ErrorCode::MessageNotFound: return "MessageNotFound";
	case ErrorCode::MessageSaveFailed: return "MessageSaveFailed";
	case ErrorCode::MessageReadFailed: return "MessageReadFailed";
	case ErrorCode::ResponseQueueFull: return "ResponseQueueFull";
	case ErrorCode::CommandExecutionFailed: return "CommandExecutionFailed";
	case ErrorCode::InternalError: return "InternalError";
	case ErrorCode::AuthFailed: return "AuthFailed";
	case ErrorCode::AuthClientDisabled: return "AuthClientDisabled";
	case ErrorCode::AuthRequired: return "AuthRequired";
	case ErrorCode::AuthClientIdMissing: return "AuthClientIdMissing";
	case ErrorCode::AuthClientNameMissing: return "AuthClientNameMissing";
	case ErrorCode::AuthFlashSerialMissing: return "AuthFlashSerialMissing";
	case ErrorCode::AuthSignatureMissing: return "AuthSignatureMissing";
	case ErrorCode::AuthClientIdNotFound: return "AuthClientIdNotFound";
	case ErrorCode::AuthClientNameMismatch: return "AuthClientNameMismatch";
	case ErrorCode::AuthFlashSerialMismatch: return "AuthFlashSerialMismatch";
	case ErrorCode::AuthSignatureInvalid: return "AuthSignatureInvalid";
	}
	return "UnknownErrorCode";
}

bool isAuthRejectionErrorCode(std::uint32_t rawCode) noexcept
{
	using search_protocol::ErrorCode;
	return rawCode >= static_cast<std::uint32_t>(ErrorCode::AuthFailed) &&
		rawCode <= static_cast<std::uint32_t>(ErrorCode::AuthSignatureInvalid);
}
std::string legacyServerErrorUserMessage()
{
	return std::string(
		"\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\x20"
		"\xD1\x81\xD0\xBE\xD0\xBE\xD0\xB1\xD1\x89\xD0\xB8\xD0\xBB\x20"
		"\xD0\xBE\x20\xD0\xB2\xD0\xBD\xD1\x83\xD1\x82\xD1\x80\xD0\xB5\xD0\xBD\xD0\xBD\xD0\xB5\xD0\xB9\x20"
		"\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB5\x2E\x20"
		"\xD0\x9F\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80\xD0\xB8\xD1\x82\xD0\xB5\x20"
		"\xD0\xBF\xD0\xBE\xD0\xBF\xD1\x8B\xD1\x82\xD0\xBA\xD1\x83\x20"
		"\xD0\xB8\xD0\xBB\xD0\xB8\x20\xD0\xBE\xD0\xB1\xD1\x80\xD0\xB0\xD1\x82\xD0\xB8\xD1\x82\xD0\xB5\xD1\x81\xD1\x8C\x20"
		"\xD0\xBA\x20\xD0\xB0\xD0\xB4\xD0\xBC\xD0\xB8\xD0\xBD\xD0\xB8\xD1\x81\xD1\x82\xD1\x80\xD0\xB0\xD1\x82\xD0\xBE\xD1\x80\xD1\x83\x2E");
}
std::string busyServerErrorUserMessage()
{
	return std::string(
		"\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\x20"
		"\xD0\xB7\xD0\xB0\xD0\xBD\xD1\x8F\xD1\x82\x2E\x20"
		"\xD0\x9F\xD0\xBE\xD0\xB2\xD1\x82\xD0\xBE\xD1\x80\xD0\xB8\xD1\x82\xD0\xB5\x20"
		"\xD0\xBF\xD0\xBE\xD0\xBF\xD1\x8B\xD1\x82\xD0\xBA\xD1\x83\x20"
		"\xD0\xBF\xD0\xBE\xD0\xB7\xD0\xB6\xD0\xB5\x2E");
}

namespace {

bool read_exact_idle_timeout(boost::asio::ip::tcp::socket& socket,
                             void* dest,
                             std::size_t len,
                             std::string* errMsg)
{
	struct ReadState {
		boost::system::error_code readEc;
		std::size_t bytesRead{0};
		bool readDone{false};
		bool timerDone{false};
		bool timedOut{false};
	};

	auto* p = static_cast<std::uint8_t*>(dest);
	std::size_t totalRead = 0;
	constexpr std::size_t kBuf = 64 * 1024;
	std::vector<std::uint8_t> buffer(kBuf);
	boost::asio::steady_timer timer(socket.get_executor());

	while (totalRead < len) {
		const std::size_t need = len - totalRead;
		const std::size_t toRead = (std::min)(need, buffer.size());
		auto state = std::make_shared<ReadState>();

		socket.async_read_some(
			boost::asio::buffer(buffer.data(), toRead),
			[state, &timer](const boost::system::error_code& ec, std::size_t n) {
				state->readEc = ec;
				state->bytesRead = n;
				state->readDone = true;
				boost::system::error_code ignored;
				timer.cancel(ignored);
			});

		timer.expires_after(kSocketReadIdleTimeout);
		timer.async_wait([state, &socket](const boost::system::error_code& ec) {
			state->timerDone = true;
			if (!ec && !state->readDone) {
				state->timedOut = true;
				boost::system::error_code ignored;
				socket.cancel(ignored);
			}
		});

		auto& ctx = static_cast<boost::asio::io_context&>(socket.get_executor().context());
		ctx.restart();
		while (!state->readDone || !state->timerDone) {
			if (ctx.run_one() == 0) {
				if (errMsg)
					*errMsg = "Внутренняя ошибка ожидания операции чтения";
				return false;
			}
		}

		if (state->timedOut) {
			if (errMsg)
				*errMsg = "Таймаут чтения из сокета (idle " +
					std::to_string(kSocketReadIdleTimeout.count()) + " с)";
			return false;
		}
		if (state->readEc) {
			if (errMsg)
				*errMsg = state->readEc.message();
			return false;
		}
		if (state->bytesRead == 0) {
			if (errMsg)
				*errMsg = "Соединение закрыто до завершения чтения";
			return false;
		}
		std::memcpy(p + totalRead, buffer.data(), state->bytesRead);
		totalRead += state->bytesRead;
	}
	return true;
}

bool consumeServerErrorResponse(boost::asio::ip::tcp::socket& socket,
								const header& response,
								std::string* errMsg,
								const char* legacySomeErrorMessage = "Сервер вернул ошибку",
								search_protocol::ServerErrorInfo* serverError = nullptr)
{
	auto closeTransport = [&socket]() {
		boost::system::error_code ignored;
		socket.close(ignored);
	};

	if (response.command == COMMAND::SOMEERROR) {
		if (errMsg)
			*errMsg = legacyServerErrorUserMessage();
		closeTransport();
		return true;
	}

	if (response.command == COMMAND::SERVER_BUSY_ERROR) {
		if (serverError) {
			serverError->set(
				search_protocol::ErrorCode::ServerBusy,
				false);
		}
		if (errMsg)
			*errMsg = busyServerErrorUserMessage();
		closeTransport();
		return true;
	}

	if (response.command != COMMAND::ERROR_RESPONSE)
		return false;

	if (response.size != sizeof(search_protocol::ErrorResponseV1)) {
		if (errMsg) {
			*errMsg = "Некорректный ERROR_RESPONSE: размер " +
				std::to_string(static_cast<std::uint64_t>(response.size)) +
				", ожидалось " +
				std::to_string(sizeof(search_protocol::ErrorResponseV1));
		}
		closeTransport();
		return true;
	}

	search_protocol::ErrorResponseV1 payload{};
	if (!read_exact_idle_timeout(socket, &payload, sizeof(payload), errMsg)) {
		closeTransport();
		return true;
	}

	if (payload.version != search_protocol::ERROR_RESPONSE_VERSION) {
		if (errMsg) {
			*errMsg = "Неподдерживаемая версия ERROR_RESPONSE: " +
				std::to_string(payload.version);
		}
		closeTransport();
		return true;
	}
	if (serverError)
		serverError->setRaw(payload.errorCode, true);

	if (payload.errorCode ==
		static_cast<std::uint32_t>(search_protocol::ErrorCode::ServerBusy)) {
		if (errMsg)
			*errMsg = busyServerErrorUserMessage();
		closeTransport();
		return true;
	}

	if (errMsg) {
		*errMsg = serverErrorUserMessage(payload.errorCode);
	}
	closeTransport();
	return true;
}

} // namespace

void MyAsioClient::disConnect()
{
	std::lock_guard<std::mutex> lk(io_mutex_);

	if (socket_) {
        boost::system::error_code ec;
		if (socket_->is_open()) {
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_->close(ec);
			// òóò ìîæíî ëîãèðîâàòü ec, åñëè íóæíî
        }
		delete socket_;
		socket_ = nullptr;
	}

	if (resolver_) {
        delete resolver_;
		resolver_ = nullptr;
	}

}

bool MyAsioClient::isConnected()
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	return socket_ && socket_->is_open();
}

bool MyAsioClient::requestBinFile(const std::string& path,
								  std::vector<uint8_t>& out,
								  std::atomic_uint64_t* progress,
								  std::string* errMsg)
{
	return requestBinary(
		std::vector<uint8_t>(path.begin(), path.end()),
		COMMAND::GETBINFILE,
		out,
		progress,
		errMsg);
}

MyAsioClient::ProtocolNegotiationResult MyAsioClient::negotiateProtocol(
	std::string* errMsg)
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	if (errMsg)
		errMsg->clear();

	auto closeTransport = [this]() {
		if (socket_) {
			boost::system::error_code ignored;
			socket_->close(ignored);
		}
	};

	try {
		if (!socket_ || !socket_->is_open()) {
			if (errMsg)
				*errMsg = "Нет подключения к серверу";
			return ProtocolNegotiationResult::Failed;
		}

		header request{};
		request.command = COMMAND::NEGOTIATE_PROTOCOL_V1;
		request.size = 0;
		boost::asio::write(*socket_, boost::asio::buffer(&request, sizeof(request)));

		header response{};
		if (!read_exact_idle_timeout(*socket_, &response, sizeof(response), errMsg)) {
			// Some legacy servers close immediately after rejecting an unknown
			// command. A single reconnect in legacy mode will verify that case.
			closeTransport();
			return ProtocolNegotiationResult::LegacyReconnectRequired;
		}

		if (response.command == COMMAND::SOMEERROR) {
			consumeServerErrorResponse(*socket_, response, errMsg);
			return ProtocolNegotiationResult::LegacyReconnectRequired;
		}

		if (response.command == COMMAND::SERVER_BUSY_ERROR ||
			response.command == COMMAND::ERROR_RESPONSE) {
			consumeServerErrorResponse(*socket_, response, errMsg);
			return ProtocolNegotiationResult::Failed;
		}

		if (response.command != COMMAND::NEGOTIATE_PROTOCOL_V1) {
			if (errMsg) {
				*errMsg = "Некорректный ответ negotiation: command=" +
					std::to_string(static_cast<std::uint64_t>(response.command));
			}
			closeTransport();
			return ProtocolNegotiationResult::Failed;
		}

		if (response.size != sizeof(search_protocol::ProtocolCapabilitiesV1)) {
			if (errMsg) {
				*errMsg = "Некорректный размер ProtocolCapabilitiesV1: " +
					std::to_string(static_cast<std::uint64_t>(response.size));
			}
			closeTransport();
			return ProtocolNegotiationResult::Failed;
		}

		search_protocol::ProtocolCapabilitiesV1 capabilities{};
		if (!read_exact_idle_timeout(
				*socket_, &capabilities, sizeof(capabilities), errMsg)) {
			closeTransport();
			return ProtocolNegotiationResult::Failed;
		}

		if (capabilities.version !=
			search_protocol::PROTOCOL_CAPABILITIES_VERSION) {
			if (errMsg) {
				*errMsg = "Неподдерживаемая версия ProtocolCapabilitiesV1: " +
					std::to_string(capabilities.version);
			}
			closeTransport();
			return ProtocolNegotiationResult::Failed;
		}

		if ((capabilities.capabilities &
			 search_protocol::CAPABILITY_TYPED_ERRORS_V1) != 0) {
			return ProtocolNegotiationResult::TypedErrorsV1;
		}

		return ProtocolNegotiationResult::LegacyOnCurrentConnection;
	}
	catch (const std::exception& e) {
		closeTransport();
		if (errMsg)
			*errMsg = e.what();
		return ProtocolNegotiationResult::Failed;
	}
	catch (...) {
		closeTransport();
		if (errMsg)
			*errMsg = "Неизвестная ошибка negotiation протокола";
		return ProtocolNegotiationResult::Failed;
	}
}

bool MyAsioClient::requestBinary(const std::vector<uint8_t>& request,
								 COMMAND command,
								 std::vector<uint8_t>& out,
								 std::atomic_uint64_t* progress,
								 std::string* errMsg,
								 search_protocol::ServerErrorInfo* serverError)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    out.clear();
	if (serverError)
		serverError->reset();
    try
	{
		if (!socket_ || !socket_->is_open()) {
			if (errMsg) *errMsg = "Нет подключения к серверу";
			return false;
		}

		// --- write request ---
        header req{};
		req.size = static_cast<uint_fast64_t>(request.size());
		req.command = command;

		boost::asio::write(*socket_, boost::asio::buffer(&req, sizeof(req)));
		if (!request.empty())
            boost::asio::write(*socket_, boost::asio::buffer(request.data(), request.size()));

        // --- read response header ---
		header resp{};
		if (!read_exact_idle_timeout(*socket_, &resp, sizeof(resp), errMsg)) {
			boost::system::error_code ignored;
			socket_->close(ignored);
			return false;
		}

		if (consumeServerErrorResponse(
				*socket_,
				resp,
				errMsg,
				"Сервер вернул ошибку",
				serverError)) {
			return false;
		}

		const std::uint64_t total64 = static_cast<std::uint64_t>(resp.size);
		if (total64 > kMaxInMemoryPayloadBytes ||
			total64 > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
			if (errMsg) {
				*errMsg =
					"Ответ сервера слишком большой для загрузки в память (" +
					std::to_string(total64) + " байт, лимит " +
					std::to_string(kMaxInMemoryPayloadBytes) + " байт)";
			}
			boost::system::error_code ignored;
			socket_->close(ignored);
			return false;
		}

		const std::size_t total = static_cast<std::size_t>(total64);
		last_total_.store(total64, std::memory_order_relaxed);
		if (progress) progress->store(0, std::memory_order_relaxed);

		// --- read payload ---
		out.resize(total);

        std::size_t off = 0;
        constexpr std::size_t block = 64 * 1024;

        while (off < total)
        {
            const std::size_t chunk = std::min(block, total - off);
            if (!read_exact_idle_timeout(*socket_, out.data() + off, chunk, errMsg)) {
				boost::system::error_code ignored;
				socket_->close(ignored);
                return false;
			}
            off += chunk;
            if (progress) progress->store(off, std::memory_order_relaxed);
        }

        return true;
    }
    catch (const std::exception& e)
    {
		if (socket_) {
			boost::system::error_code ignored;
			socket_->close(ignored);
		}
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

bool MyAsioClient::requestText(const std::string& request,
							   COMMAND command,
							   std::string& out,
							   std::atomic_uint64_t* progress,
							   std::string* errMsg,
							   search_protocol::ServerErrorInfo* serverError)
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	out.clear();
	if (serverError)
		serverError->reset();

	try {
		if (!socket_ || !socket_->is_open()) {
			if (errMsg) *errMsg = "Нет подключения к серверу";
			return false;
		}

		header req{};
		req.size = static_cast<uint_fast64_t>(request.size());
		req.command = command;
		boost::asio::write(*socket_, boost::asio::buffer(&req, sizeof(req)));
		if (!request.empty())
			boost::asio::write(*socket_, boost::asio::buffer(request.data(), request.size()));

		header resp{};
		if (!read_exact_idle_timeout(*socket_, &resp, sizeof(resp), errMsg)) {
			boost::system::error_code ignored;
			socket_->close(ignored);
			return false;
		}
		if (consumeServerErrorResponse(
				*socket_, resp, errMsg, "Сервер вернул ошибку", serverError)) {
			return false;
		}

		const std::uint64_t total64 = static_cast<std::uint64_t>(resp.size);
		if (total64 > kMaxInMemoryPayloadBytes ||
			total64 > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
			if (errMsg) {
				*errMsg =
					"Ответ сервера слишком большой для загрузки в память (" +
					std::to_string(total64) + " байт, лимит " +
					std::to_string(kMaxInMemoryPayloadBytes) + " байт)";
			}
			boost::system::error_code ignored;
			socket_->close(ignored);
			return false;
		}

		const std::size_t total = static_cast<std::size_t>(total64);
		last_total_.store(total64, std::memory_order_relaxed);
		if (progress) progress->store(0, std::memory_order_relaxed);
		out.reserve(total);

		std::size_t left = total;
		constexpr std::size_t block = 64 * 1024;
		std::vector<char> buffer(block);
		while (left > 0) {
			const std::size_t chunk = (std::min)(left, buffer.size());
			if (!read_exact_idle_timeout(*socket_, buffer.data(), chunk, errMsg)) {
				boost::system::error_code ignored;
				socket_->close(ignored);
				return false;
			}
			out.append(buffer.data(), chunk);
			left -= chunk;
			if (progress) progress->fetch_add(chunk, std::memory_order_relaxed);
		}

		return true;
	}
	catch (const std::exception& e) {
		if (socket_) {
			boost::system::error_code ignored;
			socket_->close(ignored);
		}
		if (errMsg) *errMsg = e.what();
		return false;
	}
}

bool MyAsioClient::requestFileToPath(const std::string& request,
									COMMAND command,
									const std::string& outPath,
									FileProgressCallback onProgress,
									std::uint64_t* receivedSize,
									std::string* errMsg,
									bool flushStreamToOs,
									bool replaceAfterSuccess)
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	namespace fs = std::filesystem;

	if (receivedSize)
		*receivedSize = 0;
	if (errMsg)
		errMsg->clear();

	const fs::path targetPath(outPath);

	{
		std::error_code existsEc;
		if (!replaceAfterSuccess &&
			fs::exists(targetPath, existsEc) && fs::is_regular_file(targetPath, existsEc)) {
			if (errMsg)
				*errMsg = "Файл уже существует; замена запрещена";
			return false;
		}
		if (fs::exists(targetPath, existsEc) && fs::is_directory(targetPath, existsEc)) {
			if (errMsg)
				*errMsg = "По конечному пути уже существует каталог";
			return false;
		}
	}

	auto invalidateTransport = [&]() {
		if (socket_) {
			boost::system::error_code ignored;
			socket_->shutdown(tcp::socket::shutdown_both, ignored);
			socket_->close(ignored);
		}
	};

	if (!socket_ || !socket_->is_open()) {
		if (errMsg) *errMsg = "Нет подключения к серверу";
		return false;
	}

	const fs::path parent = targetPath.parent_path();
	if (parent.empty()) {
		if (errMsg) *errMsg = "Не удалось определить каталог назначения";
		return false;
	}

	{
		std::error_code ec;
		fs::create_directories(parent, ec);
		if (ec && !client_file_publish::IsDirectoryW(parent)) {
			if (errMsg) *errMsg = "Не удалось создать каталог назначения";
			return false;
		}
	}

	std::wstring temporaryPath;
	client_file_publish::HandleGuard ownedTemp;
	std::wstring createError;
	if (!client_file_publish::CreateUniqueTemporary(
			parent, temporaryPath, ownedTemp.handle, &createError)) {
		if (errMsg)
			*errMsg = client_file_publish::Narrow(createError);
		return false;
	}

	// Ownership is proven by CREATE_NEW; close the exclusive handle and stream
	// into the same unique sibling. Only this path is deleted on failure.
	CloseHandle(ownedTemp.Release());
	client_file_publish::TemporaryFileGuard temporaryGuard;
	temporaryGuard.path = temporaryPath;

	const fs::path partPath(temporaryPath);
	std::ofstream file(partPath, std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		if (errMsg)
			*errMsg = "Не удалось открыть временный файл для записи: " +
				client_file_publish::Narrow(temporaryPath);
		return false;
	}

	auto failCleanup = [&]() {
		if (file.is_open())
			file.close();
	};

	try {
		header req{};
		req.size = static_cast<uint_fast64_t>(request.size());
		req.command = command;

		boost::asio::write(*socket_, boost::asio::buffer(&req, sizeof(req)));
		if (!request.empty())
			boost::asio::write(*socket_, boost::asio::buffer(request.data(), request.size()));

		header resp{};
		if (!read_exact_idle_timeout(*socket_, &resp, sizeof(resp), errMsg)) {
			invalidateTransport();
			failCleanup();
			return false;
		}

		if (consumeServerErrorResponse(
				*socket_, resp, errMsg, "Сервер не нашёл запрошенный файл")) {
			failCleanup();
			return false;
		}

		const std::uint64_t total = static_cast<std::uint64_t>(resp.size);
		last_total_.store(total, std::memory_order_relaxed);
		if (total == 0) {
			if (errMsg) *errMsg = "Сервер вернул пустой файл";
			failCleanup();
			return false;
		}

		if (onProgress)
			onProgress(0, total);

		constexpr std::size_t block = 64 * 1024;
		std::vector<std::uint8_t> buffer(block);
		std::uint64_t totalRead = 0;

		while (totalRead < total) {
			const std::size_t chunk = static_cast<std::size_t>(
				(std::min)(static_cast<std::uint64_t>(buffer.size()), total - totalRead));

			if (!read_exact_idle_timeout(*socket_, buffer.data(), chunk, errMsg)) {
				invalidateTransport();
				failCleanup();
				return false;
			}

			file.write(reinterpret_cast<const char*>(buffer.data()), chunk);
			if (!file) {
				if (errMsg) *errMsg = "Ошибка записи во временный файл";
				invalidateTransport(); // остаток ответа нельзя оставлять в потоке протокола
				failCleanup();
				return false;
			}

			totalRead += chunk;
			if (receivedSize)
				*receivedSize = totalRead;
			if (onProgress)
				onProgress(totalRead, total);
		}

		if (flushStreamToOs) {
			file.flush();
			if (!file) {
				if (errMsg) *errMsg = "Ошибка сброса буфера временного файла";
				failCleanup();
				return false;
			}
		}

		file.close();
		if (!file) {
			if (errMsg) *errMsg = "Ошибка закрытия временного файла";
			failCleanup();
			return false;
		}

		std::wstring publishError;
		if (replaceAfterSuccess) {
			// Target stays untouched until staging is complete; ReplaceFileW /
			// create-new publish lives in user_file_replace.
			temporaryGuard.Release();
			if (!user_file_replace::PublishExistingTemporary(
					temporaryPath, targetPath.wstring(), &publishError)) {
				if (errMsg)
					*errMsg = client_file_publish::Narrow(publishError);
				return false;
			}
			return true;
		}

		// Create-new publish: never delete/replace an existing final path.
		// If another process created the name after our early check, MoveNewFile
		// fails closed instead of removing a foreign file.
		if (!client_file_publish::MoveNewFile(partPath, targetPath, &publishError)) {
			if (errMsg)
				*errMsg = client_file_publish::Narrow(publishError);
			failCleanup();
			return false;
		}
		temporaryGuard.Release();
		return true;
	}
	catch (const std::exception& e) {
		invalidateTransport();
		failCleanup();
		if (errMsg) *errMsg = e.what();
		return false;
	}
	catch (...) {
		invalidateTransport();
		failCleanup();
		if (errMsg) *errMsg = "Неизвестная ошибка передачи файла";
		return false;
	}
}


// Îñíîâíàÿ âåðñèÿ (std::string)
bool MyAsioClient::tryConnect(const std::string& host,
                              const std::string& port,
                              std::string& errMsg)
{
	std::lock_guard<std::mutex> lk(io_mutex_);

	try {
		if (socket_) { delete socket_; socket_ = nullptr; }
		if (resolver_) { delete resolver_; resolver_ = nullptr; }

		io_context_.restart();
		socket_ = new tcp::socket(io_context_);
		resolver_ = new tcp::resolver(io_context_);

		// Разрешаем адрес
		boost::system::error_code resolve_ec;
		auto endpoints = resolver_->resolve(host, port, resolve_ec);
		if (resolve_ec) {
			errMsg = "Не удалось разрешить адрес сервера: " + resolve_ec.message();
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		// Проверяем, что есть хотя бы один endpoint
		if (endpoints.begin() == endpoints.end()) {
			errMsg = "Не найден адрес сервера";
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		// Подключение и таймер обслуживаются в одном потоке. Перед выходом
		// обязательно завершаем оба callback, чтобы в io_context не оставались
		// обработчики со ссылками на уже уничтоженные локальные переменные.
		boost::asio::steady_timer timer(io_context_);
		timer.expires_after(std::chrono::seconds(2));

		boost::system::error_code connectEc = boost::asio::error::would_block;
		bool connectDone = false;
		bool timerDone = false;
		bool timedOut = false;

		boost::asio::async_connect(*socket_, endpoints,
			[&](const boost::system::error_code& ec, const tcp::endpoint&) {
				connectEc = ec;
				connectDone = true;
				boost::system::error_code ignored;
				timer.cancel(ignored);
			});

		timer.async_wait([&](const boost::system::error_code& ec) {
			timerDone = true;
			if (!ec && !connectDone) {
				timedOut = true;
				boost::system::error_code ignored;
				socket_->cancel(ignored);
			}
		});

		while (!connectDone || !timerDone) {
			if (io_context_.run_one() == 0) {
				errMsg = "Операция подключения прервалась";
				if (socket_) { delete socket_; socket_ = nullptr; }
				if (resolver_) { delete resolver_; resolver_ = nullptr; }
				return false;
			}
		}

		if (timedOut) {
			errMsg = "Таймаут подключения к серверу";
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		if (connectEc) {
			errMsg = "Не удалось подключиться к серверу: " + connectEc.message();
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		// Подключение успешно.
        return true;
    }
    catch (const std::exception& e) {
        if (socket_) { delete socket_; socket_ = nullptr; }
        if (resolver_) { delete resolver_; resolver_ = nullptr; }
        errMsg = "Ошибка подключения: " + std::string(e.what());
        return false;
    }
    catch (...) {
        if (socket_) { delete socket_; socket_ = nullptr; }
        if (resolver_) { delete resolver_; resolver_ = nullptr; }
        errMsg = "Неизвестная ошибка подключения";
        return false;
    }
}

// Ïåðåãðóçêà äëÿ AnsiString
bool MyAsioClient::tryConnect(const AnsiString& host,
                              const AnsiString& port,
                              AnsiString& errMsg)
{
    std::string err;
    bool ok = tryConnect(std::string(host.c_str()),
                         std::string(port.c_str()),
                         err);
    if (!ok)
        errMsg = err.c_str();
    return ok;
}




bool MyAsioClient::socketWrite(const std::string& s,
                               COMMAND command,
                               std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    if (!socket_ || !socket_->is_open()) {
        if (errMsg) *errMsg = "Нет подключения к серверу";
        return false;
    }

    myHeader_.size    = static_cast<uint32_t>(s.size());
    myHeader_.command = command;

    try {
        boost::asio::write(*socket_, boost::asio::buffer(&myHeader_, sizeof(myHeader_)));
        if (!s.empty())
            boost::asio::write(*socket_, boost::asio::buffer(s.data(), s.size()));
        return true;
    } catch (const std::exception& e) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

bool MyAsioClient::socketWrite(const std::vector<uint8_t>& data,
                               COMMAND command,
                               std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    if (!socket_ || !socket_->is_open()) {
        if (errMsg) *errMsg = "Нет подключения к серверу";
        return false;
    }

    myHeader_.size    = static_cast<uint32_t>(data.size());
    myHeader_.command = command;

    try {
        boost::asio::write(*socket_, boost::asio::buffer(&myHeader_, sizeof(myHeader_)));
        if (!data.empty())
            boost::asio::write(*socket_, boost::asio::buffer(data.data(), data.size()));
        return true;
    } catch (const std::exception& e) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

bool MyAsioClient::socketRead(std::string& out,
                              std::atomic_uint64_t* progress,
                              std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    if (!socket_ || !socket_->is_open()) {
        if (errMsg) *errMsg = "Нет подключения к серверу";
        return false;
    }

    out.clear();
    try {
        // 1) читаем заголовок ровно sizeof(header)
        if (!read_exact_idle_timeout(*socket_, &myHeader_, sizeof(myHeader_), errMsg)) {
			boost::system::error_code ignored;
			socket_->close(ignored);
            return false;
		}
		if (consumeServerErrorResponse(*socket_, myHeader_, errMsg)) {
            return false;
        }

        const std::size_t total = myHeader_.size;
        last_total_.store(total, std::memory_order_relaxed);
        if (progress) progress->store(0, std::memory_order_relaxed);

        out.reserve(out.size() + total);

        // 2) ÷èòàåì òåëî ôèêñèðîâàííûìè áëîêàìè
        std::size_t left = total;
        constexpr std::size_t block = 64 * 1024;
        std::vector<char> buf;

        while (left > 0) {
            const std::size_t chunk = std::min(block, left);
            buf.resize(chunk);

            if (!read_exact_idle_timeout(*socket_, buf.data(), chunk, errMsg)) {
				boost::system::error_code ignored;
				socket_->close(ignored);
                return false;
			}

            out.append(buf.data(), buf.size());
            left -= chunk;

            if (progress) progress->fetch_add(buf.size(), std::memory_order_relaxed);
        }

        return true;
    } catch (const std::exception& e) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

bool MyAsioClient::socketReadBin(std::vector<uint8_t>& out,
                                 std::atomic_uint64_t* progress,
                                 std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    if (!socket_ || !socket_->is_open()) {
        if (errMsg) *errMsg = "Нет подключения к серверу";
        return false;
    }

    out.clear();
    try {
        // 1) читаем заголовок
        if (!read_exact_idle_timeout(*socket_, &myHeader_, sizeof(myHeader_), errMsg)) {
			boost::system::error_code ignored;
			socket_->close(ignored);
            return false;
		}
		if (consumeServerErrorResponse(*socket_, myHeader_, errMsg)) {
            return false;
        }

        const std::size_t total = myHeader_.size;
        last_total_.store(total, std::memory_order_relaxed);
        if (progress) progress->store(0, std::memory_order_relaxed);

        out.reserve(total);

        std::size_t left = total;
        constexpr std::size_t block = 64 * 1024;
        std::vector<uint8_t> buf;

        while (left > 0) {
            const std::size_t chunk = std::min(block, left);
            buf.resize(chunk);

            if (!read_exact_idle_timeout(*socket_, buf.data(), chunk, errMsg)) {
				boost::system::error_code ignored;
				socket_->close(ignored);
                return false;
			}

            out.insert(out.end(), buf.begin(), buf.end());
            left -= chunk;

            if (progress) progress->fetch_add(buf.size(), std::memory_order_relaxed);
        }

        return true;
    } catch (const std::exception& e) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

MyAsioClient::~MyAsioClient() noexcept
{
    try {
        disConnect();          // àêêóðàòíî çàêðîåì ñîêåò/ðåçîëâåð è îñâîáîäèì ïàìÿòü
    } catch (...) {
        // íè÷åãî íå äåëàåì — äåñòðóêòîð íå äîëæåí áðîñàòü èñêëþ÷åíèÿ
    }
}

bool MyAsioClient::readHeader(header& hdr, std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);
    
    if (!socket_ || !socket_->is_open()) {
        if (errMsg) *errMsg = "Нет подключения к серверу";
        return false;
    }
    
    if (!read_exact_idle_timeout(*socket_, &hdr, sizeof(hdr), errMsg)) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        return false;
	}
    return true;
}

