#pragma once
#include <string>
#include <functional>
#include <boost/asio.hpp>

class SocketFileSaver {
public:
    using ProgressCallback = std::function<void(std::size_t)>;

    SocketFileSaver(boost::asio::ip::tcp::socket& socket,
                    const std::string& outPath,
                    std::size_t fileSize,
                    std::size_t bufferSize = 64 * 1024,
                    ProgressCallback onProgress = nullptr);

    bool saveToFile(std::string& errorMsg);

private:
    boost::asio::ip::tcp::socket& socket_;
    std::string outPath_;
    std::size_t fileSize_;
    std::size_t bufferSize_;
    ProgressCallback progressCallback_;
};
