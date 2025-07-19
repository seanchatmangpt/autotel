#!/bin/bash

# Test OpenTelemetry Integration for CNS Domain Commands

echo "Testing OpenTelemetry Integration..."

# Build CNS if needed
if [ ! -f "build/cns" ]; then
    echo "Building CNS..."
    make clean && make
fi

# Set up OpenTelemetry environment (optional - for local testing)
export OTEL_SERVICE_NAME="cns-test"
export OTEL_EXPORTER_OTLP_ENDPOINT="localhost:4317"
export OTEL_TRACES_EXPORTER="otlp"
export OTEL_METRICS_EXPORTER="otlp"

echo "Running benchmark commands with telemetry..."

# Test benchmark all
echo "1. Testing 'cns bench all'"
./build/cns bench all

# Test benchmark sparql
echo "2. Testing 'cns bench sparql'"
./build/cns bench sparql

# Test benchmark memory
echo "3. Testing 'cns bench memory'"
./build/cns bench memory

# Test benchmark report
echo "4. Testing 'cns bench report'"
./build/cns bench report

# Test test commands
echo "5. Testing 'cns test all'"
./build/cns test all

echo "OpenTelemetry integration test complete!"
echo "Check your OTLP endpoint for traces and metrics."