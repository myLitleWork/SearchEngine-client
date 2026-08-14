#include <vcl.h>
#pragma hdrstop

#include "MultiServerSelfTest.h"

#ifdef _DEBUG

#include "MultiServerManager.h"

#include <atomic>
#include <memory>
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
			COMMAND responseCommand = COMMAND::GETSQLJSONANSWEAR)
			: acceptor_(io_, tcp::endpoint(tcp::v4(), 0))
			, response_(std::move(response))
			, responseCommand_(responseCommand)
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

		void run()
		{
			try {
				socket_ = std::make_unique<tcp::socket>(io_);
				acceptor_.accept(*socket_);
				for (int requestIndex = 0; requestIndex < 2; ++requestIndex) {
					header request{};
					boost::asio::read(*socket_, boost::asio::buffer(&request, sizeof(request)));
					std::string body(static_cast<std::size_t>(request.size), '\0');
					if (!body.empty())
						boost::asio::read(*socket_, boost::asio::buffer(&body[0], body.size()));

					if (request.command == COMMAND::USER_REGISTRY) {
						registered_.store(body == "self-test-user", std::memory_order_release);
						send(*socket_, request.command, "OK");
					}
					else {
						searched_.store(
							request.command == COMMAND::GETSQLJSONANSWEAR && body == "needle",
							std::memory_order_release);
						send(*socket_, responseCommand_, response_);
					}
				}
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
		std::thread worker_;
		std::atomic_bool registered_{false};
		std::atomic_bool searched_{false};
		std::atomic_bool failed_{false};
	};
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
		search_protocol::ErrorCode::FileNotFound);
	FakeSearchServer typedError(
		std::string(reinterpret_cast<const char*>(&errorPayload), sizeof(errorPayload)),
		COMMAND::ERROR_RESPONSE);

	MultiServerManager manager;
	manager.configure({
		{"first", "First", "127.0.0.1", std::to_string(first.port()), true},
		{"second", "Second", "127.0.0.1", std::to_string(second.port()), true},
		{"typed-error", "Typed error", "127.0.0.1", std::to_string(typedError.port()), true}
	}, "first");
	manager.setUser("self-test-user");

	std::string firstResponse;
	std::string secondResponse;
	std::string firstError;
	std::string secondError;
	std::string typedErrorResponse;
	std::string typedErrorMessage;
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
	const bool typedErrorOk = manager.requestText(
		"typed-error", "needle", COMMAND::GETSQLJSONANSWEAR,
		typedErrorResponse, nullptr, &typedErrorMessage);
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
	if (typedErrorMessage != "Ошибка сервера: FileNotFound (код 9)")
		return 47;
	if (!typedError.registered() || !typedError.searched() || typedError.failed())
		return 48;
	return 0;
}

#endif
