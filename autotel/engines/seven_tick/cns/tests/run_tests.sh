#!/bin/bash

# CNS Unit Tests Runner
# Comprehensive test suite for all CNS include files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$TEST_DIR"
REPORT_DIR="$TEST_DIR/reports"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Test executables
TESTS=(
    "test_cns_core"
    "test_cns_parser"
    "test_cns_dispatch"
    "test_cns_commands"
    "test_cns_benchmark"
    "test_cns_types"
    "test_cns_cli"
)

# Test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Functions
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  CNS Unit Tests - Comprehensive Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
}

print_test_header() {
    echo -e "${YELLOW}Running: $1${NC}"
    echo -e "${YELLOW}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ PASSED: $1${NC}"
}

print_failure() {
    echo -e "${RED}✗ FAILED: $1${NC}"
}

print_skipped() {
    echo -e "${YELLOW}⚠ SKIPPED: $1${NC}"
}

print_summary() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Total Tests: ${TOTAL_TESTS}"
    echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
    echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"
    echo -e "${YELLOW}Skipped: ${SKIPPED_TESTS}${NC}"
    
    if [ $TOTAL_TESTS -gt 0 ]; then
        SUCCESS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))
        echo -e "Success Rate: ${SUCCESS_RATE}%"
    fi
    
    echo ""
}

check_dependencies() {
    echo -e "${BLUE}Checking dependencies...${NC}"
    
    # Check for gcc
    if ! command -v gcc &> /dev/null; then
        echo -e "${RED}Error: gcc not found${NC}"
        exit 1
    fi
    
    # Check for make
    if ! command -v make &> /dev/null; then
        echo -e "${RED}Error: make not found${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Dependencies OK${NC}"
    echo ""
}

build_tests() {
    echo -e "${BLUE}Building test executables...${NC}"
    
    cd "$BUILD_DIR"
    
    if ! make clean all; then
        echo -e "${RED}Build failed${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}Build completed${NC}"
    echo ""
}

run_single_test() {
    local test_name="$1"
    local test_executable="$BUILD_DIR/$test_name"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ ! -f "$test_executable" ]; then
        print_skipped "$test_name (executable not found)"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        return
    fi
    
    if [ ! -x "$test_executable" ]; then
        print_skipped "$test_name (not executable)"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
        return
    fi
    
    print_test_header "$test_name"
    
    # Run the test and capture output
    local output
    local exit_code
    
    if output=$(cd "$BUILD_DIR" && ./"$test_name" 2>&1); then
        exit_code=0
    else
        exit_code=$?
    fi
    
    # Parse test results from output
    if echo "$output" | grep -q "Success Rate: 100%"; then
        print_success "$test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    elif [ $exit_code -eq 0 ]; then
        print_success "$test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        print_failure "$test_name (exit code: $exit_code)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    # Show test output
    echo "$output"
    echo ""
}

run_all_tests() {
    echo -e "${BLUE}Running all tests...${NC}"
    echo ""
    
    for test in "${TESTS[@]}"; do
        run_single_test "$test"
    done
}

run_specific_test() {
    local test_name="$1"
    
    if [[ " ${TESTS[*]} " =~ " ${test_name} " ]]; then
        run_single_test "$test_name"
    else
        echo -e "${RED}Error: Unknown test '$test_name'${NC}"
        echo -e "Available tests: ${TESTS[*]}"
        exit 1
    fi
}

generate_report() {
    if [ ! -d "$REPORT_DIR" ]; then
        mkdir -p "$REPORT_DIR"
    fi
    
    local report_file="$REPORT_DIR/test_report_${TIMESTAMP}.txt"
    
    {
        echo "CNS Unit Tests Report"
        echo "===================="
        echo "Generated: $(date)"
        echo ""
        echo "Test Configuration:"
        echo "  Test Directory: $TEST_DIR"
        echo "  Build Directory: $BUILD_DIR"
        echo "  Report Directory: $REPORT_DIR"
        echo ""
        echo "Test Results:"
        echo "  Total Tests: $TOTAL_TESTS"
        echo "  Passed: $PASSED_TESTS"
        echo "  Failed: $FAILED_TESTS"
        echo "  Skipped: $SKIPPED_TESTS"
        
        if [ $TOTAL_TESTS -gt 0 ]; then
            SUCCESS_RATE=$((PASSED_TESTS * 100 / TOTAL_TESTS))
            echo "  Success Rate: ${SUCCESS_RATE}%"
        fi
        
        echo ""
        echo "Test Details:"
        for test in "${TESTS[@]}"; do
            local test_executable="$BUILD_DIR/$test"
            if [ -f "$test_executable" ] && [ -x "$test_executable" ]; then
                echo "  ✓ $test"
            else
                echo "  ⚠ $test (not built)"
            fi
        done
    } > "$report_file"
    
    echo -e "${BLUE}Report generated: $report_file${NC}"
}

show_help() {
    echo "CNS Unit Tests Runner"
    echo "===================="
    echo ""
    echo "Usage: $0 [OPTIONS] [TEST_NAME]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -b, --build    Build tests only (don't run)"
    echo "  -c, --clean    Clean build artifacts"
    echo "  -r, --report   Generate detailed report"
    echo "  -v, --verbose  Verbose output"
    echo ""
    echo "Test Names:"
    for test in "${TESTS[@]}"; do
        echo "  $test"
    done
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests"
    echo "  $0 test_cns_core      # Run specific test"
    echo "  $0 --build            # Build only"
    echo "  $0 --clean            # Clean build"
    echo "  $0 --report           # Generate report"
}

# Parse command line arguments
BUILD_ONLY=false
CLEAN_ONLY=false
GENERATE_REPORT=false
VERBOSE=false
SPECIFIC_TEST=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -b|--build)
            BUILD_ONLY=true
            shift
            ;;
        -c|--clean)
            CLEAN_ONLY=true
            shift
            ;;
        -r|--report)
            GENERATE_REPORT=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -*)
            echo -e "${RED}Error: Unknown option $1${NC}"
            show_help
            exit 1
            ;;
        *)
            if [ -z "$SPECIFIC_TEST" ]; then
                SPECIFIC_TEST="$1"
            else
                echo -e "${RED}Error: Multiple test names specified${NC}"
                exit 1
            fi
            shift
            ;;
    esac
done

# Main execution
main() {
    print_header
    
    check_dependencies
    
    if [ "$CLEAN_ONLY" = true ]; then
        echo -e "${BLUE}Cleaning build artifacts...${NC}"
        cd "$BUILD_DIR"
        make clean
        echo -e "${GREEN}Clean completed${NC}"
        exit 0
    fi
    
    build_tests
    
    if [ "$BUILD_ONLY" = true ]; then
        echo -e "${GREEN}Build completed successfully${NC}"
        exit 0
    fi
    
    if [ -n "$SPECIFIC_TEST" ]; then
        run_specific_test "$SPECIFIC_TEST"
    else
        run_all_tests
    fi
    
    print_summary
    
    if [ "$GENERATE_REPORT" = true ]; then
        generate_report
    fi
    
    # Exit with appropriate code
    if [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    else
        exit 0
    fi
}

# Run main function
main "$@" 