#include "message_format.h"
#include "metrics.h"
#include "ring_buffer.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <iomanip>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace mdfh {

constexpr size_t RING_BUFFER_SIZE = 65536;
using MessageQueue = RingBuffer<Message, RING_BUFFER_SIZE>;

class FeedHandler {
public:
    FeedHandler(uint16_t port, bool use_tcp, bool benchmark_mode)
        : io_context_()
        , port_(port)
        , use_tcp_(use_tcp)
        , benchmark_mode_(benchmark_mode)
        , running_(true) {
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
    
    void stop() {
        running_ = false;
    }
    
private:
    void receive_udp() {
        udp::socket socket(io_context_, udp::endpoint(udp::v4(), port_));
        std::cout << "Listening on UDP port " << port_ << "\n";
        
        uint8_t buffer[1024];
        udp::endpoint sender_endpoint;
        
        while (running_) {
            boost::system::error_code ec;
            size_t len = socket.receive_from(boost::asio::buffer(buffer), sender_endpoint, 0, ec);
            
            if (!ec && len > 0) {
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
    }
    
    void receive_tcp() {
        tcp::acceptor acceptor(io_context_, tcp::endpoint(tcp::v4(), port_));
        std::cout << "Listening on TCP port " << port_ << "\n";
        
        tcp::socket socket(io_context_);
        acceptor.accept(socket);
        std::cout << "Client connected\n";
        
        uint8_t buffer[1024];
        
        while (running_) {
            boost::system::error_code ec;
            size_t len = socket.read_some(boost::asio::buffer(buffer), ec);
            
            if (ec) break;
            
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
    }
    
    void process_messages() {
        std::cout << "Processing thread started\n";
        throughput_metrics_.start();
        
        uint64_t processed = 0;
        auto last_print = std::chrono::steady_clock::now();
        
        while (running_) {
            auto msg_opt = message_queue_.try_pop();
            
            if (msg_opt) {
                const Message& msg = *msg_opt;
                
                if (msg.header.type == MessageType::Trade) {
                    // Process trade
                } else if (msg.header.type == MessageType::Quote) {
                    // Process quote
                }
                
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
                std::cout << "Latency (ns) - Min: " << stats.min_ns 
                          << ", Avg: " << stats.avg_ns 
                          << ", P99: " << stats.p99_ns 
                          << ", Max: " << stats.max_ns << "\n";
                std::cout << "Latency (us) - Min: " << stats.min_ns / 1000 
                          << ", Avg: " << stats.avg_ns / 1000 
                          << ", P99: " << stats.p99_ns / 1000 
                          << ", Max: " << stats.max_ns / 1000 << "\n";
            }
        }
        std::cout << std::flush;
    }
    
    boost::asio::io_context io_context_;
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
            if (arg == "--tcp") {
                use_tcp = true;
            } else if (arg == "--udp") {
                use_tcp = false;
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (arg == "--benchmark") {
                benchmark = true;
            }
        }
        
        std::cout << "Market Data Feed Handler\n";
        std::cout << "Protocol: " << (use_tcp ? "TCP" : "UDP") << "\n";
        std::cout << "Benchmark mode: " << (benchmark ? "ON" : "OFF") << "\n\n";
        
        mdfh::FeedHandler handler(port, use_tcp, benchmark);
        handler.run();
        
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
