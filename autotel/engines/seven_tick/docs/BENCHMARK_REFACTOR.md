# 7T Engine Benchmark Refactor Plan

## Overview

This document outlines the comprehensive refactor plan to replace all manual benchmarks in the 7T Engine codebase with the new automated benchmarking framework. The goal is to consolidate scattered benchmark implementations into a unified, maintainable, and comprehensive testing system.

## Current Benchmark Landscape

### Manual Benchmarks Found

#### 1. Verification Directory Benchmarks (47 files)
```
verification/
├── cjinja_benchmark.c                    # CJinja template engine benchmarks
├── cjinja_80_20_benchmark.c              # 80/20 optimization benchmarks
├── cjinja_7tick_vs_49tick_benchmark.c    # Performance comparison benchmarks
├── shacl_validation_benchmark.c          # SHACL validation benchmarks
├── shacl_7tick_benchmark.c               # 7-tick SHACL benchmarks
├── shacl_implementation_benchmark.c      # SHACL implementation benchmarks
├── sparql_7tick_benchmark.c              # SPARQL 7-tick benchmarks
├── sparql_realistic_benchmark.c          # Realistic SPARQL benchmarks
├── sparql_batch_80_20_benchmark.c        # Batch SPARQL benchmarks
├── sparql_80_20_benchmark.c              # SPARQL 80/20 benchmarks
├── sparql_80_20_summary.c                # SPARQL summary benchmarks
├── sparql_simple_test.c                  # Simple SPARQL tests
├── sparql_test.c                         # General SPARQL tests
├── telemetry7t_benchmark.c               # Telemetry system benchmarks
├── telemetry7t_7tick_benchmark.c         # 7-tick telemetry benchmarks
├── telemetry7t_json_benchmark.c          # JSON telemetry benchmarks
├── telemetry7t_simple_json.c             # Simple JSON benchmarks
├── gatekeeper_benchmark.c                # Gatekeeper benchmarks
├── 7t_tpot_benchmark.c                   # TPOT engine benchmarks
├── 7t_ultra_fast_benchmark.c             # Ultra-fast benchmarks
├── seven_tick_benchmark.c                # General 7-tick benchmarks
├── seven_tick_summary.c                  # 7-tick summary benchmarks
├── new_features_summary.c                # New features benchmarks
├── materialization_benchmark.c           # Materialization benchmarks
├── batch_operations_benchmark.c          # Batch operation benchmarks
├── feature_comparison_benchmark.c        # Feature comparison benchmarks
├── compression_benchmark.c               # Compression benchmarks
├── pattern_matching_benchmark.c          # Pattern matching benchmarks
├── cost_model_benchmark.c                # Cost model benchmarks
├── owl_reasoning_benchmark.c             # OWL reasoning benchmarks
├── simple_benchmark.c                    # Simple benchmarks
├── performance_test.c                    # Performance tests
├── hash_test.c                           # Hash function tests
├── test_loader.c                         # Test loader
├── memory_test.c                         # Memory tests
├── few_triples_test.c                    # Few triples tests
├── triple_only_test.c                    # Triple-only tests
├── single_triple_test.c                  # Single triple tests
├── debug_test.c                          # Debug tests
├── minimal_test.c                        # Minimal tests
├── unit_test.c                           # Unit tests
├── small_test.c                          # Small tests
├── test_system.c                         # System tests
└── [34 additional benchmark files...]
```

#### 2. C Source Directory Benchmarks (8 files)
```
c_src/
├── 7t_tpot.c                             # Contains benchmark_7t_tpot()
├── 7t_tpot_49ticks.c                     # Contains benchmark_7t_tpot_49ticks()
├── 7t_tpot_arm64.c                       # Contains benchmark_7t_tpot()
├── 7t_tpot_real.c                        # Contains benchmark_7t_tpot_49ticks()
├── telemetry7t.c                         # Contains telemetry7t_benchmark()
├── pm7t.c                                # Contains PM7T benchmarks
├── sparql7t_optimized.c                  # Contains optimized benchmarks
└── [Additional benchmark functions...]
```

