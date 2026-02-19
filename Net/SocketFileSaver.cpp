#include "SocketFileSaver.h"
#include <cstdint>
#include <fstream>
#include <vector>
#include <boost/system/error_code.hpp>

SocketFileSaver::SocketFileSaver(boost::asio::ip::tcp::socket& socket,
                                 const std::string& outPath,
                                 std::size_t fileSize,
                                 std::size_t bufferSize,
                                 ProgressCallback onProgress)
    : socket_(socket),
      outPath_(outPath),
      fileSize_(fileSize),
      bufferSize_(bufferSize),
      progressCallback_(onProgress) {}

bool SocketFileSaver::saveToFile(std::string& errorMsg)
{
    std::ofstream file(outPath_, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "Не удалось открыть файл для записи: " + outPath_;
        return false;
    }

    std::vector<std::uint8_t> buffer(bufferSize_);

    std::size_t totalRead = 0;
    boost::system::error_code ec;

    while (totalRead < fileSize_) {
        std::size_t toRead = std::min(bufferSize_, fileSize_ - totalRead);
        std::size_t bytesRead = socket_.read_some(boost::asio::buffer(buffer.data(), toRead), ec);

        if (ec) {
            errorMsg = "Ошибка чтения сокета: " + ec.message();
            return false;
        }
        if (bytesRead == 0) {
            errorMsg = "Socket closed before transfer complete.";
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
        if (!file) {
            errorMsg = "Ошибка записи в файл.";
            return false;
        }

        totalRead += bytesRead;

        if (progressCallback_)
            progressCallback_(bytesRead);
    }

    file.close();
    return true;
}
