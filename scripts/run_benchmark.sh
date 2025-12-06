#!/bin/bash
echo "Starting benchmark mode..."
echo ""
./build/feed_handler --udp --benchmark &
HANDLER_PID=$!
sleep 2
./build/feed_simulator --udp --rate 100000 --benchmark &
SIMULATOR_PID=$!

echo ""
echo "Both processes started. Press Ctrl+C to stop."
trap "kill $HANDLER_PID $SIMULATOR_PID 2>/dev/null" EXIT
wait
