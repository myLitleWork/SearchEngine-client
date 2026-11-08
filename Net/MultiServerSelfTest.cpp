#include <vcl.h>
#pragma hdrstop

#include "MultiServerSelfTest.h"

#ifdef _DEBUG

#include "MultiServerManager.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
	using boost::asio::ip::tcp;

	class FakeSearchServer
	{
	public:
		explicit FakeSearchServer(
			std::string response,
			COMMAND responseCommand = COMMAND::GETSQLJSONANSWEAR,
			bool supportsNegotiation = true,
			bool anonymous = false)
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, response_(std::move(response))
			, responseCommand_(responseCommand)
			, supportsNegotiation_(supportsNegotiation)
			, anonymous_(anonymous)
			, worker_([this]() { run(); })
		{
		}

		~FakeSearchServer()
		{
			boost::system::error_code ignored;
			acceptor_.close(ignored);
			if (socket_)
				socket_->close(ignored);
			if (worker_.joinable())
				worker_.join();
		}

		unsigned short port() const { return acceptor_.local_endpoint().port(); }
		bool registered() const { return registered_.load(std::memory_order_acquire); }
		bool searched() const { return searched_.load(std::memory_order_acquire); }
		bool failed() const { return failed_.load(std::memory_order_acquire); }
		bool negotiated() const { return negotiated_.load(std::memory_order_acquire); }
		bool legacyFallbackUsed() const {
			return legacyFallbackUsed_.load(std::memory_order_acquire);
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

		void readRequest(tcp::socket& socket, header& request, std::string& body)
		{
			boost::asio::read(socket, boost::asio::buffer(&request, sizeof(request)));
			body.assign(static_cast<std::size_t>(request.size), '\0');
			if (!body.empty())
				boost::asio::read(socket, boost::asio::buffer(&body[0], body.size()));
		}

		void serveApplicationRequests(tcp::socket& socket)
		{
			for (int requestIndex = 0; requestIndex < (anonymous_ ? 1 : 2); ++requestIndex) {
				header request{};
				std::string body;
				readRequest(socket, request, body);

				if (request.command == COMMAND::USER_REGISTRY) {
					if (anonymous_) failed_.store(true, std::memory_order_release);
					registered_.store(
						body == "self-test-user", std::memory_order_release);
					send(socket, request.command, "OK");
				}
				else {
					searched_.store(
						request.command == COMMAND::GETSQLJSONANSWEAR && body == "needle",
						std::memory_order_release);
					send(socket, responseCommand_, response_);
				}
			}
		}

		void acceptConnection()
		{
			socket_ = std::make_unique<tcp::socket>(io_);
			acceptor_.accept(*socket_);
		}

		void run()
		{
			try {
				acceptConnection();
				header negotiationRequest{};
				std::string negotiationBody;
				readRequest(*socket_, negotiationRequest, negotiationBody);

				if (negotiationRequest.command != COMMAND::NEGOTIATE_PROTOCOL_V1 ||
					!negotiationBody.empty()) {
					failed_.store(true, std::memory_order_release);
					return;
				}

				if (supportsNegotiation_) {
					search_protocol::ProtocolCapabilitiesV1 capabilities{};
					capabilities.capabilities =
						search_protocol::CAPABILITY_TYPED_ERRORS_V1 |
						(anonymous_ ? search_protocol::CAPABILITY_ANONYMOUS_ACCESS_V1 : 0u);
					send(
						*socket_,
						COMMAND::NEGOTIATE_PROTOCOL_V1,
						std::string(
							reinterpret_cast<const char*>(&capabilities),
							sizeof(capabilities)));
					negotiated_.store(true, std::memory_order_release);
				}
				else {
					send(*socket_, COMMAND::SOMEERROR, {});
					boost::system::error_code ignored;
					socket_->shutdown(tcp::socket::shutdown_both, ignored);
					socket_->close(ignored);
					legacyFallbackUsed_.store(true, std::memory_order_release);
					acceptConnection();
				}

				serveApplicationRequests(*socket_);
			}
			catch (...) {
				failed_.store(true, std::memory_order_release);
			}
		}

		boost::asio::io_context io_;
		tcp::acceptor acceptor_;
		std::unique_ptr<tcp::socket> socket_;
		std::string response_;
		COMMAND responseCommand_;
		bool supportsNegotiation_;
		bool anonymous_;
		std::thread worker_;
		std::atomic_bool registered_{false};
		std::atomic_bool searched_{false};
		std::atomic_bool failed_{false};
		std::atomic_bool negotiated_{false};
		std::atomic_bool legacyFallbackUsed_{false};
	};

	// Negotiates, then rejects AUTHENTICATE_V1 with a typed ErrorResponseV1.
	// Any USER_REGISTRY / search request marks the fake as failed (no fallback).
	class FakeAuthRejectServer
	{
	public:
		explicit FakeAuthRejectServer(search_protocol::ErrorCode code)
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, code_(code)
			, worker_([this]() { run(); })
		{
		}

		~FakeAuthRejectServer()
		{
			boost::system::error_code ignored;
			acceptor_.close(ignored);
			if (socket_)
				socket_->close(ignored);
			if (worker_.joinable())
				worker_.join();
		}

		unsigned short port() const { return acceptor_.local_endpoint().port(); }
		bool authenticated() const { return authenticated_.load(std::memory_order_acquire); }
		bool userRegistrySeen() const {
			return userRegistrySeen_.load(std::memory_order_acquire);
		}
		bool searched() const { return searched_.load(std::memory_order_acquire); }
		bool failed() const { return failed_.load(std::memory_order_acquire); }
		std::string lastAuthBody() const
		{
			std::lock_guard<std::mutex> lock(bodyMutex_);
			return authBody_;
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

		void readRequest(tcp::socket& socket, header& request, std::string& body)
		{
			boost::asio::read(socket, boost::asio::buffer(&request, sizeof(request)));
			body.assign(static_cast<std::size_t>(request.size), '\0');
			if (!body.empty())
				boost::asio::read(socket, boost::asio::buffer(&body[0], body.size()));
		}

		void run()
		{
			try {
				socket_ = std::make_unique<tcp::socket>(io_);
				acceptor_.accept(*socket_);

				header negotiationRequest{};
				std::string negotiationBody;
				readRequest(*socket_, negotiationRequest, negotiationBody);
				if (negotiationRequest.command != COMMAND::NEGOTIATE_PROTOCOL_V1) {
					failed_.store(true, std::memory_order_release);
					return;
				}
				search_protocol::ProtocolCapabilitiesV1 capabilities{};
				capabilities.capabilities =
					search_protocol::CAPABILITY_TYPED_ERRORS_V1 |
					search_protocol::CAPABILITY_CLIENT_AUTH_V1;
				send(
					*socket_,
					COMMAND::NEGOTIATE_PROTOCOL_V1,
					std::string(
						reinterpret_cast<const char*>(&capabilities),
						sizeof(capabilities)));

				header authRequest{};
				std::string authBody;
				readRequest(*socket_, authRequest, authBody);
				authenticated_.store(
					authRequest.command == COMMAND::AUTHENTICATE_V1,
					std::memory_order_release);
				if (authRequest.command != COMMAND::AUTHENTICATE_V1) {
					failed_.store(true, std::memory_order_release);
					return;
				}
				{
					std::lock_guard<std::mutex> lock(bodyMutex_);
					authBody_ = authBody;
				}

				search_protocol::ErrorResponseV1 errorPayload{};
				errorPayload.errorCode = static_cast<std::uint32_t>(code_);
				send(
					*socket_,
					COMMAND::ERROR_RESPONSE,
					std::string(
						reinterpret_cast<const char*>(&errorPayload),
						sizeof(errorPayload)));
				boost::system::error_code ignored;
				socket_->shutdown(tcp::socket::shutdown_both, ignored);
				socket_->close(ignored);
				acceptor_.close(ignored);
			}
			catch (...) {
				failed_.store(true, std::memory_order_release);
			}
		}

		boost::asio::io_context io_;
		tcp::acceptor acceptor_;
		std::unique_ptr<tcp::socket> socket_;
		search_protocol::ErrorCode code_;
		std::thread worker_;
		mutable std::mutex bodyMutex_;
		std::string authBody_;
		std::atomic_bool authenticated_{false};
		std::atomic_bool userRegistrySeen_{false};
		std::atomic_bool searched_{false};
		std::atomic_bool failed_{false};
	};

	bool expectAuthMessage(search_protocol::ErrorCode code, const char* needleUtf8)
	{
		const auto message = serverErrorUserMessage(static_cast<std::uint32_t>(code));
		if (message.find(needleUtf8) == std::string::npos)
			return false;
		if (message.find(std::to_string(static_cast<std::uint32_t>(code))) ==
			std::string::npos)
			return false;
		if (serverErrorCodeName(static_cast<std::uint32_t>(code)).empty())
			return false;
		return isAuthRejectionErrorCode(static_cast<std::uint32_t>(code));
	}
}

bool IsMultiServerSelfTestInvocation()
{
	return ParamCount() >= 1 && SameText(ParamStr(1), L"--multi-server-self-test");
}

int RunMultiServerSelfTestFromCommandLine()
{
	FakeSearchServer first("first-response");
	FakeSearchServer second("second-response");
	search_protocol::ErrorResponseV1 errorPayload{};
	errorPayload.errorCode = static_cast<std::uint32_t>(
		search_protocol::ErrorCode::OperatorNotRegistered);
	FakeSearchServer typedError(
		std::string(reinterpret_cast<const char*>(&errorPayload), sizeof(errorPayload)),
		COMMAND::ERROR_RESPONSE);
	FakeSearchServer legacy(
		"legacy-response",
		COMMAND::GETSQLJSONANSWEAR,
		false);

	MultiServerManager manager;
	// IPv4-only listeners must also work through localhost, including the
	// second connection used by legacy protocol fallback. Keep numeric IPv4
	// endpoints in the same test to cover both forms of server configuration.
	manager.configure({
		{"first", "First", "localhost", std::to_string(first.port()), true},
		{"second", "Second", "127.0.0.1", std::to_string(second.port()), true},
		{"typed-error", "Typed error", "127.0.0.1", std::to_string(typedError.port()), true},
		{"legacy", "Legacy", "localhost", std::to_string(legacy.port()), true}
	}, "first");
	manager.setUser("self-test-user");

	std::string firstResponse;
	std::string secondResponse;
	std::string firstError;
	std::string secondError;
	std::vector<uint8_t> typedErrorResponse;
	std::string typedErrorMessage;
	search_protocol::ServerErrorInfo typedErrorInfo;
	std::string legacyResponse;
	std::string legacyError;
	bool firstOk = false;
	bool secondOk = false;

	std::thread firstRequest([&]() {
		firstOk = manager.requestText(
			"first", "needle", COMMAND::GETSQLJSONANSWEAR,
			firstResponse, nullptr, &firstError);
	});
	std::thread secondRequest([&]() {
		secondOk = manager.requestText(
			"second", "needle", COMMAND::GETSQLJSONANSWEAR,
			secondResponse, nullptr, &secondError);
	});
	firstRequest.join();
	secondRequest.join();
	const bool typedErrorOk = manager.requestBinary(
		"typed-error",
		std::vector<uint8_t>{'n', 'e', 'e', 'd', 'l', 'e'},
		COMMAND::GETSQLJSONANSWEAR,
		typedErrorResponse,
		nullptr,
		&typedErrorMessage,
		&typedErrorInfo);
	const bool legacyOk = manager.requestText(
		"legacy", "needle", COMMAND::GETSQLJSONANSWEAR,
		legacyResponse, nullptr, &legacyError);
	manager.disconnectAll();

	if (!firstOk || !secondOk)
		return 41;
	if (firstResponse != "first-response" || secondResponse != "second-response")
		return 42;
	if (!first.registered() || !second.registered())
		return 43;
	if (!first.searched() || !second.searched())
		return 44;
	if (first.failed() || second.failed())
		return 45;
	if (typedErrorOk || !typedErrorResponse.empty())
		return 46;
	static const char kExpectedTypedError[] =
		"\xD0\x9E\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB0\xD1\x82\xD0\xBE\xD1\x80\x20"
		"\xD0\xBD\xD0\xB5\x20\xD0\xB7\xD0\xB0\xD1\x80\xD0\xB5\xD0\xB3\xD0\xB8\xD1\x81\xD1\x82"
		"\xD1\x80\xD0\xB8\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD0\xBD\x20"
		"\xD0\xBD\xD0\xB0\x20\xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB5\x2E\x20"
		"\xD0\x9A\xD0\xBE\xD0\xB4\x20\xD0\xBE\xD1\x88\xD0\xB8\xD0\xB1\xD0\xBA\xD0\xB8\x3A\x20"
		"25.";
	if (typedErrorMessage != kExpectedTypedError)
		return 47;
	if (!typedErrorInfo.hasCode || !typedErrorInfo.isTyped ||
		!typedErrorInfo.is(search_protocol::ErrorCode::OperatorNotRegistered))
		return 53;
	if (!typedError.registered() || !typedError.searched() || typedError.failed())
		return 48;
	if (!first.negotiated() || !second.negotiated() || !typedError.negotiated())
		return 49;
	if (!legacyOk || legacyResponse != "legacy-response" || !legacyError.empty())
		return 50;
	if (!legacy.registered() || !legacy.searched() || legacy.failed())
		return 51;
	if (legacy.negotiated() || !legacy.legacyFallbackUsed())
		return 52;

	// --- AUTHENTICATE_V1 wire ErrorCode mapping (server table 33..45) ---
	if (static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthFailed) != 33u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientDisabled) != 34u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthRequired) != 35u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientIdMissing) != 36u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientNameMissing) != 37u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceTypeMissing) != 38u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceIdMissing) != 39u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthSignatureMissing) != 40u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientIdNotFound) != 41u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientNameMismatch) != 42u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceTypeMismatch) != 43u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthDeviceIdMismatch) != 44u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthSignatureInvalid) != 45u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceDisabled) != 46u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceUnavailable) != 47u ||
		static_cast<std::uint32_t>(search_protocol::ErrorCode::ArchiveAdminRequired) != 48u)
		return 60;

	// UTF-8 substrings from config/server-error-messages.json (stable wire texts).
	if (!expectAuthMessage(search_protocol::ErrorCode::AuthFailed,
			"\xD0\xBD\xD0\xB5\xD0\xBE\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xBB") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthClientDisabled,
			"\xD0\xBE\xD1\x82\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthRequired,
			"\xD1\x82\xD1\x80\xD0\xB5\xD0\xB1\xD1\x83\xD0\xB5\xD1\x82") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthClientIdMissing, "client_id") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthClientNameMissing, "client_name") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthDeviceTypeMissing, "device_type") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthDeviceIdMissing, "device_id") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthSignatureMissing,
			"\xD0\xBF\xD0\xBE\xD0\xB4\xD0\xBF\xD0\xB8\xD1\x81") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthClientIdNotFound,
			"\xD0\xBD\xD0\xB5\x20\xD0\xB7\xD0\xB0\xD1\x80\xD0\xB5\xD0\xB3\xD0\xB8\xD1\x81\xD1\x82") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthClientNameMismatch,
			"\xD0\xB8\xD0\xBC\xD1\x8F") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthDeviceTypeMismatch,
			"\xD1\x82\xD0\xB8\xD0\xBF") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthDeviceIdMismatch,
			"device_id") ||
		!expectAuthMessage(search_protocol::ErrorCode::AuthSignatureInvalid,
			"\xD0\xBD\xD0\xB5\xD0\xB4\xD0\xB5\xD0\xB9\xD1\x81\xD1\x82\xD0\xB2"))
		return 61;

	const auto unknownMsg = serverErrorUserMessage(999u);
	if (unknownMsg.find("999") == std::string::npos)
		return 62;
	if (serverErrorCodeName(999u) != "UnknownErrorCode")
		return 63;
	if (isAuthRejectionErrorCode(999u))
		return 64;

	const auto dsDisabledMsg = serverErrorUserMessage(
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceDisabled));
	if (dsDisabledMsg.find("46") == std::string::npos ||
		dsDisabledMsg.find("AutoPad") == std::string::npos ||
		serverErrorCodeName(static_cast<std::uint32_t>(
			search_protocol::ErrorCode::DataSourceDisabled)) != "DataSourceDisabled")
		return 76;

	const auto dsUnavailableMsg = serverErrorUserMessage(
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DataSourceUnavailable));
	if (dsUnavailableMsg.find("47") == std::string::npos ||
		dsUnavailableMsg.find("AutoPad") == std::string::npos ||
		serverErrorCodeName(static_cast<std::uint32_t>(
			search_protocol::ErrorCode::DataSourceUnavailable)) != "DataSourceUnavailable")
		return 77;

	const auto archiveAdminMsg = serverErrorUserMessage(
		static_cast<std::uint32_t>(search_protocol::ErrorCode::ArchiveAdminRequired));
	if (archiveAdminMsg.find("48") == std::string::npos ||
		serverErrorCodeName(static_cast<std::uint32_t>(
			search_protocol::ErrorCode::ArchiveAdminRequired)) != "ArchiveAdminRequired")
		return 78;

	if (!serverErrorCatalogLoaded())
		return 81;
	const auto catalog16 = serverErrorCatalogMessage(
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DatabaseOpenFailed));
	if (catalog16.empty() || catalog16.find("16") != std::string::npos)
		return 82;
	const auto user16 = serverErrorUserMessage(
		static_cast<std::uint32_t>(search_protocol::ErrorCode::DatabaseOpenFailed));
	if (user16.find(catalog16) != 0)
		return 83;
	std::uint32_t extracted16 = 0;
	if (!tryExtractServerErrorCode(user16, extracted16) || extracted16 != 16u)
		return 84;
	if (serverErrorCodeName(16u) != "DatabaseOpenFailed")
		return 85;

	// Anonymous sessions need no local credentials and send no USER_REGISTRY.
	FakeSearchServer openServer("[]", COMMAND::GETSQLJSONANSWEAR, true, true);
	MultiServerManager openManager;
	openManager.configure({
		{"open", "Open", "127.0.0.1", std::to_string(openServer.port()), true}
	}, "open");
	openManager.setAuthPolicy(ServerAuthPolicy::AnonymousOnly);
	std::string openError, openReply;
	if (!openManager.connectPrimary(&openError) ||
		!openManager.isServerAnonymous("open") ||
		!openManager.isServerAuthorized("open")) return 90;
	if (!openManager.requestText("open", "needle", COMMAND::GETSQLJSONANSWEAR,
		openReply, nullptr, &openError) || openReply != "[]") return 91;
	if (openServer.registered() || !openServer.searched() || openServer.failed()) return 92;
	openManager.disconnectAll();
	if (openManager.isServerAnonymous("open") || openManager.hasAnyServerAuthorized()) return 93;

	FakeAuthRejectServer authReject(search_protocol::ErrorCode::AuthClientIdNotFound);
	FakeAuthRejectServer authSig(search_protocol::ErrorCode::AuthSignatureInvalid);
	MultiServerManager authManager;
	authManager.configure({
		{"auth-a", "Auth A", "127.0.0.1", std::to_string(authReject.port()), true},
		{"auth-b", "Auth B", "127.0.0.1", std::to_string(authSig.port()), true}
	}, "auth-a");
	authManager.setAuthPolicy(ServerAuthPolicy::AuthenticateV1);

	const auto authAttempts = authManager.authenticateAllV1(
		"desk", "missing-client", "usb", "USBSERIAL01", "sig");
	if (authAttempts.size() != 2)
		return 65;
	if (authAttempts[0].ok || authAttempts[1].ok)
		return 66;
	if (!authAttempts[0].hasWireErrorCode ||
		authAttempts[0].wireErrorCode !=
			static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthClientIdNotFound))
		return 67;
	if (!authAttempts[1].hasWireErrorCode ||
		authAttempts[1].wireErrorCode !=
			static_cast<std::uint32_t>(search_protocol::ErrorCode::AuthSignatureInvalid))
		return 68;
	if (authAttempts[0].error.find("41") == std::string::npos ||
		authAttempts[1].error.find("45") == std::string::npos)
		return 69;
	if (authManager.isServerAuthorized("auth-a") ||
		authManager.isServerAuthorized("auth-b"))
		return 70;
	if (authManager.isConnected("auth-a") || authManager.isConnected("auth-b"))
		return 71;
	if (!authManager.authorizedServerIds().empty())
		return 79;
	if (unauthorizedSearchSkipMessage().find("35") != std::string::npos)
		return 80;

	std::string searchOut;
	std::string searchErr;
	if (authManager.requestText(
			"auth-a", "needle", COMMAND::GETSQLJSONANSWEAR, searchOut, nullptr, &searchErr))
		return 72;

	// Give the fake a moment; client must not reconnect for USER_REGISTRY/search.
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	if (!authReject.authenticated() || !authSig.authenticated())
		return 73;
	if (authReject.failed() || authSig.failed())
		return 74;

	try {
		const auto parsed = nlohmann::json::parse(authReject.lastAuthBody());
		if (parsed.contains("flash_serial") ||
			parsed.value("device_type", std::string()) != "usb" ||
			parsed.value("device_id", std::string()) != "USBSERIAL01")
			return 75;
	} catch (...) {
		return 75;
	}

	authManager.disconnectAll();

	// A selected device token is used only for servers that require it.
	FakeSearchServer mixedOpen("[]", COMMAND::GETSQLJSONANSWEAR, true, true);
	FakeAuthRejectServer mixedRequired(search_protocol::ErrorCode::AuthSignatureInvalid);
	MultiServerManager mixedManager;
	mixedManager.configure({
		{"open", "Open", "127.0.0.1", std::to_string(mixedOpen.port()), true},
		{"required", "Required", "127.0.0.1", std::to_string(mixedRequired.port()), true}
	}, "open");
	mixedManager.setAuthPolicy(ServerAuthPolicy::AuthenticateV1);
	const auto mixedAttempts = mixedManager.authenticateAllV1(
		"desk", "client", "usb", "SERIAL", "sig");
	if (mixedAttempts.size() != 2 || !mixedAttempts[0].ok || mixedAttempts[1].ok ||
		mixedManager.anonymousServerCount() != 1 || mixedManager.authorizedServerCount() != 1)
		return 94;
    SearchRequestControl cancelledSearch;
    cancelledSearch.cancelled.store(true);
    if (mixedManager.requestText("open", "needle", COMMAND::GETSQLJSONANSWEAR,
        openReply, nullptr, &openError, &cancelledSearch) || mixedOpen.searched()) return 99;
	if (!mixedManager.requestText("open", "needle", COMMAND::GETSQLJSONANSWEAR,
		openReply, nullptr, &openError)) return 95;
	if (mixedOpen.registered() || !mixedOpen.searched() || mixedOpen.failed() ||
		mixedRequired.userRegistrySeen() || mixedRequired.searched() || mixedRequired.failed())
		return 96;
	mixedManager.disconnectAll();

	// With no credentials, an old/required server must never get USER_REGISTRY.
	FakeSearchServer requiresToken("[]");
	MultiServerManager guestManager;
	guestManager.configure({
		{"required", "Required", "127.0.0.1", std::to_string(requiresToken.port()), true}
	}, "required");
	guestManager.setAuthPolicy(ServerAuthPolicy::AnonymousOnly);
	if (guestManager.connectPrimary(&openError) ||
		guestManager.hasAnyServerAuthorized() || guestManager.isConnected("required"))
		return 97;
	if (requiresToken.registered() || requiresToken.searched()) return 98;
	guestManager.disconnectAll();
	return 0;
}

#endif
