#!/bin/bash
# Test CNS implementation

echo "=== CNS Implementation Test Suite ==="
echo

# Build CNS
echo "Building CNS..."
cd /Users/sac/autotel/autotel/engines/seven_tick/cns
make clean
make

if [ ! -f ./cns ]; then
    echo "Build failed!"
    exit 1
fi

echo
echo "=== Testing CNS Commands ==="
echo

# Test basic commands
echo "1. Testing help command:"
./cns help

echo
echo "2. Testing cognitive commands:"
./cns think -h
./cns learn -h
./cns adapt -h

echo
echo "3. Testing SPARQL commands:"
./cns sparql ask "Pattern"
./cns sparql select

echo
echo "4. Testing SHACL commands:"
./cns shacl report

echo
echo "5. Testing C-Jinja commands:"
./cns cjinja filter

echo
echo "6. Testing telemetry commands:"
./cns telemetry status

echo
echo "7. Testing benchmark:"
./cns benchmark all

echo
echo "=== CNS Test Complete ==="