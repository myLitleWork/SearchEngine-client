#pragma once
#include "SearchRequestControl.h"
#include <boost/asio.hpp>

// Socket operations and cancellation run on the owning request thread. The UI
// changes only atomics, never closes a socket or waits for a session mutex.
template<bool Writing, class Buffer>
bool searchCancellableIo(boost::asio::ip::tcp::socket& socket, Buffer buffer,
    SearchRequestControl& control, std::string* error) {
    control.checkpoint();
    if (control.cancelled.load()) {
        if (error) *error = "Search cancelled";
        return false;
    }
    bool done = false;
    boost::system::error_code result;
    auto complete = [&](const boost::system::error_code& ec, std::size_t) { result = ec; done = true; };
    if constexpr (Writing) boost::asio::async_write(socket, buffer, complete);
    else boost::asio::async_read(socket, buffer, complete);
    auto& context = static_cast<boost::asio::io_context&>(socket.get_executor().context());
    context.restart();
    while (!done) {
        context.run_one_for(std::chrono::milliseconds(100));
        if (control.cancelled.load() && !done) {
            boost::system::error_code ignored;
            socket.cancel(ignored);
        }
    }
    if (control.cancelled.load() || result) {
        if (error) *error = control.cancelled.load() ? "Search cancelled" : result.message();
        return false;
    }
    control.checkpoint();
    return true;
}
