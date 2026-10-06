#pragma once
#include "config.hpp"
#include "store.hpp"
#include <boost/asio.hpp>

// Beast HTTP 查询服务：GET /ships, GET /encounters
class HttpServer {
public:
    HttpServer(boost::asio::io_context& io, const Config& cfg, const Store& store);
    void start();

private:
    void do_accept();
    boost::asio::ip::tcp::acceptor acceptor_;
    const Config& cfg_;
    const Store& store_;
};
