#!/bin/bash
# TTL Lexer Test Runner Script
# Comprehensive testing with performance and quality metrics

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║            TTL LEXER TEST RUNNER v1.0                          ║"
echo "║         Automated Testing with Quality Metrics                 ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running on macOS for memory profiling
if [[ "$OSTYPE" == "darwin"* ]]; then
    MEMORY_TOOL="leaks"
else
    MEMORY_TOOL="valgrind"
fi

# Function to run tests with timing
run_test() {
    local test_name=$1
    local command=$2
    
    echo -e "\n${YELLOW}Running: $test_name${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    start_time=$(date +%s.%N)
    
    if eval $command; then
        end_time=$(date +%s.%N)
        duration=$(echo "$end_time - $start_time" | bc)
        echo -e "${GREEN}✓ $test_name completed in ${duration}s${NC}"
    else
        echo -e "${RED}✗ $test_name failed${NC}"
        exit 1
    fi
}

# Change to test directory
cd "$(dirname "$0")"

# Clean previous builds
echo "🧹 Cleaning previous builds..."
make -f Makefile.ttl_lexer clean

# Build all variants
echo -e "\n🔨 Building test variants..."
make -f Makefile.ttl_lexer all
make -f Makefile.ttl_lexer debug
make -f Makefile.ttl_lexer benchmark

# Run standard tests
run_test "Standard Test Suite" "make -f Makefile.ttl_lexer test"

# Run debug tests with sanitizers
run_test "Debug Tests with Sanitizers" "make -f Makefile.ttl_lexer test-debug"

# Run performance benchmarks
run_test "Performance Benchmarks" "make -f Makefile.ttl_lexer bench"

# Run code coverage
echo -e "\n${YELLOW}Generating code coverage report...${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
make -f Makefile.ttl_lexer coverage

# Memory leak detection
echo -e "\n${YELLOW}Running memory leak detection...${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS leaks tool
    if leaks -atExit -- ./test_ttl_lexer > leaks_report.txt 2>&1; then
        echo -e "${GREEN}✓ No memory leaks detected${NC}"
    else
        echo -e "${RED}✗ Memory leaks found! Check leaks_report.txt${NC}"
    fi
else
    # Linux valgrind
    make -f Makefile.ttl_lexer memcheck
fi

# Generate performance report
echo -e "\n${YELLOW}Generating performance report...${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

./test_ttl_lexer_bench > performance_report.txt
echo "Performance metrics saved to performance_report.txt"

# Summary
echo
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║                    TEST EXECUTION COMPLETE                     ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo
echo "📊 Reports generated:"
echo "   • Test results: see console output above"
echo "   • Code coverage: *.gcov files"
echo "   • Memory analysis: leaks_report.txt (if applicable)"
echo "   • Performance metrics: performance_report.txt"
echo
echo "✨ All tests completed successfully!"