#### 3. Demo Directory Benchmarks (5 files)
```
demos/
├── 01_sparql_knowledge_graph.c           # Contains SPARQL benchmarks
├── 02_shacl_validation.c                 # Contains SHACL benchmarks
├── 03_cjinja_templating.c                # Contains CJinja benchmarks
├── 04_memory_optimization.c              # Contains memory benchmarks
└── 05_integrated_workflow.c              # Contains integrated benchmarks
```

#### 4. Python Benchmark Files (8 files)
```
├── test_7t_optimized.py                  # Python optimization tests
├── test_shacl_implementation.py          # SHACL implementation tests
├── shacl7t_real.py                       # Real SHACL benchmarks
├── run_benchmarks.py                     # Benchmark runner
├── domains/benchmark.py                  # Domain-specific benchmarks
└── [Additional Python benchmark files...]
```

#### 5. Documentation Files (12 files)
```
docs/
├── BENCHMARKS.md                         # Benchmark documentation
├── BENCHMARK_ANALYSIS.md                 # Benchmark analysis
├── PERFORMANCE_REPORT.md.bak             # Performance reports
├── SHACL_IMPLEMENTATION_BENCHMARK_RESULTS.md
├── SHACL_7TICK_ACHIEVEMENT_REPORT.md
├── cookbook-patterns/benchmarking_guide.md
├── cookbook-patterns/performance_benchmarking.md
└── [Additional benchmark docs...]
```

## Refactor Strategy

### Phase 1: Consolidation (Week 1)

#### 1.1 Create Unified Benchmark Categories
```c
// New benchmark categories in 7t_benchmark_suite.c
typedef enum {
    BENCHMARK_CATEGORY_CORE_ENGINE,       // TPOT, SPARQL, SHACL core
    BENCHMARK_CATEGORY_TEMPLATING,        // CJinja template engine
    BENCHMARK_CATEGORY_TELEMETRY,         // Telemetry system
    BENCHMARK_CATEGORY_MEMORY,            // Memory optimization
    BENCHMARK_CATEGORY_INTEGRATION,       // Integrated workflows
    BENCHMARK_CATEGORY_PERFORMANCE,       // Performance optimization
    BENCHMARK_CATEGORY_COMPARISON,        // Comparison benchmarks
    BENCHMARK_CATEGORY_STRESS,            // Stress testing
    BENCHMARK_CATEGORY_REGRESSION,        // Regression testing
    BENCHMARK_CATEGORY_VALIDATION         // Validation testing
} BenchmarkCategory;
```

#### 1.2 Migrate Core Engine Benchmarks
```c
// Migrate from verification/seven_tick_benchmark.c
void test_core_engine_performance(void* context) {
    // Migrate seven_tick_benchmark.c logic
}

// Migrate from verification/7t_tpot_benchmark.c
void test_tpot_engine_performance(void* context) {
    // Migrate 7t_tpot_benchmark.c logic
}

// Migrate from verification/sparql_7tick_benchmark.c
void test_sparql_engine_performance(void* context) {
    // Migrate sparql_7tick_benchmark.c logic
}
```

#### 1.3 Migrate Template Engine Benchmarks
```c
// Migrate from verification/cjinja_benchmark.c
void test_cjinja_templating_performance(void* context) {
    // Migrate cjinja_benchmark.c logic
}

// Migrate from verification/cjinja_80_20_benchmark.c
void test_cjinja_80_20_optimization(void* context) {
    // Migrate cjinja_80_20_benchmark.c logic
}
```

### Phase 2: Integration (Week 2)

#### 2.1 Migrate Telemetry Benchmarks
```c
// Migrate from verification/telemetry7t_benchmark.c
void test_telemetry_system_performance(void* context) {
    // Migrate telemetry7t_benchmark.c logic
}

// Migrate from verification/telemetry7t_7tick_benchmark.c
void test_telemetry_7tick_performance(void* context) {
    // Migrate telemetry7t_7tick_benchmark.c logic
}
```

