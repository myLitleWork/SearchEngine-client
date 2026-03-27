#include "SocketFileSaver.h"
#include <cstdint>
#include <fstream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <boost/system/error_code.hpp>
#include <boost/asio/steady_timer.hpp>



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
    // Пишем сначала во временный файл: при ошибке не оставляем "обрубок".
    const std::string tmpPath = outPath_ + ".part";
    std::error_code fsEc;
    std::filesystem::remove(tmpPath, fsEc); // на всякий случай

    std::ofstream file(tmpPath, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "Не удалось открыть файл для записи: " + tmpPath;
        return false;
    }

    std::vector<std::uint8_t> buffer(bufferSize_);

    std::size_t totalRead = 0;

    // Таймаут бездействия (нет новых байт). Если размер неверный и сервер не досылает — не зависаем.
	constexpr auto kIdleTimeout = std::chrono::seconds(5);
    boost::asio::steady_timer timer(socket_.get_executor());

    while (totalRead < fileSize_) {
        std::size_t toRead = std::min(bufferSize_, fileSize_ - totalRead);

        boost::system::error_code readEc;
        std::size_t bytesRead = 0;
        bool readDone = false;
        bool timedOut = false;

        // Запускаем асинхронное чтение + таймер и "ждём" через run_one().
        socket_.async_read_some(
            boost::asio::buffer(buffer.data(), toRead),
            [&](const boost::system::error_code& ec, std::size_t n) {
                readEc = ec;
                bytesRead = n;
                readDone = true;
                timer.cancel(); // остановить таймер, если чтение завершилось
            }
        );

        timer.expires_after(kIdleTimeout);
        timer.async_wait([&](const boost::system::error_code& ec) {
            if (!ec && !readDone) {
                timedOut = true;
                // Прерываем операции на сокете (чтобы async_read_some завершился).
                boost::system::error_code ignored;
                socket_.cancel(ignored);
            }
        });

        // В Embarcadero/Boost 1.70 context() возвращает execution_context (база).
        // Реально под сокетом используется io_context, у него и есть restart()/run_one().
        auto& ctx = static_cast<boost::asio::io_context&>(socket_.get_executor().context());
        ctx.restart();
        while (!readDone && !timedOut) {
            ctx.run_one();
        }

        if (timedOut) {
            errorMsg = "Таймаут чтения из сокета (ожидалось ещё " +
                       std::to_string(fileSize_ - totalRead) + " байт). Файл не будет сохранён.";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        if (readEc) {
            errorMsg = "Ошибка чтения сокета: " + readEc.message();
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        if (bytesRead == 0) {
            errorMsg = "Соединение закрыто до завершения передачи (получено " +
                       std::to_string(totalRead) + " из " + std::to_string(fileSize_) + " байт).";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
        if (!file) {
            errorMsg = "Ошибка записи в файл.";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        totalRead += bytesRead;

        if (progressCallback_)
            progressCallback_(bytesRead);
    }

    file.close();
    // Атомарно "доставляем" файл в целевой путь.
    std::filesystem::remove(outPath_, fsEc); // если был старый/частичный (на всякий случай)
    std::filesystem::rename(tmpPath, outPath_, fsEc);
    if (fsEc) {
        errorMsg = "Не удалось переименовать временный файл: " + fsEc.message();
        std::filesystem::remove(tmpPath, fsEc);
        return false;
    }
    return true;
}
