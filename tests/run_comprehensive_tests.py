#!/usr/bin/env python3
"""
Comprehensive AutoTel Test Runner

This script provides specialized test execution for the comprehensive test suite
with different modes for development, CI/CD, and debugging.
"""

import sys
import subprocess
import argparse
import time
from pathlib import Path
from typing import List, Dict, Any

# Add the project root to the path
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

def run_pytest(args: List[str], description: str = "") -> int:
    """Run pytest with the given arguments"""
    cmd = ["pytest"] + args
    
    print(f"🚀 {description}")
    print(f"📋 Command: {' '.join(cmd)}")
    print("=" * 80)
    
    start_time = time.time()
    result = subprocess.run(cmd, cwd=project_root)
    end_time = time.time()
    
    duration = end_time - start_time
    print(f"\n⏱️  Test execution time: {duration:.2f} seconds")
    print(f"📊 Exit code: {result.returncode}")
    
    return result.returncode

def run_test_mode(mode: str, args: Dict[str, Any]) -> int:
    """Run tests in a specific mode"""
    
    if mode == "fast":
        # Fast tests only - unit tests, no external dependencies
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "unit",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running Fast Tests (Unit Tests Only)")
    
    elif mode == "integration":
        # Integration tests - medium complexity
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "integration",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running Integration Tests")
    
    elif mode == "dspy":
        # DSPy tests - LLM calls
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "dspy",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running DSPy Tests (LLM Integration)")
    
    elif mode == "slow":
        # Slow tests - performance and long-running
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "slow",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running Slow Tests (Performance & Long-running)")
    
    elif mode == "all":
        # All tests except manual
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "not manual",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running All Tests (Except Manual)")
    
    elif mode == "coverage":
        # Tests with coverage reporting
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "not manual",
            "--cov=autotel",
            "--cov-report=html",
            "--cov-report=term-missing",
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running Tests with Coverage")
    
    elif mode == "debug":
        # Debug mode - verbose output, stop on first failure
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-x",  # Stop on first failure
            "-v",  # Verbose
            "--tb=long",  # Full traceback
            "--durations=0"  # Show all durations
        ]
        
        return run_pytest(pytest_args, "Running Tests in Debug Mode")
    
    elif mode == "parallel":
        # Parallel test execution
        pytest_args = [
            "tests/test_comprehensive_suite.py",
            "-m", "not manual",
            "-n", "auto",  # Auto-detect number of workers
            "--tb=short",
            "--durations=10"
        ]
        if args.get("verbose"):
            pytest_args.append("-v")
        
        return run_pytest(pytest_args, "Running Tests in Parallel")
    
    else:
        print(f"❌ Unknown test mode: {mode}")
        return 1

def run_specific_test(test_name: str, args: Dict[str, Any]) -> int:
    """Run a specific test by name"""
    pytest_args = [
        "tests/test_comprehensive_suite.py",
        f"test_{test_name}",
        "--tb=short"
    ]
    
    if args.get("verbose"):
        pytest_args.append("-v")
    
    return run_pytest(pytest_args, f"Running Specific Test: {test_name}")

def run_test_category(category: str, args: Dict[str, Any]) -> int:
    """Run tests by category"""
    pytest_args = [
        "tests/test_comprehensive_suite.py",
        "-k", category,  # Keyword matching
        "--tb=short",
        "--durations=10"
    ]
    
    if args.get("verbose"):
        pytest_args.append("-v")
    
    return run_pytest(pytest_args, f"Running Tests Matching Category: {category}")

def list_available_tests() -> None:
    """List all available tests"""
    pytest_args = [
        "tests/test_comprehensive_suite.py",
        "--collect-only",
        "-q"
    ]
    
    print("📋 Available Tests:")
    print("=" * 50)
    
    result = subprocess.run(pytest_args, cwd=project_root, capture_output=True, text=True)
    
    if result.returncode == 0:
        lines = result.stdout.strip().split('\n')
        for line in lines:
            if line.startswith('test_'):
                print(f"  {line}")
    else:
        print("❌ Failed to collect tests")

def show_test_statistics() -> None:
    """Show test statistics"""
    pytest_args = [
        "tests/test_comprehensive_suite.py",
        "--collect-only",
        "-q"
    ]
    
    result = subprocess.run(pytest_args, cwd=project_root, capture_output=True, text=True)
    
    if result.returncode == 0:
        lines = result.stdout.strip().split('\n')
        test_count = len([line for line in lines if line.startswith('test_')])
        
        print("📊 Test Statistics:")
        print("=" * 30)
        print(f"Total tests: {test_count}")
        
        # Count by category (approximate)
        categories = {
            "unit": 0,
            "integration": 0,
            "dspy": 0,
            "slow": 0,
            "e2e": 0
        }
        
        for line in lines:
            if line.startswith('test_'):
                if "dspy" in line.lower():
                    categories["dspy"] += 1
                elif "integration" in line.lower():
                    categories["integration"] += 1
                elif "slow" in line.lower():
                    categories["slow"] += 1
                elif "e2e" in line.lower():
                    categories["e2e"] += 1
                else:
                    categories["unit"] += 1
        
        for category, count in categories.items():
            print(f"{category.capitalize()}: {count}")
    else:
        print("❌ Failed to collect test statistics")

def main():
    parser = argparse.ArgumentParser(
        description="Comprehensive AutoTel Test Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Test Modes:
  fast        - Unit tests only (fastest)
  integration - Integration tests (medium)
  dspy        - DSPy/LLM tests (slow)
  slow        - Performance tests (slowest)
  all         - All tests except manual
  coverage    - Tests with coverage reporting
  debug       - Debug mode (verbose, stop on failure)
  parallel    - Parallel test execution

Examples:
  python tests/run_comprehensive_tests.py fast
  python tests/run_comprehensive_tests.py dspy --verbose
  python tests/run_comprehensive_tests.py test framework_initialization
  python tests/run_comprehensive_tests.py category telemetry
        """
    )
    
    parser.add_argument(
        "mode",
        nargs="?",
        choices=["fast", "integration", "dspy", "slow", "all", "coverage", "debug", "parallel", "list", "stats"],
        default="fast",
        help="Test execution mode"
    )
    
    parser.add_argument(
        "test_name",
        nargs="?",
        help="Specific test name (when mode is 'test')"
    )
    
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Verbose output"
    )
    
    parser.add_argument(
        "--test",
        action="store_true",
        help="Run a specific test by name"
    )
    
    parser.add_argument(
        "--category",
        action="store_true",
        help="Run tests matching a category"
    )
    
    args = parser.parse_args()
    
    # Handle special modes
    if args.mode == "list":
        list_available_tests()
        return 0
    
    if args.mode == "stats":
        show_test_statistics()
        return 0
    
    # Handle specific test execution
    if args.test and args.test_name:
        return run_specific_test(args.test_name, vars(args))
    
    if args.category and args.test_name:
        return run_test_category(args.test_name, vars(args))
    
    # Run the specified mode
    return run_test_mode(args.mode, vars(args))

if __name__ == "__main__":
    sys.exit(main()) 