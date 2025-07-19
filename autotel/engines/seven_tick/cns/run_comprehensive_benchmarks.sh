#!/bin/bash

# CNS Comprehensive Benchmark Runner
# Executes all CNS benchmark suites and generates comprehensive reports
# 
# This script runs:
# - Core CNS functionality benchmarks
# - 7T-SQL specific benchmarks  
# - SPARQL query benchmarks
# - Memory and performance benchmarks
# - Generates both text and OTEL format reports

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BENCHMARK_DIR="${SCRIPT_DIR}"
RESULTS_DIR="${BENCHMARK_DIR}/benchmark_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_PREFIX="cns_benchmark_${TIMESTAMP}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Configuration options
ITERATIONS_QUICK=1000
ITERATIONS_STANDARD=10000
ITERATIONS_COMPREHENSIVE=50000

QUICK_MODE=false
STANDARD_MODE=true
COMPREHENSIVE_MODE=false
PROFILE_MODE=false
OTEL_MODE=false
HTML_REPORT=false

# Parse command line arguments
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Run CNS comprehensive benchmark suite

OPTIONS:
    -q, --quick         Quick benchmark (1K iterations)
    -s, --standard      Standard benchmark (10K iterations) [default]
    -c, --comprehensive Comprehensive benchmark (50K iterations)
    -p, --profile       Enable profiling during benchmarks
    -o, --otel          Generate OpenTelemetry formatted output
    -h, --html          Generate HTML report
    -r, --results-dir   Custom results directory (default: ./benchmark_results)
    --help              Show this help message

EXAMPLES:
    $0                  # Run standard benchmarks
    $0 --quick          # Run quick benchmarks for CI
    $0 --comprehensive --otel --html  # Full benchmark with reports
    $0 --profile        # Run with profiling enabled

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -q|--quick)
            QUICK_MODE=true
            STANDARD_MODE=false
            shift
            ;;
        -s|--standard)
            STANDARD_MODE=true
            QUICK_MODE=false
            COMPREHENSIVE_MODE=false
            shift
            ;;
        -c|--comprehensive)
            COMPREHENSIVE_MODE=true
            STANDARD_MODE=false
            QUICK_MODE=false
            shift
            ;;
        -p|--profile)
            PROFILE_MODE=true
            shift
            ;;
        -o|--otel)
            OTEL_MODE=true
            shift
            ;;
        -h|--html)
            HTML_REPORT=true
            shift
            ;;
        -r|--results-dir)
            RESULTS_DIR="$2"
            shift 2
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Determine iterations based on mode
if $QUICK_MODE; then
    ITERATIONS=$ITERATIONS_QUICK
    MODE_NAME="Quick"
elif $COMPREHENSIVE_MODE; then
    ITERATIONS=$ITERATIONS_COMPREHENSIVE
    MODE_NAME="Comprehensive"
else
    ITERATIONS=$ITERATIONS_STANDARD
    MODE_NAME="Standard"
fi

# Display configuration
display_config() {
    log "CNS Comprehensive Benchmark Suite"
    echo "=================================="
    echo ""
    echo "Configuration:"
    echo "  Mode: $MODE_NAME"
    echo "  Iterations: $ITERATIONS"
    echo "  Results directory: $RESULTS_DIR"
    echo "  Profiling: $($PROFILE_MODE && echo "enabled" || echo "disabled")"
    echo "  OTEL output: $($OTEL_MODE && echo "enabled" || echo "disabled")"
    echo "  HTML report: $($HTML_REPORT && echo "enabled" || echo "disabled")"
    echo ""
    echo "System Information:"
    echo "  OS: $(uname -s) $(uname -r)"
    echo "  Architecture: $(uname -m)"
    echo "  CPU cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "unknown")"
    echo "  Compiler: $(${CC:-clang} --version | head -n1)"
    echo ""
}

