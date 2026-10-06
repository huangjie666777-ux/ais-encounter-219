#pragma once
#include "config.hpp"
#include "reassembler.hpp"
#include "store.hpp"
#include <boost/asio.hpp>
#include <array>

// 异步UDP接入：接收 !AIVDM 数据报 -> 校验 -> 重组 -> 解码 -> 入库
class UdpServer {
public:
    UdpServer(boost::asio::io_context& io, const Config& cfg, Store& store);
    void start();

private:
    void do_receive();
    void handle_line(const std::string& line, const std::string& endpoint);
    void do_expire();

    boost::asio::ip::udp::socket socket_;
    boost::asio::steady_timer expire_timer_;
    const Config& cfg_;
    Store& store_;
    Reassembler reasm_;
    std::array<char, 2048> buf_{};
    boost::asio::ip::udp::endpoint remote_;
};
