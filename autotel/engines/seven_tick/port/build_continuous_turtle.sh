#!/bin/bash

# Build script for Continuous Turtle Pipeline
# Supports various optimization levels and configurations

set -e

echo "🐢 Building CNS v8 Continuous Turtle Pipeline..."
echo "=============================================="

# Default values
WORKERS=8
DURATION=30
OPTIMIZATION="O3"
PROFILE=false
DEBUG=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --workers)
            WORKERS="$2"
            shift 2
            ;;
        --duration)
            DURATION="$2"
            shift 2
            ;;
        --debug)
            DEBUG=true
            OPTIMIZATION="O0"
            shift
            ;;
        --profile)
            PROFILE=true
            shift
            ;;
        --optimization)
            OPTIMIZATION="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --workers N      Initial worker count (default: 8)"
            echo "  --duration S     Test duration in seconds (default: 30)"
            echo "  --debug          Enable debug build"
            echo "  --profile        Enable profiling"
            echo "  --optimization L Optimization level (O0, O1, O2, O3)"
            echo "  --help           Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Set compiler flags
CFLAGS="-Wall -${OPTIMIZATION} -march=native -pthread"

if [ "$DEBUG" = true ]; then
    CFLAGS="$CFLAGS -g -DDEBUG"
    echo "🔧 Debug mode enabled"
fi

if [ "$PROFILE" = true ]; then
    CFLAGS="$CFLAGS -pg -fprofile-arcs -ftest-coverage"
    echo "📊 Profiling enabled"
fi

# Check for required tools
if ! command -v gcc &> /dev/null; then
    echo "❌ Error: gcc not found. Please install GCC."
    exit 1
fi

# Create build directory
BUILD_DIR="build_turtle"
mkdir -p "$BUILD_DIR"

echo "🔨 Compiling with flags: $CFLAGS"

# Compile object files
echo "📦 Compiling continuous_turtle_pipeline.c..."
gcc $CFLAGS -c continuous_turtle_pipeline.c -o "$BUILD_DIR/continuous_turtle_pipeline.o"

echo "📦 Compiling continuous_turtle_test.c..."
gcc $CFLAGS -c continuous_turtle_test.c -o "$BUILD_DIR/continuous_turtle_test.o"

echo "📦 Compiling dependencies..."
gcc $CFLAGS -c bitactor.c -o "$BUILD_DIR/bitactor.o"
gcc $CFLAGS -c tick_collapse_engine.c -o "$BUILD_DIR/tick_collapse_engine.o"
gcc $CFLAGS -c signal_engine.c -o "$BUILD_DIR/signal_engine.o"
gcc $CFLAGS -c bitmask_compiler.c -o "$BUILD_DIR/bitmask_compiler.o"
gcc $CFLAGS -c actuator.c -o "$BUILD_DIR/actuator.o"

# Link executable
echo "🔗 Linking continuous_turtle_test..."
gcc $CFLAGS -o continuous_turtle_test \
    "$BUILD_DIR/continuous_turtle_test.o" \
    "$BUILD_DIR/continuous_turtle_pipeline.o" \
    "$BUILD_DIR/bitactor.o" \
    "$BUILD_DIR/tick_collapse_engine.o" \
    "$BUILD_DIR/signal_engine.o" \
    "$BUILD_DIR/bitmask_compiler.o" \
    "$BUILD_DIR/actuator.o" \
    -lm -pthread

echo "✅ Build completed successfully!"
echo ""

# Run tests if requested
read -p "Run test with $WORKERS workers for $DURATION seconds? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "🚀 Starting Continuous Turtle Pipeline test..."
    echo "   Workers: $WORKERS"
    echo "   Duration: $DURATION seconds"
    echo ""
    
    ./continuous_turtle_test "$WORKERS" "$DURATION"
    
    if [ "$PROFILE" = true ]; then
        echo ""
        echo "📊 Generating profiling report..."
        gprof continuous_turtle_test gmon.out > turtle_profile.txt
        echo "Profile saved to turtle_profile.txt"
        
        if command -v gcov &> /dev/null; then
            echo "📊 Generating coverage report..."
            gcov continuous_turtle_pipeline.c
            echo "Coverage report generated"
        fi
    fi
fi

echo ""
echo "🎉 Done!"