#include "config.hpp"
#include "http_server.hpp"
#include "store.hpp"
#include "udp_server.hpp"
#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    try {
        Config cfg = load_config(argc > 1 ? argv[1] : "");
        std::cerr << "[cfg] udp=" << cfg.udp_port << " http=" << cfg.http_port
                  << " own_mmsi=" << cfg.own_mmsi
                  << " origin=(" << cfg.origin_lat << "," << cfg.origin_lon << ")"
                  << " max_age=" << cfg.max_age_s << "s predict=" << cfg.predict_s
                  << "s warn=" << cfg.warn_m << "m\n";

        boost::asio::io_context io;
        Store store(cfg.max_vessels);
        UdpServer udp(io, cfg, store);
        HttpServer http(io, cfg, store);
        udp.start();
        http.start();

        boost::asio::signal_set sigs(io, SIGINT, SIGTERM);
        sigs.async_wait([&](boost::system::error_code, int) {
            std::cerr << "[main] shutting down\n";
            io.stop();
        });

        auto work = boost::asio::make_work_guard(io);
        std::vector<std::thread> pool;
        unsigned nthreads = std::max(2u, std::thread::hardware_concurrency() / 2);
        for (unsigned i = 0; i < nthreads; ++i)
            pool.emplace_back([&io] { io.run(); });
        for (auto& t : pool) t.join();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
