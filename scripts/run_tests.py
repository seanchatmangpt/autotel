#!/usr/bin/env python3
"""
AutoTel Test Runner

Easy commands for running different types of tests with pytest.
"""

import sys
import subprocess
import argparse
from pathlib import Path


def run_pytest(args):
    """Run pytest with the given arguments"""
    cmd = ["pytest"] + args
    print(f"🚀 Running: {' '.join(cmd)}")
    print("=" * 60)
    
    result = subprocess.run(cmd, cwd=Path(__file__).parent)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description="AutoTel Test Runner")
    parser.add_argument(
        "test_type",
        choices=["fast", "all", "dspy", "integration", "slow", "manual"],
        help="Type of tests to run"
    )
    parser.add_argument(
        "--file", "-f",
        help="Specific test file to run"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Verbose output"
    )
    
    args = parser.parse_args()
    
    # Build pytest arguments
    pytest_args = []
    
    if args.verbose:
        pytest_args.append("-v")
    
    if args.file:
        pytest_args.append(args.file)
    
    # Add test type specific arguments
    if args.test_type == "fast":
        pytest_args.extend(["-m", "not slow and not integration and not dspy"])
        print("🏃 Running fast tests only (unit tests)")
        
    elif args.test_type == "all":
        pytest_args.extend(["-m", "not skip"])
        print("🎯 Running all tests (except manual)")
        
    elif args.test_type == "dspy":
        pytest_args.extend(["-m", "dspy"])
        print("🤖 Running DSPy/LLM tests")
        
    elif args.test_type == "integration":
        pytest_args.extend(["-m", "integration"])
        print("🔗 Running integration tests")
        
    elif args.test_type == "slow":
        pytest_args.extend(["-m", "slow"])
        print("🐌 Running slow tests")
        
    elif args.test_type == "manual":
        pytest_args.append("--runxfail")
        print("👤 Running manual tests")
    
    # Run the tests
    return run_pytest(pytest_args)


if __name__ == "__main__":
    sys.exit(main()) 