#include "message_format.h"
#include "metrics.h"
#include "ring_buffer.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>

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

constexpr size_t RING_BUFFER_SIZE = 65536;
using MessageQueue = RingBuffer<Message, RING_BUFFER_SIZE>;

class FeedHandler {
public:
    FeedHandler(uint16_t port, bool use_tcp, bool benchmark_mode)
        : port_(port)
        , use_tcp_(use_tcp)
        , benchmark_mode_(benchmark_mode)
        , running_(true) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    }
    
    ~FeedHandler() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    void run() {
        std::thread processor([this]() { process_messages(); });
        
        if (use_tcp_) {
            receive_tcp();
        } else {
            receive_udp();
        }
        
        processor.join();
    }
    
private:
    void receive_udp() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        bind(sock, (sockaddr*)&addr, sizeof(addr));
        std::cout << "Listening on UDP port " << port_ << "\n";
        
        uint8_t buffer[1024];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        
        while (running_) {
            int len = recvfrom(sock, (char*)buffer, sizeof(buffer), 0, (sockaddr*)&sender, &sender_len);
            
            if (len > 0) {
                uint64_t receive_timestamp = get_timestamp_ns();
                
                Message msg;
                if (deserialize_message(buffer, len, msg)) {
                    uint64_t latency = receive_timestamp - msg.header.exchange_timestamp;
                    
                    while (!message_queue_.try_push(msg) && running_) {
                        std::this_thread::yield();
                    }
                    
                    if (!benchmark_mode_) {
                        latency_metrics_.record(latency);
                    }
                    throughput_metrics_.record_message();
                }
            }
        }
        
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
    
    void receive_tcp() {
        int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = INADDR_ANY;
        
        bind(listen_sock, (sockaddr*)&addr, sizeof(addr));
        listen(listen_sock, 1);
        std::cout << "Listening on TCP port " << port_ << "\n";
        
        sockaddr_in client{};
        socklen_t client_len = sizeof(client);
        int sock = accept(listen_sock, (sockaddr*)&client, &client_len);
        std::cout << "Client connected\n";
        
        uint8_t buffer[1024];
        
        while (running_) {
            int len = recv(sock, (char*)buffer, sizeof(buffer), 0);
            
            if (len <= 0) break;
            
            uint64_t receive_timestamp = get_timestamp_ns();
            
            Message msg;
            if (deserialize_message(buffer, len, msg)) {
                uint64_t latency = receive_timestamp - msg.header.exchange_timestamp;
                
                while (!message_queue_.try_push(msg) && running_) {
                    std::this_thread::yield();
                }
                
                if (!benchmark_mode_) {
                    latency_metrics_.record(latency);
                }
                throughput_metrics_.record_message();
            }
        }
        
#ifdef _WIN32
        closesocket(sock);
        closesocket(listen_sock);
#else
        close(sock);
        close(listen_sock);
#endif
    }
    
    void process_messages() {
        std::cout << "Processing thread started\n";
        throughput_metrics_.start();
        
        uint64_t processed = 0;
        auto last_print = std::chrono::steady_clock::now();
        
        while (running_) {
            auto msg_opt = message_queue_.try_pop();
            
            if (msg_opt) {
                ++processed;
                
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 1) {
                    print_stats(processed);
                    last_print = now;
                }
            } else {
                std::this_thread::yield();
            }
        }
    }
    
    void print_stats(uint64_t processed) {
        std::cout << "\n=== Statistics ===\n";
        std::cout << "Processed: " << processed << " messages\n";
        std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
                  << throughput_metrics_.messages_per_second() << " msg/sec\n";
        
        if (!benchmark_mode_) {
            auto stats = latency_metrics_.compute();
            if (stats.count > 0) {
                std::cout << "Latency (us) - Min: " << stats.min_ns / 1000 
                          << ", Avg: " << stats.avg_ns / 1000 
                          << ", P99: " << stats.p99_ns / 1000 
                          << ", Max: " << stats.max_ns / 1000 << "\n";
            }
        }
        std::cout << std::flush;
    }
    
    uint16_t port_;
    bool use_tcp_;
    bool benchmark_mode_;
    std::atomic<bool> running_;
    MessageQueue message_queue_;
    LatencyMetrics latency_metrics_;
    ThroughputMetrics throughput_metrics_;
};

} // namespace mdfh

int main(int argc, char* argv[]) {
    try {
        uint16_t port = 9999;
        bool use_tcp = false;
        bool benchmark = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--tcp") use_tcp = true;
            else if (arg == "--udp") use_tcp = false;
            else if (arg == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
            else if (arg == "--benchmark") benchmark = true;
        }
        
        std::cout << "Market Data Feed Handler\n";
        std::cout << "Protocol: " << (use_tcp ? "TCP" : "UDP") << "\n\n";
        
        mdfh::FeedHandler handler(port, use_tcp, benchmark);
        handler.run();
        
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
