# MarketDataFeedHandler

A high-performance, real-time market data feed handler that simulates, ingests, and processes high-frequency market data using modern C++20.

## Architecture

### Components

1. **feed_simulator**: Generates and transmits binary market data (trades and quotes) over TCP or UDP
2. **feed_handler**: Receives, parses, timestamps, and processes incoming market data messages

### Design Highlights

- **Binary Protocol**: Compact 32-byte trades and 40-byte quotes for minimal network overhead
- **Lock-Free Ring Buffer**: Zero-allocation SPSC queue for inter-thread message passing
- **Asynchronous I/O**: Boost.Asio for high-performance network operations
- **Multi-threaded**: Separate threads for network I/O and message processing
- **Low Latency**: Optimized for microsecond-level latency tracking

### Message Format

```
MessageHeader (16 bytes):
  - type: 1 byte (Trade=1, Quote=2)
  - reserved: 1 byte
  - symbol_id: 2 bytes
  - sequence: 4 bytes
  - exchange_timestamp: 8 bytes (nanoseconds)

TradeMessage (32 bytes total):
  - header: 16 bytes
  - price: 8 bytes (double)
  - quantity: 8 bytes
  - trade_id: 8 bytes

QuoteMessage (40 bytes total):
  - header: 16 bytes
  - bid_price: 8 bytes (double)
  - ask_price: 8 bytes (double)
  - bid_size: 8 bytes
  - ask_size: 8 bytes
```

## Performance Expectations

- **Throughput**: 100K+ messages/second on modern hardware
- **Latency**: Sub-microsecond processing latency (excluding network)
- **Memory**: Zero allocations in hot path after initialization
- **CPU**: ~1-2 cores at full load

## Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20+
- Boost 1.70+ (system component)

### Installing Boost

**Windows (vcpkg)**:
```bash
vcpkg install boost-asio:x64-windows
```

**Ubuntu/Debian**:
```bash
sudo apt-get install libboost-all-dev
```

**macOS**:
```bash
brew install boost
```

## Build Instructions

### Windows
```bash
scripts\build.bat
```

### Linux/macOS
```bash
chmod +x scripts/build.sh
./scripts/build.sh
```

### Manual Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Running the System

### Basic Usage (UDP)

Terminal 1 - Start handler:
```bash
./build/feed_handler --udp --port 9999
```

Terminal 2 - Start simulator:
```bash
./build/feed_simulator --udp --port 9999 --rate 10000
```

### TCP Mode

Terminal 1:
```bash
./build/feed_handler --tcp --port 9999
```

Terminal 2:
```bash
./build/feed_simulator --tcp --port 9999 --rate 10000
```

### Command Line Options

**feed_handler**:
- `--tcp`: Use TCP (default: UDP)
- `--udp`: Use UDP
- `--port <N>`: Listen port (default: 9999)
- `--benchmark`: Disable latency tracking for max throughput

**feed_simulator**:
- `--tcp`: Use TCP (default: UDP)
- `--udp`: Use UDP
- `--host <addr>`: Target host (default: 127.0.0.1)
- `--port <N>`: Target port (default: 9999)
- `--rate <N>`: Messages per second (default: 10000)
- `--benchmark`: Send at maximum rate without throttling

## Benchmark Mode

Measures maximum throughput without latency tracking overhead.

### Windows
```bash
scripts\run_benchmark.bat
```

### Linux/macOS
```bash
chmod +x scripts/run_benchmark.sh
./scripts/run_benchmark.sh
```

### Manual Benchmark

Terminal 1:
```bash
./build/feed_handler --udp --benchmark
```

Terminal 2:
```bash
./build/feed_simulator --udp --rate 100000 --benchmark
```

Expected output: 100K-500K+ msg/sec depending on hardware.

## Running Tests

```bash
cd build
ctest --output-on-failure
```

Or run directly:
```bash
./build/unit_tests  # Linux/macOS
.\build\Release\unit_tests.exe  # Windows
```

## Project Structure

```
MarketDataFeedHandler/
├── include/
│   ├── message_format.h    # Binary message definitions
│   ├── metrics.h           # Latency and throughput tracking
│   └── ring_buffer.h       # Lock-free SPSC queue
├── src/
│   ├── feed_simulator.cpp  # Market data generator
│   ├── feed_handler.cpp    # Market data receiver/processor
│   ├── message_format.cpp  # Serialization implementation
│   └── metrics.cpp         # Metrics implementation
├── tests/
│   ├── test_ring_buffer.cpp
│   ├── test_message_format.cpp
│   └── test_metrics.cpp
├── scripts/
│   ├── build.bat           # Windows build script
│   ├── build.sh            # Linux/macOS build script
│   ├── run_benchmark.bat   # Windows benchmark
│   └── run_benchmark.sh    # Linux/macOS benchmark
├── CMakeLists.txt
└── README.md
```

## Key Implementation Details

### Lock-Free Ring Buffer
- Single Producer Single Consumer (SPSC) design
- Uses atomic operations with relaxed memory ordering for performance
- Cache-line aligned head/tail pointers to prevent false sharing
- Fixed size (65536 messages) to avoid dynamic allocation

### Latency Measurement
- Timestamps captured at message generation (exchange_timestamp)
- Receive timestamp captured immediately upon network receipt
- Latency = receive_timestamp - exchange_timestamp
- Statistics computed: min, avg, p99, max

### Threading Model
- **Network Thread**: Receives messages, deserializes, pushes to ring buffer
- **Processing Thread**: Pops from ring buffer, processes messages, updates metrics
- Non-blocking communication via lock-free queue

### Performance Optimizations
- Zero-copy message handling where possible
- Packed binary structures for minimal size
- Batch statistics printing to reduce I/O overhead
- Compiler optimizations (-O3, /O2) and native architecture targeting

## Extending the System

### Adding New Message Types
1. Define new message struct in `message_format.h`
2. Add serialization/deserialization functions
3. Update `MessageType` enum
4. Handle in feed_handler processing loop

### Custom Processing Logic
Modify the `process_messages()` function in `feed_handler.cpp`:
- Update order books
- Calculate technical indicators
- Trigger trading signals
- Persist to database

### Alternative Transports
The architecture supports easy integration of:
- Shared memory
- Multicast UDP
- WebSocket
- Unix domain sockets

## Troubleshooting

**Build fails with Boost not found**:
- Ensure Boost is installed and in CMAKE_PREFIX_PATH
- On Windows with vcpkg: `cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake`

**Low throughput**:
- Use UDP instead of TCP for higher throughput
- Enable benchmark mode to disable latency tracking
- Check CPU affinity and governor settings
- Reduce rate limiting on simulator

**High latency**:
- Check system load and background processes
- Disable power saving features
- Use dedicated cores for network threads
- Consider kernel bypass (DPDK) for production

## License

MIT License - Free for commercial and personal use.
