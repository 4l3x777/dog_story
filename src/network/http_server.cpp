#include <http_server.h>

#include <boost/asio/dispatch.hpp>
#include <iostream>
#include <logger/logger.h>

namespace http_server {

void ReportError(beast::error_code ec, const std::string_view& what) {
    logger::LogMessage::where_type type;
    if (what == "write") {
        type = logger::LogMessage::where_type::write;
    }
    else if (what == "read") {
        type = logger::LogMessage::where_type::read;
    }
    else if (what == "accept") {
        type = logger::LogMessage::where_type::accept;
    }
    else if (what == "close") {
        type = logger::LogMessage::where_type::close;
    }
    LOG_MSG().network_error(ec, type);
}

const beast::tcp_stream& SessionBase::GetStream() { 
    return stream_; 
}

void SessionBase::Run(){
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

void SessionBase::Read() {
    using namespace std::literals;
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                    // По окончании операции будет вызван метод OnRead
                    beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        return ReportError(ec, "close"sv);
    }
}

}  // namespace http_server