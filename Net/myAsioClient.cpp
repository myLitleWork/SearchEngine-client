//---------------------------------------------------------------------------

#pragma hdrstop

#include "myAsioClient.h"
#include <algorithm>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <atomic>
#include <chrono>
#include <thread>
//---------------------------------------------------------------------------
#pragma package(smart_init)


void MyAsioClient::disConnect()
{
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

bool MyAsioClient::requestBinFile(const std::string& path,
								  std::vector<uint8_t>& out,
								  std::atomic_uint64_t* progress,
								  std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);

    out.clear();
    try
	{
		// --- write request ---
        header req{};
        req.size = static_cast<uint32_t>(path.size());
		req.command = COMMAND::GETBINFILE;

		boost::asio::write(*socket_, boost::asio::buffer(&req, sizeof(req)));
		if (!path.empty())
            boost::asio::write(*socket_, boost::asio::buffer(path.data(), path.size()));

        // --- read response header ---
		header resp{};
		boost::asio::read(*socket_, boost::asio::buffer(&resp, sizeof(resp)));

		if (resp.command == COMMAND::SOMEERROR) {
			if (errMsg) *errMsg = "Server returned SOMEERROR";
			return false;
		}

		if (resp.command == COMMAND::SERVER_BUSY_ERROR) {
			if (errMsg) *errMsg = "Server returned SERVER_BUSY_ERROR";
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
            boost::asio::read(*socket_, boost::asio::buffer(out.data() + off, chunk));
            off += chunk;
            if (progress) progress->store(off, std::memory_order_relaxed);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        if (errMsg) *errMsg = e.what();
        return false;
    }
}


// Îñíîâíàÿ âåðñèÿ (std::string)
bool MyAsioClient::tryConnect(const std::string& host,
                              const std::string& port,
                              std::string& errMsg)
{
	try {
		if (socket_) { delete socket_; socket_ = nullptr; }
		if (resolver_) { delete resolver_; resolver_ = nullptr; }

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

		// Используем асинхронное подключение с таймаутом (2 секунды)
		// Это предотвращает долгое зависание при недоступном сервере
		boost::asio::deadline_timer timer(io_context_);
		timer.expires_from_now(boost::posix_time::seconds(2));
		
		std::atomic<bool> completed(false);
		boost::system::error_code connect_ec = boost::asio::error::would_block;

		// Обработчик успешного подключения
		boost::asio::async_connect(*socket_, endpoints,
			[&](const boost::system::error_code& ec, const tcp::endpoint&) {
				if (!completed.exchange(true)) {
					connect_ec = ec;
					timer.cancel();
				}
			});

		// Обработчик таймаута
		timer.async_wait([&](const boost::system::error_code& ec) {
			if (!completed.exchange(true) && ec != boost::asio::error::operation_aborted) {
				socket_->cancel();
				connect_ec = boost::asio::error::timed_out;
			}
		});

		// Запускаем io_context с ограничением времени
		// Используем run() в отдельном потоке с таймаутом
		std::thread run_thread([this]() {
			try {
				io_context_.run();
			} catch (...) {
				// Игнорируем исключения в потоке
			}
		});
		
		// Ждем завершения операции (максимум 3 секунды)
		auto start_time = std::chrono::steady_clock::now();
		const auto max_timeout = std::chrono::seconds(3);
		
		while (!completed.load()) {
			if (std::chrono::steady_clock::now() - start_time > max_timeout) {
				io_context_.stop();
				if (!completed.exchange(true)) {
					connect_ec = boost::asio::error::timed_out;
				}
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		
		// Ждем завершения потока
		if (run_thread.joinable()) {
			run_thread.join();
		}
		
		// Сбрасываем io_context для следующего использования
		io_context_.reset();

		// Проверяем результат подключения
		if (connect_ec == boost::asio::error::would_block) {
			// Операция не завершилась - таймаут
			errMsg = "Connection timeout";
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}
		
		if (connect_ec) {
			// Есть ошибка подключения
			if (connect_ec == boost::asio::error::timed_out) {
				errMsg = "Connection timeout";
			} else {
				errMsg = "Connection failed: " + connect_ec.message();
			}
			if (socket_) { delete socket_; socket_ = nullptr; }
			if (resolver_) { delete resolver_; resolver_ = nullptr; }
			return false;
		}

		// Подключение успешно (connect_ec == 0)
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
        // 1) ÷èòàåì çàãîëîâîê ðîâíî sizeof(header)
        boost::asio::read(*socket_, boost::asio::buffer(&myHeader_, sizeof(myHeader_)));
        if (myHeader_.command == COMMAND::SOMEERROR) {
            if (errMsg) *errMsg = "Server returned SOMEERROR";
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

            boost::asio::read(*socket_,
                              boost::asio::buffer(buf.data(), chunk),
                              boost::asio::transfer_exactly(chunk));

            out.append(buf.data(), buf.size());
            left -= chunk;

            if (progress) progress->fetch_add(buf.size(), std::memory_order_relaxed);
        }

        return true;
    } catch (const std::exception& e) {
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
        // 1) ÷èòàåì çàãîëîâîê
        boost::asio::read(*socket_, boost::asio::buffer(&myHeader_, sizeof(myHeader_)));
        if (myHeader_.command == COMMAND::SOMEERROR) {
            if (errMsg) *errMsg = "Server returned SOMEERROR";
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

            boost::asio::read(*socket_,
                              boost::asio::buffer(buf.data(), chunk),
                              boost::asio::transfer_exactly(chunk));

            out.insert(out.end(), buf.begin(), buf.end());
            left -= chunk;

            if (progress) progress->fetch_add(buf.size(), std::memory_order_relaxed);
        }

        return true;
    } catch (const std::exception& e) {
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
    
    try {
        boost::asio::read(*socket_, boost::asio::buffer(&hdr, sizeof(hdr)));
        return true;
    } catch (const std::exception& e) {
        if (errMsg) *errMsg = e.what();
        return false;
    }
}

