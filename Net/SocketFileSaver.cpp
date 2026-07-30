#include "SocketFileSaver.h"
#include <cstdint>
#include <fstream>
#include <vector>
#include <filesystem>
#include <chrono>
#include <memory>
#include <boost/system/error_code.hpp>
#include <boost/asio/steady_timer.hpp>

SocketFileSaver::SocketFileSaver(boost::asio::ip::tcp::socket& socket,
                                 const std::string& outPath,
                                 std::size_t fileSize,
                                 std::size_t bufferSize,
                                 ProgressCallback onProgress,
                                 bool flushStreamToOs)
    : socket_(socket),
      outPath_(outPath),
      fileSize_(fileSize),
      bufferSize_(bufferSize),
      progressCallback_(onProgress),
      flushStreamToOs_(flushStreamToOs) {}

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
        struct ReadState {
            boost::system::error_code readEc;
            std::size_t bytesRead{0};
            bool readDone{false};
            bool timerDone{false};
            bool timedOut{false};
        };

        std::size_t toRead = std::min(bufferSize_, fileSize_ - totalRead);
        auto state = std::make_shared<ReadState>();

        // Запускаем асинхронное чтение + таймер и "ждём" через run_one().
        socket_.async_read_some(
            boost::asio::buffer(buffer.data(), toRead),
            [state, &timer](const boost::system::error_code& ec, std::size_t n) {
                state->readEc = ec;
                state->bytesRead = n;
                state->readDone = true;
                boost::system::error_code ignored;
                timer.cancel(ignored); // остановить таймер, если чтение завершилось
            }
        );

        timer.expires_after(kIdleTimeout);
        timer.async_wait([state, this](const boost::system::error_code& ec) {
            state->timerDone = true;
            if (!ec && !state->readDone) {
                state->timedOut = true;
                // Прерываем операции на сокете (чтобы async_read_some завершился).
                boost::system::error_code ignored;
                socket_.cancel(ignored);
            }
        });

        // В Embarcadero/Boost 1.70 context() возвращает execution_context (база).
        // Реально под сокетом используется io_context, у него и есть restart()/run_one().
        auto& ctx = static_cast<boost::asio::io_context&>(socket_.get_executor().context());
        ctx.restart();
        while (!state->readDone || !state->timerDone) {
            if (ctx.run_one() == 0) {
                errorMsg = "Внутренняя ошибка ожидания операции чтения.";
                file.close();
                std::filesystem::remove(tmpPath, fsEc);
                return false;
            }
        }

        if (state->timedOut) {
            errorMsg = "Таймаут чтения из сокета (ожидалось ещё " +
                       std::to_string(fileSize_ - totalRead) + " байт). Файл не будет сохранён.";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        if (state->readEc) {
            errorMsg = "Ошибка чтения сокета: " + state->readEc.message();
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        if (state->bytesRead == 0) {
            errorMsg = "Соединение закрыто до завершения передачи (получено " +
                       std::to_string(totalRead) + " из " + std::to_string(fileSize_) + " байт).";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data()), state->bytesRead);
        if (!file) {
            errorMsg = "Ошибка записи в файл.";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }

        totalRead += state->bytesRead;

        if (progressCallback_)
            progressCallback_(state->bytesRead);
    }

    // Только для последнего файла в пакете: сброс буфера ofstream в ОС перед close/rename.
    if (flushStreamToOs_) {
        file.flush();
        if (!file) {
            errorMsg = "Ошибка flush перед закрытием временного файла.";
            file.close();
            std::filesystem::remove(tmpPath, fsEc);
            return false;
        }
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