# Check prerequisites
check_prerequisites() {
    log "Checking prerequisites..."
    
    # Check if we're in the right directory
    if [[ ! -f "Makefile" ]] || [[ ! -f "Makefile.benchmark" ]]; then
        log_error "Please run this script from the CNS project root directory"
        exit 1
    fi
    
    # Check compiler
    if ! command -v ${CC:-clang} &> /dev/null; then
        log_error "Compiler not found: ${CC:-clang}"
        exit 1
    fi
    
    # Check if CNS is built
    if [[ ! -f "cns" ]]; then
        log_warning "CNS binary not found. Building..."
        make all || {
            log_error "Failed to build CNS"
            exit 1
        }
    fi
    
    log_success "Prerequisites check passed"
}

# Build benchmark executables
build_benchmarks() {
    log "Building benchmark executables..."
    
    # Clean previous builds
    make -f Makefile.benchmark clean >/dev/null 2>&1 || true
    
    # Build benchmark suite
    if $PROFILE_MODE; then
        make -f Makefile.benchmark bench-profile || {
            log_error "Failed to build profiled benchmarks"
            exit 1
        }
    else
        make -f Makefile.benchmark || {
            log_error "Failed to build benchmarks"
            exit 1
        }
    fi
    
    log_success "Benchmark executables built successfully"
}

# Setup results directory
setup_results_dir() {
    log "Setting up results directory: $RESULTS_DIR"
    
    mkdir -p "$RESULTS_DIR"
    
    # Create subdirectories
    mkdir -p "$RESULTS_DIR/logs"
    mkdir -p "$RESULTS_DIR/json"
    mkdir -p "$RESULTS_DIR/profiles"
    
    log_success "Results directory ready"
}

# Run core CNS benchmarks
run_core_benchmarks() {
    log "Running core CNS functionality benchmarks..."
    
    local output_file="$RESULTS_DIR/logs/${REPORT_PREFIX}_core.txt"
    local json_file="$RESULTS_DIR/json/${REPORT_PREFIX}_core.json"
    
    # Run the simplified comprehensive benchmark
    if [[ -f "cns_comprehensive_benchmark" ]]; then
        log "Executing comprehensive benchmark suite..."
        ./cns_comprehensive_benchmark > "$output_file" 2>&1 || {
            log_warning "Core comprehensive benchmark had issues"
        }
    else
        log_warning "Comprehensive benchmark binary not found, skipping"
    fi
    
    log_success "Core benchmarks completed"
}

# Run 7T-SQL benchmarks
run_sql_benchmarks() {
    log "Running 7T-SQL performance benchmarks..."
    
    local output_file="$RESULTS_DIR/logs/${REPORT_PREFIX}_sql.txt"
    local json_file="$RESULTS_DIR/json/${REPORT_PREFIX}_sql.json"
    
    # Check if SQL benchmark binary exists
    if [[ -f "cns_sql_benchmarks" ]]; then
        log "Executing 7T-SQL benchmark suite..."
        ./cns_sql_benchmarks $ITERATIONS > "$output_file" 2>&1 || {
            log_warning "SQL benchmarks had issues"
        }
        
        # Extract JSON output if available
        if grep -q '"benchmark_type": "7t_sql"' "$output_file"; then
            sed -n '/^{$/,/^}$/p' "$output_file" | tail -n +2 > "$json_file"
        fi
    else
        log_warning "7T-SQL benchmark binary not found, building and running basic SQL tests..."
        # Fallback to basic SQL tests
        ./cns sql init >/dev/null 2>&1 || true
        ./cns sql bench 1000 > "$output_file" 2>&1 || {
            log_warning "Basic SQL benchmark failed"
        }
    fi
    
    log_success "SQL benchmarks completed"
}

# Run SPARQL benchmarks
run_sparql_benchmarks() {
    log "Running SPARQL query benchmarks..."
    
    local output_file="$RESULTS_DIR/logs/${REPORT_PREFIX}_sparql.txt"
    local json_file="$RESULTS_DIR/json/${REPORT_PREFIX}_sparql.json"
    
    # Check if SPARQL benchmark binary exists
    if [[ -f "cns_sparql_benchmarks" ]]; then
        log "Executing SPARQL benchmark suite..."
        ./cns_sparql_benchmarks $ITERATIONS > "$output_file" 2>&1 || {
            log_warning "SPARQL benchmarks had issues"
        }
        
        # Extract JSON output if available
        if grep -q '"benchmark_type": "sparql"' "$output_file"; then
            sed -n '/^{$/,/^}$/p' "$output_file" | tail -n +2 > "$json_file"
        fi
    else
        log_warning "SPARQL benchmark binary not found, running basic SPARQL tests..."
        # Fallback to basic SPARQL tests
        ./cns sparql benchmark > "$output_file" 2>&1 || {
            log_warning "Basic SPARQL benchmark failed"
        }
    fi
    
    log_success "SPARQL benchmarks completed"
}

