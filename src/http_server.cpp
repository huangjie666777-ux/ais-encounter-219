#include "http_server.hpp"
#include "query.hpp"
#include <boost/beast.hpp>
#include <iostream>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    HttpSession(tcp::socket sock, const Config& cfg, const Store& store)
        : stream_(std::move(sock)), cfg_(cfg), store_(store) {}

    void run() { do_read(); }

private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    const Config& cfg_;
    const Store& store_;

    void do_read() {
        req_ = {};
        stream_.expires_after(std::chrono::seconds(15));
        auto self = shared_from_this();
        http::async_read(stream_, buffer_, req_,
            [self](beast::error_code ec, std::size_t) {
                if (ec) return;
                self->handle();
            });
    }

    void respond(http::status st, std::string body) {
        auto res = std::make_shared<http::response<http::string_body>>(st, req_.version());
        res->set(http::field::server, "ais-encounter");
        res->set(http::field::content_type, "application/json");
        res->keep_alive(false);
        res->body() = std::move(body);
        res->prepare_payload();
        auto self = shared_from_this();
        http::async_write(stream_, *res,
            [self, res](beast::error_code, std::size_t) {
                self->stream_.socket().shutdown(tcp::socket::shutdown_send);
            });
    }

    void handle() {
        if (req_.method() != http::verb::get) {
            respond(http::status::method_not_allowed, "{\"error\":\"GET only\"}");
            return;
        }
        std::string target(req_.target());
        auto q = target.find('?');
        if (q != std::string::npos) target.erase(q);
        if (target == "/ships") {
            respond(http::status::ok, ships_json(store_, cfg_));
        } else if (target == "/encounters") {
            respond(http::status::ok, encounters_json(store_, cfg_));
        } else if (target == "/health") {
            respond(http::status::ok, "{\"status\":\"up\"}");
        } else {
            respond(http::status::not_found, "{\"error\":\"unknown route\"}");
        }
    }
};

HttpServer::HttpServer(boost::asio::io_context& io, const Config& cfg, const Store& store)
    : acceptor_(io, tcp::endpoint(tcp::v4(), cfg.http_port)), cfg_(cfg), store_(store) {}

void HttpServer::start() {
    do_accept();
    std::cerr << "[http] listening on 0.0.0.0:" << cfg_.http_port << "\n";
}

void HttpServer::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket sock) {
        if (!ec) std::make_shared<HttpSession>(std::move(sock), cfg_, store_)->run();
        do_accept();
    });
}
