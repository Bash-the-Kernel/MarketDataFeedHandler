#include "message_format.h"
#include "metrics.h"
#include <boost/asio.hpp>
#include <iostream>
#include <random>
#include <thread>
#include <chrono>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace mdfh {

class FeedSimulator {
public:
    FeedSimulator(boost::asio::io_context& io_context, 
                  const std::string& host, uint16_t port, bool use_tcp, 
                  uint32_t rate_limit, bool benchmark_mode)
        : io_context_(io_context)
        , use_tcp_(use_tcp)
        , rate_limit_(rate_limit)
        , benchmark_mode_(benchmark_mode)
        , sequence_(0)
        , rng_(std::random_device{}())
        , price_dist_(100.0, 200.0)
        , size_dist_(100, 10000) {
        
        if (use_tcp_) {
            tcp::resolver resolver(io_context_);
            auto endpoints = resolver.resolve(host, std::to_string(port));
            tcp_socket_ = std::make_unique<tcp::socket>(io_context_);
            boost::asio::connect(*tcp_socket_, endpoints);
            std::cout << "Connected via TCP to " << host << ":" << port << "\n";
        } else {
            udp::resolver resolver(io_context_);
            udp_endpoint_ = *resolver.resolve(udp::v4(), host, std::to_string(port)).begin();
            udp_socket_ = std::make_unique<udp::socket>(io_context_, udp::endpoint(udp::v4(), 0));
            std::cout << "Sending via UDP to " << host << ":" << port << "\n";
        }
    }
    
    void run() {
        std::cout << "Starting feed simulator (rate: " << rate_limit_ << " msg/sec, benchmark: " 
                  << (benchmark_mode_ ? "ON" : "OFF") << ")\n";
        
        metrics_.start();
        auto next_send = std::chrono::steady_clock::now();
        const auto interval = std::chrono::microseconds(1'000'000 / rate_limit_);
        
        while (true) {
            send_random_message();
            
            if (!benchmark_mode_) {
                next_send += interval;
                std::this_thread::sleep_until(next_send);
            }
            
            if (sequence_ % 10000 == 0) {
                std::cout << "Sent: " << sequence_ << " messages, "
                          << static_cast<uint64_t>(metrics_.messages_per_second()) << " msg/sec\n";
            }
        }
    }
    
private:
    void send_random_message() {
        uint8_t buffer[64];
        size_t size;
        
        uint16_t symbol_id = rng_() % 100;
        uint64_t timestamp = get_timestamp_ns();
        
        if (rng_() % 2 == 0) {
            size = serialize_trade(buffer, symbol_id, sequence_++, timestamp,
                                  price_dist_(rng_), size_dist_(rng_), sequence_);
        } else {
            double mid = price_dist_(rng_);
            size = serialize_quote(buffer, symbol_id, sequence_++, timestamp,
                                  mid - 0.01, mid + 0.01, size_dist_(rng_), size_dist_(rng_));
        }
        
        if (use_tcp_) {
            boost::asio::write(*tcp_socket_, boost::asio::buffer(buffer, size));
        } else {
            udp_socket_->send_to(boost::asio::buffer(buffer, size), udp_endpoint_);
        }
        
        metrics_.record_message();
    }
    
    boost::asio::io_context& io_context_;
    std::unique_ptr<tcp::socket> tcp_socket_;
    std::unique_ptr<udp::socket> udp_socket_;
    udp::endpoint udp_endpoint_;
    bool use_tcp_;
    uint32_t rate_limit_;
    bool benchmark_mode_;
    uint32_t sequence_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> price_dist_;
    std::uniform_int_distribution<uint64_t> size_dist_;
    ThroughputMetrics metrics_;
};

} // namespace mdfh

int main(int argc, char* argv[]) {
    try {
        std::string host = "127.0.0.1";
        uint16_t port = 9999;
        bool use_tcp = false;
        uint32_t rate = 10000;
        bool benchmark = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--tcp") {
                use_tcp = true;
            } else if (arg == "--udp") {
                use_tcp = false;
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (arg == "--rate" && i + 1 < argc) {
                rate = std::stoi(argv[++i]);
            } else if (arg == "--benchmark") {
                benchmark = true;
            } else if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            }
        }
        
        boost::asio::io_context io_context;
        mdfh::FeedSimulator simulator(io_context, host, port, use_tcp, rate, benchmark);
        simulator.run();
        
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
