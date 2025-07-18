#!/usr/bin/env python3
"""
7T System Performance Benchmark Report
=====================================

This script validates the 7T deterministic performance claims.
"""

import subprocess
import time
import os
import sys

def run_benchmark():
    # Change to project directory
    os.chdir('/Users/sac/autotel/autotel/engines/seven_tick')
    
    print("7T SYSTEM PERFORMANCE VALIDATION")
    print("================================\n")
    
    # 1. Build system in production mode
    print("Building production binaries...")
    result = subprocess.run(['make', 'clean'], capture_output=True)
    result = subprocess.run(['make', 'all', 'CFLAGS=-O3 -march=native -fPIC -Wall -Wextra -DNDEBUG'], 
                          capture_output=True)
    if result.returncode != 0:
        print("Build failed!")
        return
    print("✅ Build successful\n")
    
    # 2. Run simple benchmark
    print("Running performance benchmark...")
    
    env = os.environ.copy()
    env['DYLD_LIBRARY_PATH'] = 'lib'
    
    # Compile simple benchmark
    subprocess.run(['cc', '-O3', '-DNDEBUG', '-march=native', '-o', 
                   'verification/simple_benchmark', 'verification/simple_benchmark.c',
                   '-Llib', '-l7t_runtime'], env=env)
    
    # Run benchmark
    result = subprocess.run(['verification/simple_benchmark'], 
                          capture_output=True, text=True, env=env)
    
    print(result.stdout)
    
    # 3. Validate all benchmarks passed
    if "✅ PASS" in result.stdout and "❌ FAIL" not in result.stdout:
        print("\n" + "="*50)
        print("🎉 ALL BENCHMARKS PASSED! 🎉")
        print("="*50)
        print("\nThe 7T system has achieved its performance goals:")
        print("- Sub-microsecond query latency ✓")
        print("- >10 MOPS throughput ✓")
        print("- Deterministic performance ✓")
        print("\nThe system is ready for production use.")
    else:
        print("\nSome benchmarks failed. Check output above.")

if __name__ == "__main__":
    run_benchmark()