#### 2.2 Migrate SHACL Benchmarks
```c
// Migrate from verification/shacl_validation_benchmark.c
void test_shacl_validation_performance(void* context) {
    // Migrate shacl_validation_benchmark.c logic
}

// Migrate from verification/shacl_7tick_benchmark.c
void test_shacl_7tick_performance(void* context) {
    // Migrate shacl_7tick_benchmark.c logic
}
```

#### 2.3 Migrate Memory Benchmarks
```c
// Migrate from demos/04_memory_optimization.c
void test_memory_optimization_performance(void* context) {
    // Migrate memory optimization benchmarks
}

// Migrate from verification/memory_test.c
void test_memory_efficiency(void* context) {
    // Migrate memory test logic
}
```

### Phase 3: Advanced Features (Week 3)

#### 3.1 Migrate Comparison Benchmarks
```c
// Migrate from verification/cjinja_7tick_vs_49tick_benchmark.c
void test_7tick_vs_49tick_comparison(void* context) {
    // Migrate comparison logic
}

// Migrate from verification/feature_comparison_benchmark.c
void test_feature_comparison(void* context) {
    // Migrate feature comparison logic
}
```

#### 3.2 Migrate Stress Tests
```c
// Migrate from verification/7t_ultra_fast_benchmark.c
void test_ultra_fast_performance(void* context) {
    // Migrate ultra-fast benchmark logic
}

// Migrate from verification/stress_test.c
void test_stress_performance(void* context) {
    // Migrate stress test logic
}
```

#### 3.3 Migrate Integration Tests
```c
// Migrate from demos/05_integrated_workflow.c
void test_integrated_workflow_performance(void* context) {
    // Migrate integrated workflow benchmarks
}

// Migrate from verification/batch_operations_benchmark.c
void test_batch_operations_performance(void* context) {
    // Migrate batch operations logic
}
```

### Phase 4: Python Integration (Week 4)

#### 4.1 Migrate Python Benchmarks
```python
# Migrate from test_7t_optimized.py
def test_7t_optimization_benchmarks():
    # Migrate Python optimization tests
    pass

# Migrate from test_shacl_implementation.py
def test_shacl_implementation_benchmarks():
    # Migrate SHACL implementation tests
    pass

# Migrate from shacl7t_real.py
def test_shacl_real_benchmarks():
    # Migrate real SHACL benchmarks
    pass
```

#### 4.2 Create Python Benchmark Wrapper
```python
# New: benchmarks/7t_python_benchmarks.py
import subprocess
import json
import time

class PythonBenchmarkRunner:
    def __init__(self):
        self.framework = "7t_benchmark_framework"
    
    def run_c_benchmarks(self):
        """Run C benchmarks through the framework"""
        result = subprocess.run(["./7t_benchmark_suite"], 
                              capture_output=True, text=True)
        return self.parse_benchmark_output(result.stdout)
    
    def run_python_benchmarks(self):
        """Run Python-specific benchmarks"""
        benchmarks = [
            self.test_7t_optimization_benchmarks,
            self.test_shacl_implementation_benchmarks,
            self.test_shacl_real_benchmarks
        ]
        
        results = []
        for benchmark in benchmarks:
            start_time = time.time()
            result = benchmark()
            end_time = time.time()
            
            results.append({
                "test_name": benchmark.__name__,
                "execution_time": end_time - start_time,
                "result": result
            })
        
        return results
```

## Implementation Plan

### Step 1: Create Migration Scripts