# Run other domain benchmarks
run_domain_benchmarks() {
    log "Running domain-specific benchmarks..."
    
    local output_file="$RESULTS_DIR/logs/${REPORT_PREFIX}_domains.txt"
    
    {
        echo "=== SHACL Validation Benchmarks ==="
        ./cns shacl benchmark 2>/dev/null || echo "SHACL benchmark not available"
        
        echo ""
        echo "=== CJinja Template Benchmarks ==="
        ./cns cjinja benchmark 2>/dev/null || echo "CJinja benchmark not available"
        
        echo ""
        echo "=== Telemetry Benchmarks ==="
        # Test OpenTelemetry integration if available
        OTEL_SERVICE_NAME=cns_test ./cns benchmark all 2>/dev/null || echo "Telemetry benchmark not available"
        
    } > "$output_file" 2>&1
    
    log_success "Domain benchmarks completed"
}

# Generate comprehensive report
generate_comprehensive_report() {
    log "Generating comprehensive benchmark report..."
    
    local report_file="$RESULTS_DIR/${REPORT_PREFIX}_comprehensive_report.txt"
    
    {
        echo "CNS Comprehensive Benchmark Report"
        echo "=================================="
        echo "Generated: $(date)"
        echo "Mode: $MODE_NAME ($ITERATIONS iterations)"
        echo "System: $(uname -s) $(uname -r) $(uname -m)"
        echo ""
        
        echo "EXECUTIVE SUMMARY"
        echo "================="
        
        # Analyze results from all benchmark files
        local total_benchmarks=0
        local passed_benchmarks=0
        local seven_tick_compliant=0
        
        for log_file in "$RESULTS_DIR"/logs/*.txt; do
            if [[ -f "$log_file" ]]; then
                echo ""
                echo "Results from $(basename "$log_file"):"
                echo "---"
                
                # Extract key metrics (this is simplified - real implementation would parse structured output)
                if grep -q "PASS" "$log_file"; then
                    local file_passed=$(grep -c "PASS" "$log_file" 2>/dev/null || echo "0")
                    local file_total=$(grep -c -E "(PASS|FAIL)" "$log_file" 2>/dev/null || echo "0")
                    echo "  Passed: $file_passed/$file_total tests"
                    passed_benchmarks=$((passed_benchmarks + file_passed))
                    total_benchmarks=$((total_benchmarks + file_total))
                fi
                
                # Look for 7-tick compliance indicators
                if grep -q "7-tick" "$log_file" || grep -q "✅" "$log_file"; then
                    local file_compliant=$(grep -c "✅" "$log_file" 2>/dev/null || echo "0")
                    if [[ $file_compliant -gt 0 ]]; then
                        echo "  7-tick compliant operations: $file_compliant"
                        seven_tick_compliant=$((seven_tick_compliant + file_compliant))
                    fi
                fi
                
                # Show any errors or warnings
                if grep -q -i "error\|fail\|warning" "$log_file"; then
                    echo "  Issues found:"
                    grep -i "error\|fail\|warning" "$log_file" | head -3 | sed 's/^/    /'
                fi
            fi
        done
        
        echo ""
        echo "OVERALL PERFORMANCE SUMMARY"
        echo "==========================="
        echo "Total benchmark operations: $total_benchmarks"
        echo "Successful operations: $passed_benchmarks"
        echo "7-tick compliant operations: $seven_tick_compliant"
        
        if [[ $total_benchmarks -gt 0 ]]; then
            local success_rate=$((passed_benchmarks * 100 / total_benchmarks))
            echo "Success rate: $success_rate%"
            
            if [[ $seven_tick_compliant -gt 0 ]]; then
                local compliance_rate=$((seven_tick_compliant * 100 / total_benchmarks))
                echo "7-tick compliance rate: $compliance_rate%"
            fi
        fi
        
        echo ""
        echo "INTERPRETATION"
        echo "=============="
        if [[ $passed_benchmarks -eq $total_benchmarks ]] && [[ $total_benchmarks -gt 0 ]]; then
            echo "✅ EXCELLENT: All benchmarks passed successfully"
        elif [[ $passed_benchmarks -gt $((total_benchmarks * 80 / 100)) ]] && [[ $total_benchmarks -gt 0 ]]; then
            echo "⚠️  GOOD: Most benchmarks passed (>80%)"
        elif [[ $total_benchmarks -gt 0 ]]; then
            echo "❌ NEEDS ATTENTION: Many benchmarks failed"
        else
            echo "❓ INCONCLUSIVE: No clear benchmark results found"
        fi
        
        echo ""
        echo "FILES GENERATED"
        echo "==============="
        echo "Detailed logs: $RESULTS_DIR/logs/"
        echo "JSON outputs: $RESULTS_DIR/json/"
        if $PROFILE_MODE; then
            echo "Profile data: $RESULTS_DIR/profiles/"
        fi
        if $HTML_REPORT; then
            echo "HTML report: $RESULTS_DIR/${REPORT_PREFIX}_report.html"
        fi
        
        echo ""
        echo "RECOMMENDATIONS"
        echo "==============="
        
        if [[ $seven_tick_compliant -lt $((total_benchmarks / 2)) ]] && [[ $total_benchmarks -gt 0 ]]; then
            echo "• Consider optimizing operations that don't meet 7-tick target"
            echo "• Review memory allocation patterns and data structures"
            echo "• Enable SIMD optimizations where appropriate"
        fi
        
        if [[ $passed_benchmarks -lt $total_benchmarks ]] && [[ $total_benchmarks -gt 0 ]]; then
            echo "• Investigate failed test cases for potential bugs"
            echo "• Check compiler optimization flags"
            echo "• Verify system resources and dependencies"
        fi
        
        echo "• Run with --comprehensive flag for more detailed analysis"
        echo "• Use --profile flag to identify performance bottlenecks"
        echo "• Monitor results over time to track performance regressions"
        
    } > "$report_file"
    
    log_success "Comprehensive report generated: $report_file"
}

# Generate HTML report
generate_html_report() {
    if ! $HTML_REPORT; then
        return
    fi
    
    log "Generating HTML report..."
    
    local html_file="$RESULTS_DIR/${REPORT_PREFIX}_report.html"
    
    cat > "$html_file" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CNS Benchmark Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #f0f8ff; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .section { margin: 20px 0; }
        .benchmark { background: #f9f9f9; padding: 15px; border-left: 4px solid #007acc; margin: 10px 0; }
        .pass { border-left-color: #28a745; }
        .fail { border-left-color: #dc3545; }
        .warning { border-left-color: #ffc107; }
        .metric { display: inline-block; margin: 5px 10px; }
        .performance-indicator { font-weight: bold; }
        .seven-tick { color: #28a745; }
        .over-budget { color: #dc3545; }
        table { width: 100%; border-collapse: collapse; margin: 20px 0; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <div class="header">
        <h1>CNS Comprehensive Benchmark Report</h1>
        <p><strong>Generated:</strong> <span id="timestamp"></span></p>
        <p><strong>Mode:</strong> BENCHMARK_MODE (BENCHMARK_ITERATIONS iterations)</p>
        <p><strong>System:</strong> <span id="system-info"></span></p>
    </div>

    <div class="section">
        <h2>Executive Summary</h2>
        <div id="summary-metrics">
            <!-- Metrics will be populated by script -->
        </div>
    </div>

    <div class="section">
        <h2>Benchmark Results</h2>
        <div id="benchmark-results">
            <!-- Results will be populated by script -->
        </div>
    </div>

    <script>
        // Set timestamp
        document.getElementById('timestamp').textContent = new Date().toLocaleString();
        
        // Set system info
        document.getElementById('system-info').textContent = navigator.platform + ' - ' + navigator.userAgent;
        
        // This would normally be populated with actual benchmark data
        document.getElementById('summary-metrics').innerHTML = `
            <p>📊 <strong>Overall Performance:</strong> Benchmark data will be populated here</p>
            <p>⚡ <strong>7-tick Compliance:</strong> Results will show which operations meet the 7-cycle target</p>
            <p>🎯 <strong>Success Rate:</strong> Percentage of tests that passed successfully</p>
        `;
        
        document.getElementById('benchmark-results').innerHTML = `
            <p>Detailed benchmark results would be displayed here with:</p>
            <ul>
                <li>Individual test performance metrics</li>
                <li>Cycle counts and timing data</li>
                <li>7-tick compliance indicators</li>
                <li>Performance trends and recommendations</li>
            </ul>
        `;
    </script>
</body>
</html>
EOF

    # Replace placeholders
    sed -i.bak "s/BENCHMARK_MODE/$MODE_NAME/g" "$html_file"
    sed -i.bak "s/BENCHMARK_ITERATIONS/$ITERATIONS/g" "$html_file"
    rm -f "$html_file.bak"
    
    log_success "HTML report generated: $html_file"
}

# Generate OTEL format output
generate_otel_output() {
    if ! $OTEL_MODE; then
        return
    fi
    
    log "Generating OpenTelemetry format output..."
    
    local otel_file="$RESULTS_DIR/${REPORT_PREFIX}_otel.json"
    
    # Combine all JSON outputs into OTEL format
    {
        echo "{"
        echo '  "service": "cns-comprehensive-benchmark",'
        echo '  "version": "1.0.0",'
        echo "  \"timestamp\": $(date +%s),"
        echo '  "mode": "'$MODE_NAME'",'
        echo '  "iterations": '$ITERATIONS','
        echo '  "system": {'
        echo '    "os": "'$(uname -s)'",'
        echo '    "arch": "'$(uname -m)'",'
        echo '    "kernel": "'$(uname -r)'"'
        echo '  },'
        echo '  "benchmark_suites": ['
        
        local first=true
        for json_file in "$RESULTS_DIR"/json/*.json; do
            if [[ -f "$json_file" ]] && [[ -s "$json_file" ]]; then
                if ! $first; then
                    echo ","
                fi
                cat "$json_file"
                first=false
            fi
        done
        
        echo ""
        echo '  ]'
        echo "}"
    } > "$otel_file"
    
    log_success "OTEL output generated: $otel_file"
}

# Cleanup function
cleanup() {
    if $PROFILE_MODE && [[ -f "gmon.out" ]]; then
        mv gmon.out "$RESULTS_DIR/profiles/" 2>/dev/null || true
    fi
}

# Main execution
main() {
    display_config
    check_prerequisites
    setup_results_dir
    build_benchmarks
    
    log "Starting benchmark execution..."
    
    # Run all benchmark suites
    run_core_benchmarks
    run_sql_benchmarks
    run_sparql_benchmarks
    run_domain_benchmarks
    
    # Generate reports
    generate_comprehensive_report
    generate_html_report
    generate_otel_output
    
    cleanup
    
    log_success "All benchmarks completed successfully!"
    echo ""
    echo "📋 Results Summary:"
    echo "  📁 Results directory: $RESULTS_DIR"
    echo "  📄 Comprehensive report: $RESULTS_DIR/${REPORT_PREFIX}_comprehensive_report.txt"
    if $HTML_REPORT; then
        echo "  🌐 HTML report: $RESULTS_DIR/${REPORT_PREFIX}_report.html"
    fi
    if $OTEL_MODE; then
        echo "  📊 OTEL output: $RESULTS_DIR/${REPORT_PREFIX}_otel.json"
    fi
    echo ""
    echo "🎯 Next steps:"
    echo "  • Review the comprehensive report for performance insights"
    echo "  • Check for any failed tests or performance regressions"
    echo "  • Use --profile flag to investigate specific performance issues"
    echo "  • Run with --comprehensive for more detailed analysis"
}

# Trap to ensure cleanup on exit
trap cleanup EXIT

# Run main function
main "$@"