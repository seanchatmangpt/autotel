#!/bin/bash

# SPARQL 80/20 System Deployment Script
# Validates and deploys the complete SPARQL system

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SPARQL_ENGINE="libsparql.a"
TEST_BINARY="test_sparql_engine"
VALIDATION_BINARY="sparql_validation"
DEPLOY_DIR="/opt/cns/sparql"
BACKUP_DIR="/opt/cns/backup"

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check for required tools
    command -v clang >/dev/null 2>&1 || { log_error "clang is required but not installed"; exit 1; }
    command -v make >/dev/null 2>&1 || { log_error "make is required but not installed"; exit 1; }
    command -v ar >/dev/null 2>&1 || { log_error "ar is required but not installed"; exit 1; }
    
    # Check for required directories
    if [ ! -d "include/cns" ]; then
        log_error "include/cns directory not found"
        exit 1
    fi
    
    if [ ! -d "src/engines" ]; then
        log_error "src/engines directory not found"
        exit 1
    fi
    
    log_success "Prerequisites check passed"
}

# Build SPARQL system
build_sparql_system() {
    log_info "Building SPARQL system..."
    
    # Clean previous builds
    make -f Makefile.sparql_validation clean
    
    # Build all components
    make -f Makefile.sparql_validation all
    
    # Verify build artifacts
    if [ ! -f "$SPARQL_ENGINE" ]; then
        log_error "SPARQL engine library not built"
        exit 1
    fi
    
    if [ ! -f "$TEST_BINARY" ]; then
        log_error "SPARQL test binary not built"
        exit 1
    fi
    
    if [ ! -f "$VALIDATION_BINARY" ]; then
        log_error "SPARQL validation binary not built"
        exit 1
    fi
    
    log_success "SPARQL system built successfully"
}

# Run tests
run_tests() {
    log_info "Running SPARQL engine tests..."
    
    if ! ./$TEST_BINARY; then
        log_error "SPARQL engine tests failed"
        exit 1
    fi
    
    log_success "SPARQL engine tests passed"
}

# Run validation
run_validation() {
    log_info "Running SPARQL 80/20 validation..."
    
    if ! ./$VALIDATION_BINARY; then
        log_error "SPARQL 80/20 validation failed"
        exit 1
    fi
    
    log_success "SPARQL 80/20 validation passed"
}

# Performance benchmark
run_benchmark() {
    log_info "Running performance benchmark..."
    
    # Run benchmark and capture output
    benchmark_output=$(./$VALIDATION_BINARY 2>&1)
    
    # Check for success
    if echo "$benchmark_output" | grep -q "SYSTEM FULLY OPERATIONAL"; then
        log_success "Performance benchmark passed"
    else
        log_error "Performance benchmark failed"
        echo "$benchmark_output"
        exit 1
    fi
}

# Create backup
create_backup() {
    log_info "Creating backup of existing installation..."
    
    if [ -d "$DEPLOY_DIR" ]; then
        mkdir -p "$BACKUP_DIR"
        backup_name="sparql_backup_$(date +%Y%m%d_%H%M%S)"
        cp -r "$DEPLOY_DIR" "$BACKUP_DIR/$backup_name"
        log_success "Backup created: $BACKUP_DIR/$backup_name"
    else
        log_info "No existing installation found, skipping backup"
    fi
}

# Deploy system
deploy_system() {
    log_info "Deploying SPARQL system..."
    
    # Create deployment directory
    sudo mkdir -p "$DEPLOY_DIR"
    sudo mkdir -p "$DEPLOY_DIR/lib"
    sudo mkdir -p "$DEPLOY_DIR/include"
    sudo mkdir -p "$DEPLOY_DIR/bin"
    
    # Copy files
    sudo cp "$SPARQL_ENGINE" "$DEPLOY_DIR/lib/"
    sudo cp "include/cns/sparql.h" "$DEPLOY_DIR/include/"
    sudo cp "$TEST_BINARY" "$DEPLOY_DIR/bin/"
    sudo cp "$VALIDATION_BINARY" "$DEPLOY_DIR/bin/"
    
    # Set permissions
    sudo chmod 755 "$DEPLOY_DIR/bin/"*
    sudo chmod 644 "$DEPLOY_DIR/lib/"*
    sudo chmod 644 "$DEPLOY_DIR/include/"*
    
    # Create symlinks
    sudo ln -sf "$DEPLOY_DIR/lib/$SPARQL_ENGINE" "/usr/local/lib/$SPARQL_ENGINE"
    sudo ln -sf "$DEPLOY_DIR/include/sparql.h" "/usr/local/include/cns/sparql.h"
    
    log_success "SPARQL system deployed to $DEPLOY_DIR"
}