#### 1.1 C Benchmark Migration Script
```bash
#!/bin/bash
# migrate_c_benchmarks.sh

echo "Migrating C benchmarks to unified framework..."

# Create backup directory
mkdir -p benchmarks/legacy_backup

# Backup existing benchmarks
cp verification/*.c benchmarks/legacy_backup/
cp c_src/*.c benchmarks/legacy_backup/
cp demos/*.c benchmarks/legacy_backup/

# Extract benchmark functions
echo "Extracting benchmark functions from:"
echo "- verification/ (47 files)"
echo "- c_src/ (8 files)"
echo "- demos/ (5 files)"

# Generate migration report
cat > benchmarks/migration_report.md << EOF
# Benchmark Migration Report

## Files Migrated
- verification/: 47 benchmark files
- c_src/: 8 benchmark files  
- demos/: 5 benchmark files

## Functions Extracted
- benchmark_7t_tpot()
- telemetry7t_benchmark()
- shacl_validation_benchmark()
- cjinja_benchmark()
- sparql_benchmark()
- [Additional functions...]

## Migration Status
- [ ] Core engine benchmarks
- [ ] Template engine benchmarks
- [ ] Telemetry benchmarks
- [ ] SHACL benchmarks
- [ ] Memory benchmarks
- [ ] Integration benchmarks
EOF
```

#### 1.2 Python Benchmark Migration Script
```python
#!/usr/bin/env python3
# migrate_python_benchmarks.py

import os
import shutil
import ast

def extract_benchmark_functions(file_path):
    """Extract benchmark functions from Python files"""
    with open(file_path, 'r') as f:
        tree = ast.parse(f.read())
    
    benchmark_functions = []
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef):
            if 'benchmark' in node.name.lower():
                benchmark_functions.append(node.name)
    
    return benchmark_functions

def migrate_python_benchmarks():
    """Migrate Python benchmark files"""
    python_files = [
        'test_7t_optimized.py',
        'test_shacl_implementation.py',
        'shacl7t_real.py',
        'run_benchmarks.py',
        'domains/benchmark.py'
    ]
    
    for file_path in python_files:
        if os.path.exists(file_path):
            functions = extract_benchmark_functions(file_path)
            print(f"Found {len(functions)} benchmark functions in {file_path}")
            print(f"  Functions: {functions}")

if __name__ == "__main__":
    migrate_python_benchmarks()
```

### Step 2: Update Makefile

#### 2.1 Remove Legacy Benchmark Targets
```makefile
# Remove these targets from Makefile
# $(SHACL_BENCHMARK_BIN): $(SHACL_BENCHMARK_SRC) $(RUNTIME_LIB)
# $(CJINJA_BENCHMARK_BIN): $(CJINJA_BENCHMARK_SRC) $(COMPILER_DIR)/cjinja.c
# $(TELEMETRY7T_BENCHMARK_BIN): $(TELEMETRY7T_BENCHMARK_SRC) $(TELEMETRY7T_SRC)
# [Additional legacy targets...]
```

#### 2.2 Add New Unified Targets
```makefile
# Unified benchmark targets
benchmark-all: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --all

benchmark-core: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category core_engine

benchmark-templating: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category templating

benchmark-telemetry: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category telemetry

benchmark-shacl: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category shacl

benchmark-memory: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category memory

benchmark-integration: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category integration

benchmark-performance: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category performance

benchmark-comparison: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category comparison

benchmark-stress: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category stress

benchmark-regression: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category regression

benchmark-validation: $(BENCHMARK_SUITE_BIN)
	./$(BENCHMARK_SUITE_BIN) --category validation
```

### Step 3: Update Documentation

