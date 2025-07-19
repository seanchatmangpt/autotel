#!/usr/bin/env python3
"""
CNS Weaver Validation Script
Tests the weaver CLI functionality comprehensively.
"""

import subprocess
import json
import os
import sys
from pathlib import Path

def run_command(cmd, description):
    """Run a command and return success status."""
    print(f"🔍 {description}")
    print(f"   Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if result.returncode == 0:
            print(f"   ✅ Success")
            if result.stdout.strip():
                print(f"   Output: {result.stdout.strip()}")
            return True
        else:
            print(f"   ❌ Failed (exit code: {result.returncode})")
            if result.stderr.strip():
                print(f"   Error: {result.stderr.strip()}")
            return False
    except subprocess.TimeoutExpired:
        print(f"   ❌ Timeout")
        return False
    except Exception as e:
        print(f"   ❌ Exception: {e}")
        return False

def validate_file_exists(filepath, description):
    """Validate that a file exists."""
    if os.path.exists(filepath):
        size = os.path.getsize(filepath)
        print(f"   ✅ {description}: {filepath} ({size} bytes)")
        return True
    else:
        print(f"   ❌ {description}: {filepath} (not found)")
        return False

def validate_json_structure(json_path):
    """Validate JSON structure."""
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        required_keys = ['spans', 'functions', 'patterns', 'metadata', 'functionSignatures']
        for key in required_keys:
            if key not in data:
                print(f"   ❌ Missing required key: {key}")
                return False
        
        print(f"   ✅ JSON structure valid")
        print(f"   📊 Spans: {len(data['spans'])}")
        print(f"   📊 Functions: {len(data['functions'])}")
        print(f"   📊 Patterns: {len(data['patterns'])}")
        return True
    except Exception as e:
        print(f"   ❌ JSON validation failed: {e}")
        return False

def main():
    """Main validation function."""
    print("🧪 CNS Weaver CLI Validation")
    print("=" * 50)
    
    # Change to CNS directory
    cns_dir = Path(__file__).parent.parent
    os.chdir(cns_dir)
    
    tests_passed = 0
    tests_total = 0
    
    # Test 1: Help messages
    print("\n📋 Test 1: Help Messages")
    print("-" * 30)
    
    tests_total += 1
    if run_command(['python3', 'codegen/extract_spans.py'], "Extract spans help"):
        tests_passed += 1
    
    tests_total += 1
    if run_command(['python3', 'codegen/weaver_simple.py'], "Weaver help"):
        tests_passed += 1
    
    # Test 2: Error handling
    print("\n📋 Test 2: Error Handling")
    print("-" * 30)
    
    tests_total += 1
    if run_command(['python3', 'codegen/extract_spans.py', 'nonexistent.ttl', 'build/test.json'], "Invalid TTL file"):
        print("   ⚠️  Expected failure - this is correct behavior")
    else:
        tests_passed += 1
    
    tests_total += 1
    if run_command(['python3', 'codegen/weaver_simple.py', 'nonexistent.json', 'templates/otel_header.h.j2', 'templates/otel_inject.c.j2', 'src', 'cns.h'], "Invalid JSON file"):
        print("   ⚠️  Expected failure - this is correct behavior")
    else:
        tests_passed += 1
    
    # Test 3: Span extraction
    print("\n📋 Test 3: Span Extraction")
    print("-" * 30)
    
    tests_total += 1
    if run_command(['python3', 'codegen/extract_spans.py', 'docs/ontology/cns-core.ttl', 'build/cns_spans.json'], "Extract spans from TTL"):
        tests_passed += 1
    
    tests_total += 1
    if validate_file_exists('build/cns_spans.json', "Generated JSON file"):
        tests_passed += 1
    
    tests_total += 1
    if validate_json_structure('build/cns_spans.json'):
        tests_passed += 1
    
    # Test 4: Weaver generation
    print("\n📋 Test 4: Weaver Generation")
    print("-" * 30)
    
    tests_total += 1
    if run_command(['python3', 'codegen/weaver_simple.py', 'build/cns_spans.json', 'templates/otel_header.h.j2', 'templates/otel_inject.c.j2', 'src', 'cns.h'], "Generate OpenTelemetry code"):
        tests_passed += 1
    
    tests_total += 1
    if validate_file_exists('src/cns_otel.h', "Generated header file"):
        tests_passed += 1
    
    tests_total += 1
    if validate_file_exists('src/cns_otel_inject.c', "Generated inject file"):
        tests_passed += 1
    
    # Test 5: Generated code validation
    print("\n📋 Test 5: Generated Code Validation")
    print("-" * 30)
    
    # Check header file content
    tests_total += 1
    try:
        with open('src/cns_otel.h', 'r') as f:
            content = f.read()
            if 'CNS_OTEL_H' in content and 'opentelemetry_span_t' in content:
                print("   ✅ Header file contains expected content")
                tests_passed += 1
            else:
                print("   ❌ Header file missing expected content")
    except Exception as e:
        print(f"   ❌ Header file validation failed: {e}")
    
    # Check inject file content
    tests_total += 1
    try:
        with open('src/cns_otel_inject.c', 'r') as f:
            content = f.read()
            if 'cns_otel_inject_init' in content and 'cns_gatekeeper_validate_performance' in content:
                print("   ✅ Inject file contains expected content")
                tests_passed += 1
            else:
                print("   ❌ Inject file missing expected content")
    except Exception as e:
        print(f"   ❌ Inject file validation failed: {e}")
    
    # Test 6: Full pipeline
    print("\n📋 Test 6: Full Pipeline")
    print("-" * 30)
    
    # Clean up and run full pipeline
    for file in ['build/cns_spans.json', 'src/cns_otel.h', 'src/cns_otel_inject.c']:
        if os.path.exists(file):
            os.remove(file)
    
    tests_total += 1
    pipeline_cmd = [
        'python3', 'codegen/extract_spans.py', 'docs/ontology/cns-core.ttl', 'build/cns_spans.json',
        '&&',
        'python3', 'codegen/weaver_simple.py', 'build/cns_spans.json', 'templates/otel_header.h.j2', 'templates/otel_inject.c.j2', 'src', 'cns.h'
    ]
    
    # Run pipeline as shell command
    try:
        result = subprocess.run(' '.join(pipeline_cmd), shell=True, capture_output=True, text=True, timeout=60)
        if result.returncode == 0:
            print("   ✅ Full pipeline successful")
            tests_passed += 1
        else:
            print(f"   ❌ Full pipeline failed: {result.stderr}")
    except Exception as e:
        print(f"   ❌ Full pipeline exception: {e}")
    
    # Final validation
    tests_total += 1
    if all(os.path.exists(f) for f in ['build/cns_spans.json', 'src/cns_otel.h', 'src/cns_otel_inject.c']):
        print("   ✅ All generated files present")
        tests_passed += 1
    else:
        print("   ❌ Missing generated files")
    
    # Summary
    print("\n" + "=" * 50)
    print(f"📊 Validation Summary")
    print(f"   Tests passed: {tests_passed}/{tests_total}")
    print(f"   Success rate: {(tests_passed/tests_total)*100:.1f}%")
    
    if tests_passed == tests_total:
        print("   🎉 All tests passed! Weaver CLI is working correctly.")
        return 0
    else:
        print("   ⚠️  Some tests failed. Please review the output above.")
        return 1

if __name__ == "__main__":
    sys.exit(main()) 