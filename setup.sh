#!/bin/bash

echo "🚀 AutoTel Prototype Setup"
echo "========================="

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is required but not found"
    exit 1
fi

echo "✅ Python 3 found: $(python3 --version)"

# Create virtual environment
echo "📦 Creating virtual environment..."
python3 -m venv autotel_env

# Activate virtual environment
echo "⚡ Activating virtual environment..."
source autotel_env/bin/activate

# Install requirements
echo "📥 Installing requirements..."
pip install --upgrade pip
pip install -r requirements.txt

echo ""
echo "✅ Setup complete!"
echo ""
echo "To run the prototype:"
echo "  source autotel_env/bin/activate"
echo "  python autotel_prototype.py"
echo ""
echo "🔥 AutoTel is ready to demonstrate zero-touch telemetry!"