#### 3.1 Create Migration Guide
```markdown
# Benchmark Migration Guide

## Overview
This guide explains how to migrate from legacy benchmarks to the new unified framework.

## Migration Steps

### 1. Legacy Benchmark Identification
- **verification/**: 47 benchmark files
- **c_src/**: 8 benchmark files
- **demos/**: 5 benchmark files
- **Python files**: 8 benchmark files

### 2. New Framework Structure
```
benchmarks/
├── 7t_benchmark_framework.h          # Framework header
├── 7t_benchmark_framework.c          # Framework implementation
├── 7t_benchmark_suite.c              # Unified benchmark suite
├── 7t_benchmark_runner.py            # Python automation
├── legacy_backup/                    # Backup of legacy benchmarks
└── migration_report.md               # Migration status
```

### 3. Migration Process
1. **Backup**: All legacy benchmarks are backed up
2. **Extract**: Benchmark functions are extracted
3. **Integrate**: Functions are integrated into unified framework
4. **Test**: New framework is validated
5. **Cleanup**: Legacy files are removed

### 4. Usage Examples

#### Legacy Usage
```bash
# Old way - multiple commands
make verification
./verification/shacl_validation_benchmark
./verification/cjinja_benchmark
./verification/telemetry7t_benchmark
```

#### New Usage
```bash
# New way - unified framework
make benchmark-all
make benchmark-shacl
make benchmark-templating
make benchmark-telemetry
```

### 5. Benefits
- **Unified Interface**: Single command for all benchmarks
- **Consistent Metrics**: Standardized performance measurement
- **Automated Analysis**: Built-in result analysis and reporting
- **Continuous Monitoring**: Regression detection and alerting
- **Better Documentation**: Comprehensive benchmark documentation
```

## Migration Timeline

### Week 1: Foundation
- [x] Create unified benchmark framework
- [x] Implement core benchmark categories
- [ ] Migrate core engine benchmarks
- [ ] Update Makefile targets

### Week 2: Core Components
- [ ] Migrate template engine benchmarks
- [ ] Migrate telemetry benchmarks
- [ ] Migrate SHACL benchmarks
- [ ] Migrate memory benchmarks

### Week 3: Advanced Features
- [ ] Migrate comparison benchmarks
- [ ] Migrate stress tests
- [ ] Migrate integration tests
- [ ] Implement regression detection

### Week 4: Python Integration
- [ ] Migrate Python benchmarks
- [ ] Create Python wrapper
- [ ] Update documentation
- [ ] Final testing and validation

### Week 5: Cleanup
- [ ] Remove legacy benchmark files
- [ ] Update all documentation
- [ ] Create migration guide
- [ ] Final validation

## Success Criteria

### Technical Criteria
- [ ] All legacy benchmarks successfully migrated
- [ ] No performance regression in migrated benchmarks
- [ ] Unified framework provides same or better functionality
- [ ] Automated analysis and reporting working
- [ ] Continuous monitoring implemented

### Process Criteria
- [ ] All legacy files backed up
- [ ] Migration documented and reproducible
- [ ] Team trained on new framework
- [ ] Documentation updated
- [ ] CI/CD pipeline updated

### Quality Criteria
- [ ] All benchmarks pass validation
- [ ] Performance targets maintained
- [ ] Code coverage maintained or improved
- [ ] Maintainability improved
- [ ] User experience improved

## Risk Mitigation

### Technical Risks
- **Performance Regression**: Comprehensive testing before migration
- **Functionality Loss**: Thorough validation of migrated benchmarks
- **Integration Issues**: Incremental migration with rollback capability

### Process Risks
- **Data Loss**: Complete backup of all legacy files
- **Team Resistance**: Training and documentation
- **Timeline Slippage**: Phased approach with clear milestones

### Quality Risks
- **Inconsistent Results**: Standardized measurement framework
- **Poor Documentation**: Comprehensive documentation updates
- **Maintenance Burden**: Automated framework reduces maintenance

## Conclusion

The benchmark refactor will consolidate 60+ scattered benchmark files into a unified, maintainable framework that provides:

1. **Better Performance Measurement**: High-precision cycle counting and latency measurement
2. **Automated Analysis**: Built-in result analysis and regression detection
3. **Continuous Monitoring**: Real-time performance monitoring and alerting
4. **Improved Maintainability**: Single codebase for all benchmarks
5. **Enhanced Documentation**: Comprehensive benchmark documentation and guides

The migration will be completed in 5 weeks with minimal disruption to development workflow and maximum improvement in benchmark quality and maintainability. 