# Verify deployment
verify_deployment() {
    log_info "Verifying deployment..."
    
    # Check if files are accessible
    if [ ! -f "/usr/local/lib/$SPARQL_ENGINE" ]; then
        log_error "SPARQL engine library not accessible"
        exit 1
    fi
    
    if [ ! -f "/usr/local/include/cns/sparql.h" ]; then
        log_error "SPARQL header not accessible"
        exit 1
    fi
    
    # Test deployed binaries
    if ! "$DEPLOY_DIR/bin/$TEST_BINARY"; then
        log_error "Deployed test binary failed"
        exit 1
    fi
    
    if ! "$DEPLOY_DIR/bin/$VALIDATION_BINARY"; then
        log_error "Deployed validation binary failed"
        exit 1
    fi
    
    log_success "Deployment verification passed"
}

# Create systemd service (optional)
create_service() {
    log_info "Creating systemd service..."
    
    cat > /tmp/cns-sparql.service << EOF
[Unit]
Description=CNS SPARQL Engine Service
After=network.target

[Service]
Type=simple
User=cns
Group=cns
WorkingDirectory=$DEPLOY_DIR
ExecStart=$DEPLOY_DIR/bin/$VALIDATION_BINARY
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
    
    sudo cp /tmp/cns-sparql.service /etc/systemd/system/
    sudo systemctl daemon-reload
    
    log_success "Systemd service created (not enabled by default)"
}

# Main deployment function
main() {
    echo "🚀 SPARQL 80/20 System Deployment"
    echo "================================="
    echo "This script will validate and deploy the SPARQL system"
    echo ""
    
    # Check if running as root for deployment
    if [ "$EUID" -ne 0 ]; then
        log_warning "Not running as root. Some operations may require sudo."
    fi
    
    # Run deployment steps
    check_prerequisites
    build_sparql_system
    run_tests
    run_validation
    run_benchmark
    create_backup
    deploy_system
    verify_deployment
    create_service
    
    echo ""
    log_success "SPARQL system deployment completed successfully!"
    echo ""
    echo "Deployment Summary:"
    echo "  - Engine Library: $DEPLOY_DIR/lib/$SPARQL_ENGINE"
    echo "  - Header Files: $DEPLOY_DIR/include/"
    echo "  - Binaries: $DEPLOY_DIR/bin/"
    echo "  - System Links: /usr/local/lib/ and /usr/local/include/"
    echo "  - Service: /etc/systemd/system/cns-sparql.service"
    echo ""
    echo "To enable the service (optional):"
    echo "  sudo systemctl enable cns-sparql.service"
    echo "  sudo systemctl start cns-sparql.service"
    echo ""
    echo "To test the deployment:"
    echo "  $DEPLOY_DIR/bin/$TEST_BINARY"
    echo "  $DEPLOY_DIR/bin/$VALIDATION_BINARY"
}

# Handle command line arguments
case "${1:-deploy}" in
    "deploy")
        main
        ;;
    "test")
        check_prerequisites
        build_sparql_system
        run_tests
        run_validation
        run_benchmark
        ;;
    "build")
        check_prerequisites
        build_sparql_system
        ;;
    "clean")
        make -f Makefile.sparql_validation clean
        sudo rm -rf "$DEPLOY_DIR"
        sudo rm -f "/usr/local/lib/$SPARQL_ENGINE"
        sudo rm -f "/usr/local/include/cns/sparql.h"
        sudo rm -f "/etc/systemd/system/cns-sparql.service"
        log_success "Cleaned all SPARQL system files"
        ;;
    "help")
        echo "SPARQL 80/20 System Deployment Script"
        echo ""
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  deploy  - Full deployment (default)"
        echo "  test    - Build and test only"
        echo "  build   - Build only"
        echo "  clean   - Clean all files"
        echo "  help    - Show this help"
        ;;
    *)
        log_error "Unknown command: $1"
        echo "Use '$0 help' for usage information"
        exit 1
        ;;
esac 