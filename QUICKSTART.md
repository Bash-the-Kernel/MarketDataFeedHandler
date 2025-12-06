# Quick Start Guide

## Build Complete! ✓

Your executables are ready in `build\Release\`:
- `feed_handler.exe` - Market data receiver
- `feed_simulator.exe` - Market data generator
- `unit_tests.exe` - Unit tests

## Running the System

### Step 1: Start the Feed Handler (Terminal 1)
```bash
cd build\Release
feed_handler.exe --udp --port 9999
```

### Step 2: Start the Feed Simulator (Terminal 2)
```bash
cd build\Release
feed_simulator.exe --udp --port 9999 --rate 10000
```

You should see:
- Handler: Statistics every second showing throughput and latency
- Simulator: Message count and send rate

### Benchmark Mode (Maximum Throughput)

Terminal 1:
```bash
cd build\Release
feed_handler.exe --udp --benchmark
```

Terminal 2:
```bash
cd build\Release
feed_simulator.exe --udp --rate 100000 --benchmark
```

Expected: 100K-500K+ messages/second

## Run Tests

```bash
cd build\Release
unit_tests.exe
```

## Command Options

**feed_handler.exe**:
- `--udp` or `--tcp` - Protocol (default: UDP)
- `--port <N>` - Port number (default: 9999)
- `--benchmark` - Disable latency tracking for max throughput

**feed_simulator.exe**:
- `--udp` or `--tcp` - Protocol (default: UDP)
- `--host <addr>` - Target host (default: 127.0.0.1)
- `--port <N>` - Port number (default: 9999)
- `--rate <N>` - Messages per second (default: 10000)
- `--benchmark` - Send at maximum rate

## Project Complete!

All components are working:
✓ Binary message format (32-byte trades, 40-byte quotes)
✓ Lock-free ring buffer for zero-allocation message passing
✓ Multi-threaded architecture (network + processing threads)
✓ Latency metrics (min/avg/p99/max)
✓ Throughput tracking
✓ UDP and TCP support
✓ Benchmark mode
✓ Unit tests
✓ No external dependencies (uses native Windows sockets)
