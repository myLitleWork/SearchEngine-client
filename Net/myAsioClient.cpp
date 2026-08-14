//---------------------------------------------------------------------------

#pragma hdrstop

#include "myAsioClient.h"
#include <algorithm>
#include <boost/asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

// Единый idle-таймаут для чтения с сокета (как SocketFileSaver): без новых байт N секунд — ошибка.
constexpr auto kSocketReadIdleTimeout = std::chrono::seconds(10);

const char* serverErrorCodeName(std::uint32_t rawCode) noexcept
{
	using search_protocol::ErrorCode;
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
	}
	return "UnknownErrorCode";
}

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
								const char* legacySomeErrorMessage = "Server returned SOMEERROR")
{
	auto closeTransport = [&socket]() {
		boost::system::error_code ignored;
		socket.close(ignored);
	};

	if (response.command == COMMAND::SOMEERROR) {
		if (errMsg)
			*errMsg = legacySomeErrorMessage;
		closeTransport();
		return true;
	}

	if (response.command == COMMAND::SERVER_BUSY_ERROR) {
		if (errMsg)
			*errMsg = "SERVER_BUSY_ERROR";
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

	if (payload.errorCode ==
		static_cast<std::uint32_t>(search_protocol::ErrorCode::ServerBusy)) {
		if (errMsg)
			*errMsg = "SERVER_BUSY_ERROR";
		closeTransport();
		return true;
	}

	if (errMsg) {
		*errMsg = "Ошибка сервера: " +
			std::string(serverErrorCodeName(payload.errorCode)) +
			" (код " + std::to_string(payload.errorCode) + ")";
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

bool MyAsioClient::requestBinary(const std::vector<uint8_t>& request,
								 COMMAND command,
								 std::vector<uint8_t>& out,
								 std::atomic_uint64_t* progress,
								 std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    out.clear();
    try
	{
		if (!socket_ || !socket_->is_open()) {
			if (errMsg) *errMsg = "Socket is not connected";
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

		if (consumeServerErrorResponse(*socket_, resp, errMsg)) {
			return false;
		}

        const std::size_t total = resp.size;
        last_total_.store(total, std::memory_order_relaxed);
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
							   std::string* errMsg)
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	out.clear();

	try {
		if (!socket_ || !socket_->is_open()) {
			if (errMsg) *errMsg = "Socket is not connected";
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
		if (consumeServerErrorResponse(*socket_, resp, errMsg)) {
			return false;
		}

		const std::size_t total = static_cast<std::size_t>(resp.size);
		last_total_.store(total, std::memory_order_relaxed);
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
									bool flushStreamToOs)
{
	std::lock_guard<std::mutex> lk(io_mutex_);
	namespace fs = std::filesystem;

	if (receivedSize)
		*receivedSize = 0;
	if (errMsg)
		errMsg->clear();

	const fs::path targetPath(outPath);
	const fs::path partPath(outPath + ".part");
	std::error_code fsEc;
	fs::remove(partPath, fsEc);

	std::ofstream file(partPath, std::ios::binary | std::ios::trunc);
	if (!file.is_open()) {
		if (errMsg)
			*errMsg = "Не удалось открыть временный файл для записи: " + partPath.string();
		return false;
	}

	auto removePart = [&]() {
		if (file.is_open())
			file.close();
		std::error_code ignored;
		fs::remove(partPath, ignored);
	};

	auto invalidateTransport = [&]() {
		if (socket_) {
			boost::system::error_code ignored;
			socket_->shutdown(tcp::socket::shutdown_both, ignored);
			socket_->close(ignored);
		}
	};

	try {
		if (!socket_ || !socket_->is_open()) {
			if (errMsg) *errMsg = "Socket is not connected";
			removePart();
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
			invalidateTransport();
			removePart();
			return false;
		}

		if (consumeServerErrorResponse(
				*socket_, resp, errMsg, "Сервер не нашёл запрошенный файл")) {
			removePart();
			return false;
		}

		const std::uint64_t total = static_cast<std::uint64_t>(resp.size);
		last_total_.store(total, std::memory_order_relaxed);
		if (total == 0) {
			if (errMsg) *errMsg = "Сервер вернул пустой файл";
			removePart();
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
				removePart();
				return false;
			}

			file.write(reinterpret_cast<const char*>(buffer.data()), chunk);
			if (!file) {
				if (errMsg) *errMsg = "Ошибка записи во временный файл";
				invalidateTransport(); // остаток ответа нельзя оставлять в потоке протокола
				removePart();
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
				removePart();
				return false;
			}
		}

		file.close();
		if (!file) {
			if (errMsg) *errMsg = "Ошибка закрытия временного файла";
			removePart();
			return false;
		}

		fsEc.clear();
		fs::remove(targetPath, fsEc);
		fsEc.clear();
		fs::rename(partPath, targetPath, fsEc);
		if (fsEc) {
			if (errMsg) *errMsg = "Не удалось сохранить файл: " + fsEc.message();
			removePart();
			return false;
		}

		return true;
	}
	catch (const std::exception& e) {
		invalidateTransport();
		removePart();
		if (errMsg) *errMsg = e.what();
		return false;
	}
	catch (...) {
		invalidateTransport();
		removePart();
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
			errMsg = "DNS resolution failed: " + resolve_ec.message();
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		// Проверяем, что есть хотя бы один endpoint
		if (endpoints.begin() == endpoints.end()) {
			errMsg = "No endpoints resolved";
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
				errMsg = "Connection operation stopped unexpectedly";
				if (socket_) { delete socket_; socket_ = nullptr; }
				if (resolver_) { delete resolver_; resolver_ = nullptr; }
				return false;
			}
		}

		if (timedOut) {
			errMsg = "Connection timeout";
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		if (connectEc) {
			errMsg = "Connection failed: " + connectEc.message();
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
        errMsg = "Exception: " + std::string(e.what());
        return false;
    }
    catch (...) {
        if (socket_) { delete socket_; socket_ = nullptr; }
        if (resolver_) { delete resolver_; resolver_ = nullptr; }
        errMsg = "Unknown exception occurred";
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
        if (errMsg) *errMsg = "Socket is not connected";
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
        if (errMsg) *errMsg = "Socket is not connected";
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
        if (errMsg) *errMsg = "Socket is not connected";
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
        if (errMsg) *errMsg = "Socket is not connected";
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
        if (errMsg) *errMsg = "Socket is not connected";
        return false;
    }
    
    if (!read_exact_idle_timeout(*socket_, &hdr, sizeof(hdr), errMsg)) {
		boost::system::error_code ignored;
		socket_->close(ignored);
        return false;
	}
    return true;
}

