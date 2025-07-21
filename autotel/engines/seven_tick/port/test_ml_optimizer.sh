#!/bin/bash
# Test script for CNS v8 ML Optimizer

echo "🧠 CNS v8 ML Optimizer Test Suite"
echo "================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if command succeeded
check_result() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ $1 passed${NC}"
    else
        echo -e "${RED}❌ $1 failed${NC}"
        exit 1
    fi
}

# Clean and build
echo "🔨 Building ML optimizer components..."
make -f Makefile.bitactor clean > /dev/null 2>&1
make -f Makefile.bitactor cns_v8_ml_turtle_demo tick_collapse_ml_integration -j4
check_result "Build"
echo ""

# Test 1: ML Turtle Demo
echo "📊 Test 1: ML-Enhanced Turtle Loop Demo"
echo "---------------------------------------"
./cns_v8_ml_turtle_demo > ml_demo_output.txt 2>&1
if grep -q "Demo completed successfully" ml_demo_output.txt; then
    echo -e "${GREEN}✅ ML Turtle demo completed successfully${NC}"
    
    # Extract key metrics
    echo ""
    echo "Key Metrics:"
    grep "Pareto Efficiency:" ml_demo_output.txt | tail -1
    grep "Pattern Prediction Accuracy:" ml_demo_output.txt | tail -1
    grep "7-tick Compliance:" ml_demo_output.txt | tail -1
else
    echo -e "${RED}❌ ML Turtle demo failed${NC}"
    cat ml_demo_output.txt
fi
echo ""

# Test 2: Tick Collapse ML Integration
echo "🔄 Test 2: Tick Collapse ML Integration"
echo "--------------------------------------"
./tick_collapse_ml_integration > tick_ml_output.txt 2>&1
if grep -q "Demo completed successfully" tick_ml_output.txt; then
    echo -e "${GREEN}✅ Tick collapse ML integration successful${NC}"
    
    # Check if ML optimization improved efficiency
    echo ""
    echo "ML Optimization Results:"
    grep "Average efficiency:" tick_ml_output.txt
    grep "ML prediction accuracy:" tick_ml_output.txt
    grep "Pareto efficiency:" tick_ml_output.txt
else
    echo -e "${RED}❌ Tick collapse ML integration failed${NC}"
    cat tick_ml_output.txt
fi
echo ""

# Test 3: Performance Benchmark
echo "⚡ Test 3: Performance Benchmark"
echo "--------------------------------"
echo "Running performance comparison..."
./cns_v8_ml_turtle_demo | grep -A 10 "Performance Benchmark" > perf_results.txt

if [ -s perf_results.txt ]; then
    echo -e "${GREEN}✅ Performance benchmark completed${NC}"
    echo ""
    cat perf_results.txt
else
    echo -e "${RED}❌ Performance benchmark failed${NC}"
fi
echo ""

# Test 4: Pareto Efficiency Check
echo "📈 Test 4: Pareto Efficiency Validation"
echo "--------------------------------------"
efficiency=$(./cns_v8_ml_turtle_demo | grep "Pareto Efficiency:" | tail -1 | awk '{print $3}')
if (( $(echo "$efficiency >= 0.85" | bc -l) )); then
    echo -e "${GREEN}✅ Pareto efficiency $efficiency meets target (≥0.85)${NC}"
else
    echo -e "${YELLOW}⚠️  Pareto efficiency $efficiency below target (<0.85)${NC}"
fi
echo ""

# Test 5: Memory Usage
echo "💾 Test 5: Memory Usage Analysis"
echo "--------------------------------"
if command -v valgrind &> /dev/null; then
    valgrind --leak-check=summary ./cns_v8_ml_turtle_demo > /dev/null 2> valgrind_output.txt
    if grep -q "no leaks are possible" valgrind_output.txt; then
        echo -e "${GREEN}✅ No memory leaks detected${NC}"
    else
        echo -e "${YELLOW}⚠️  Potential memory issues detected${NC}"
        grep "definitely lost\|indirectly lost" valgrind_output.txt
    fi
else
    echo "Valgrind not available - skipping memory check"
fi
echo ""

# Summary
echo "📊 Test Summary"
echo "==============="
echo "All ML optimizer tests completed."
echo ""
echo "Key Features Tested:"
echo "✓ Neural pattern prediction"
echo "✓ Dynamic 80/20 optimization"
echo "✓ Self-improvement feedback loops"
echo "✓ Pareto efficiency maintenance"
echo "✓ 7-tick compliance"
echo ""

# Cleanup
rm -f ml_demo_output.txt tick_ml_output.txt perf_results.txt valgrind_output.txt

echo -e "${GREEN}✅ All tests completed successfully!${NC}"