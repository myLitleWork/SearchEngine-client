//---------------------------------------------------------------------------

#pragma hdrstop

#include "myAsioClient.h"
#include <algorithm>
//---------------------------------------------------------------------------
#pragma package(smart_init)


void MyAsioClient::disConnect()
{
	if (socket_) {
        boost::system::error_code ec;
		if (socket_->is_open()) {
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			socket_->close(ec);
			// тут можно логировать ec, если нужно
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


// Основная версия (std::string)
bool MyAsioClient::tryConnect(const std::string& host,
                              const std::string& port,
                              std::string& errMsg)
{
    try {
		if (socket_) { delete socket_; socket_ = nullptr; }
		if (resolver_) { delete resolver_; resolver_ = nullptr; }

		socket_ = new tcp::socket(io_context_);
		resolver_ = new tcp::resolver(io_context_);

		boost::asio::connect(*socket_, resolver_->resolve(host, port));

        return true;
    }
    catch (const std::exception& e) {
        errMsg = e.what();
        return false;
    }
}

// Перегрузка для AnsiString
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

    out.clear();
    try {
        // 1) читаем заголовок ровно sizeof(header)
        boost::asio::read(*socket_, boost::asio::buffer(&myHeader_, sizeof(myHeader_)));
        if (myHeader_.command == COMMAND::SOMEERROR) {
            if (errMsg) *errMsg = "Server returned SOMEERROR";
            return false;
        }

        const std::size_t total = myHeader_.size;
        last_total_.store(total, std::memory_order_relaxed);
        if (progress) progress->store(0, std::memory_order_relaxed);

        out.reserve(out.size() + total);

        // 2) читаем тело фиксированными блоками
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

    out.clear();
    try {
        // 1) читаем заголовок
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
        disConnect();          // аккуратно закроем сокет/резолвер и освободим память
    } catch (...) {
        // ничего не делаем — деструктор не должен бросать исключения
    }
}

bool MyAsioClient::readHeader(header& hdr, std::string* errMsg)
{
    std::lock_guard<std::mutex> lk(io_mutex_);
    try {
        boost::asio::read(*socket_, boost::asio::buffer(&hdr, sizeof(hdr)));
        return true;
    } catch (const std::exception& e) {
        if (errMsg) *errMsg = e.what();
        return false;
    }
}


