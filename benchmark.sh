#!/bin/bash

echo "=== SIMPLE-SHELL PERFORMANCE VALIDATION (US-13) ==="
echo "Date: $(date)"
echo "Environment: macOS / Linux (GCC $(gcc --version | head -n1))"
echo "--------------------------------------------------"

# 1. Benchmark Startup Latency
echo "Running Startup Latency Test (100 iterations)..."
start_time=$(python3 -c 'import time; print(time.time())')
for i in {1..100}; do
    ./simple-shell <<EOF > /dev/null 2>&1
exit
EOF
done
end_time=$(python3 -c 'import time; print(time.time())')
total_startup=$(python3 -c "print(f'{(($end_time - $start_time) / 100) * 1000:.2f}')")
echo "Average Startup Latency: ${total_startup} ms per invocation"
echo ""

# 2. Benchmark Command-Response Latency (Built-ins & External commands)
echo "Running Command-Response Latency Test..."
# You can time a sequence of commands piped into your shell
time ./simple-shell <<EOF > /dev/null 2>&1
echo "Hello from benchmark"
pwd
true
exit
EOF

echo "--------------------------------------------------"
echo "Performance validation completed successfully!"