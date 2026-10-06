#include "udp_server.hpp"
#include "ais.hpp"
#include "nmea.hpp"
#include <iostream>

using boost::asio::ip::udp;
using Clock = std::chrono::steady_clock;

UdpServer::UdpServer(boost::asio::io_context& io, const Config& cfg, Store& store)
    : socket_(io, udp::endpoint(udp::v4(), cfg.udp_port)),
      expire_timer_(io), cfg_(cfg), store_(store),
      reasm_(cfg.max_groups, cfg.max_group_messages) {}

void UdpServer::start() {
    do_receive();
    do_expire();
    std::cerr << "[udp] listening on 0.0.0.0:" << cfg_.udp_port << "\n";
}

void UdpServer::do_receive() {
    socket_.async_receive_from(boost::asio::buffer(buf_), remote_,
        [this](boost::system::error_code ec, std::size_t n) {
            if (!ec && n > 0 && n <= cfg_.max_datagram) {
                std::string endpoint = remote_.address().to_string() + ":" +
                                       std::to_string(remote_.port());
                handle_line(std::string(buf_.data(), n), endpoint);
            }
            if (!ec) do_receive();
        });
}

void UdpServer::handle_line(const std::string& raw, const std::string& endpoint) {
    std::string line = raw;
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                             line.back() == ' ' || line.back() == '\t'))
        line.pop_back();
    auto s = parse_nmea(line);
    if (!s) return;
    auto now = Clock::now();
    auto full = reasm_.add(endpoint, *s, now);
    if (!full) return;
    auto msg = decode_ais(full->first, full->second);
    if (!msg) return;
    if (auto* d = std::get_if<DynamicReport>(&*msg)) {
        store_.apply_dynamic(*d, now);
        std::cerr << "[ais] dynamic mmsi=" << d->mmsi
                  << " pos=" << (d->has_pos ? "ok" : "missing") << "\n";
    } else if (auto* sn = std::get_if<StaticName>(&*msg)) {
        store_.apply_name(*sn);
        std::cerr << "[ais] name mmsi=" << sn->mmsi << " name=" << sn->name << "\n";
    }
}

void UdpServer::do_expire() {
    expire_timer_.expires_after(std::chrono::seconds(1));
    expire_timer_.async_wait([this](boost::system::error_code ec) {
        if (!ec) {
            reasm_.expire(Clock::now(), std::chrono::seconds(5)); // 未齐5秒清理
            do_expire();
        }
    });
}
