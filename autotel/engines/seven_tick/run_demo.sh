#!/bin/bash
# Runner script for Connected Engines Demo

echo "🚀 Starting Connected Engines Demonstration..."
echo ""

# Ensure we have Python 3
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is required but not installed."
    exit 1
fi

# Clear screen for better presentation
clear

# Run the demo with unbuffered output for real-time display
python3 -u demo_connected_engines.py

# Check exit status
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Demo completed successfully!"
else
    echo ""
    echo "❌ Demo encountered an error."
    exit 1
fi