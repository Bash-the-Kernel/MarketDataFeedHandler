#include "message_format.h"
#include "metrics.h"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace mdfh {

class FeedSimulator {
public:
    FeedSimulator(const std::string& host, uint16_t port, bool use_tcp, 
                  uint32_t rate_limit, bool benchmark_mode)
        : use_tcp_(use_tcp)
        , rate_limit_(rate_limit)
        , benchmark_mode_(benchmark_mode)
        , sequence_(0)
        , rng_(std::random_device{}())
        , price_dist_(100.0, 200.0)
        , size_dist_(100, 10000) {
        
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        
        if (use_tcp_) {
            sock_ = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
            connect(sock_, (sockaddr*)&addr, sizeof(addr));
            std::cout << "Connected via TCP to " << host << ":" << port << "\n";
        } else {
            sock_ = socket(AF_INET, SOCK_DGRAM, 0);
            addr_.sin_family = AF_INET;
            addr_.sin_port = htons(port);
            inet_pton(AF_INET, host.c_str(), &addr_.sin_addr);
            std::cout << "Sending via UDP to " << host << ":" << port << "\n";
        }
    }
    
    ~FeedSimulator() {
#ifdef _WIN32
        closesocket(sock_);
        WSACleanup();
#else
        close(sock_);
#endif
    }
    
    void run() {
        std::cout << "Starting feed simulator (rate: " << rate_limit_ << " msg/sec)\n";
        
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
            send(sock_, (const char*)buffer, size, 0);
        } else {
            sendto(sock_, (const char*)buffer, size, 0, (sockaddr*)&addr_, sizeof(addr_));
        }
        
        metrics_.record_message();
    }
    
    int sock_;
    sockaddr_in addr_{};
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
            if (arg == "--tcp") use_tcp = true;
            else if (arg == "--udp") use_tcp = false;
            else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
            else if (arg == "--rate" && i + 1 < argc) rate = std::stoi(argv[++i]);
            else if (arg == "--benchmark") benchmark = true;
            else if (arg == "--host" && i + 1 < argc) host = argv[++i];
        }
        
        mdfh::FeedSimulator simulator(host, port, use_tcp, rate, benchmark);
        simulator.run();
        
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
