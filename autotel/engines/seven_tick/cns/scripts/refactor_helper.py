#!/usr/bin/env python3
"""
CNS Refactor Helper Script
Automates the integration of specialized helper utilities into the codebase.
"""

import os
import sys
import re
import json
import subprocess
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class CNSRefactorHelper:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.backup_dir = self.project_root / "backup_refactor"
        self.results_file = self.project_root / "refactor_results.json"
        
        # Target files with their refactor configurations
        self.target_files = {
            "cns/src/domains/sql/sql_domain.c": {
                "priority": "HIGH",
                "expected_improvement": "10x",
                "helpers": ["S7TTable", "sql_column_filter_int32_simd", "S7TValidator"],
                "include_helpers": True,
                "replace_structures": True,
                "add_simd_operations": True,
                "add_performance_monitoring": True
            },
            "cns/src/cmd_ml.c": {
                "priority": "HIGH",
                "expected_improvement": "10x",
                "helpers": ["ml_matrix_multiply_simd", "MLAlgorithmRegistry", "MLModelCacheManager"],
                "include_helpers": True,
                "replace_matrix_operations": True,
                "add_algorithm_registry": True,
                "add_model_caching": True
            },
            "c_src/pm7t.c": {
                "priority": "HIGH",
                "expected_improvement": "10x",
                "helpers": ["Dataset7T", "tpot_dataset_normalize_simd", "PipelineQueue"],
                "include_helpers": True,
                "replace_dataset_structure": True,
                "add_simd_operations": True,
                "add_pipeline_queue": True
            },
            "compiler/src/cjinja.c": {
                "priority": "MEDIUM",
                "expected_improvement": "2x",
                "helpers": ["TemplateCache", "cjinja_string_upper_simd", "StringPool"],
                "include_helpers": True,
                "replace_cache_structure": True,
                "add_simd_string_operations": True,
                "add_string_pool": True
            },
            "lib/7t_performance.c": {
                "priority": "MEDIUM",
                "expected_improvement": "2x",
                "helpers": ["S7TValidator", "MemoryTracker", "cns_get_nanoseconds"],
                "include_helpers": True,
                "replace_timing_functions": True,
                "add_validator": True,
                "add_memory_tracker": True
            },
            "runtime/src/seven_t_runtime.c": {
                "priority": "MEDIUM",
                "expected_improvement": "2x",
                "helpers": ["ArenaAllocator", "ObjectPool", "RuntimeStats"],
                "include_helpers": True,
                "add_arena_allocator": True,
                "add_object_pool": True,
                "add_runtime_stats": True
            },
            "lib/7t_process_mining.c": {
                "priority": "LOW",
                "expected_improvement": "5x",
                "helpers": ["pm_matrix_multiply_simd", "EventSet", "pm_vector_add_simd"],
                "include_helpers": True,
                "replace_matrix_operations": True,
                "add_event_sets": True
            },
            "lib/s7t_perf.c": {
                "priority": "LOW",
                "expected_improvement": "2x",
                "helpers": ["CacheSimulator", "BranchAnalyzer", "PerformanceTestSuite"],
                "include_helpers": True,
                "add_cache_simulator": True,
                "add_branch_analyzer": True
            },
            "tests/test_telemetry7t.c": {
                "priority": "LOW",
                "expected_improvement": "2x",
                "helpers": ["TestDataGenerator", "TestPerformanceMetrics", "StressTestConfig"],
                "include_helpers": True,
                "add_test_data_generator": True,
                "add_performance_metrics": True
            }
        }
        
        self.results = {
            "refactored_files": [],
            "performance_improvements": {},
            "errors": [],
            "warnings": []
        }

    def backup_file(self, file_path: Path) -> bool:
        """Create a backup of the original file."""
        try:
            if not self.backup_dir.exists():
                self.backup_dir.mkdir(parents=True)
            
            backup_path = self.backup_dir / file_path.name
            if file_path.exists():
                import shutil
                shutil.copy2(file_path, backup_path)
                print(f"✓ Backed up {file_path} to {backup_path}")
                return True
        except Exception as e:
            print(f"✗ Failed to backup {file_path}: {e}")
            return False
        return False

    def add_helper_include(self, file_path: Path) -> bool:
        """Add the specialized helpers include to the file."""
        try:
            content = file_path.read_text()
            
            # Check if include already exists
            if "#include \"cns/specialized_helpers.h\"" in content:
                print(f"  - Helper include already present in {file_path}")
                return True
            
            # Find the best place to add the include
            lines = content.split('\n')
            include_index = -1
            
            # Look for existing includes
            for i, line in enumerate(lines):
                if line.strip().startswith('#include'):
                    include_index = i
            
            # Add the include after existing includes
            if include_index >= 0:
                lines.insert(include_index + 1, '#include "cns/specialized_helpers.h"')
            else:
                # Add at the beginning if no includes found
                lines.insert(0, '#include "cns/specialized_helpers.h"')
            
            # Write back the file
            file_path.write_text('\n'.join(lines))
            print(f"  ✓ Added helper include to {file_path}")
            return True
            
        except Exception as e:
            print(f"  ✗ Failed to add helper include to {file_path}: {e}")
            return False

    def replace_table_structure(self, file_path: Path) -> bool:
        """Replace table structure with specialized helper structure."""
        try:
            content = file_path.read_text()
            
            # Replace s7t_table_t typedef
            old_pattern = r'typedef struct\s*\{[^}]*\}\s*s7t_table_t;'
            new_content = 'typedef S7TTable s7t_table_t;'
            
            if re.search(old_pattern, content, re.DOTALL):
                content = re.sub(old_pattern, new_content, content, flags=re.DOTALL)
                file_path.write_text(content)
                print(f"  ✓ Replaced table structure in {file_path}")
                return True
            else:
                print(f"  - No table structure found to replace in {file_path}")
                return True
                
        except Exception as e:
            print(f"  ✗ Failed to replace table structure in {file_path}: {e}")
            return False

    def add_simd_column_operations(self, file_path: Path) -> bool:
        """Add SIMD column operations to the file."""
        try:
            content = file_path.read_text()
            
            # Look for simple column filtering patterns
            simple_filter_pattern = r'for\s*\(\s*uint32_t\s+i\s*=\s*0;\s*i\s*<\s*table->row_count;\s*i\+\+\)\s*\{[^}]*if\s*\(\s*data\[i\]\s*==\s*value\s*\)[^}]*\}'
            
            if re.search(simple_filter_pattern, content, re.DOTALL):
                # Add SIMD column filtering function
                simd_function = '''
// SIMD-optimized column filtering
static void sql_column_filter_simd(s7t_column_t* column, int32_t value, uint64_t* result_mask) {
    if (!column || column->type != S7T_TYPE_INT32) return;
    
    int32_t* data = (int32_t*)column->data;
    size_t word_count = (column->size + 63) / 64;
    
    CNS_SIMD_BATCH_START(word_count);
    
    for (size_t i = 0; i < cns_simd_count; i += CNS_VECTOR_WIDTH) {
        for (int j = 0; j < CNS_VECTOR_WIDTH && i + j < word_count; j++) {
            uint64_t mask = 0;
            for (int k = 0; k < 64 && (i + j) * 64 + k < column->size; k++) {
                if (data[(i + j) * 64 + k] == value) {
                    mask |= (1ULL << k);
                }
            }
            result_mask[i + j] = mask;
        }
    }
    
    CNS_SIMD_BATCH_END(word_count) {
        uint64_t mask = 0;
        for (int k = 0; k < 64 && cns_i * 64 + k < column->size; k++) {
            if (data[cns_i * 64 + k] == value) {
                mask |= (1ULL << k);
            }
        }
        result_mask[cns_i] = mask;
    }
}
'''
                
                # Find a good place to add the function (before main functions)
                lines = content.split('\n')
                insert_index = -1
                
                for i, line in enumerate(lines):
                    if 'int main(' in line or 'static int cmd_' in line:
                        insert_index = i
                        break
                
                if insert_index >= 0:
                    lines.insert(insert_index, simd_function)
                    content = '\n'.join(lines)
                    file_path.write_text(content)
                    print(f"  ✓ Added SIMD column operations to {file_path}")
                    return True
                else:
                    print(f"  - Could not find suitable location for SIMD function in {file_path}")
                    return True
            else:
                print(f"  - No simple column filtering found to optimize in {file_path}")
                return True
                
        except Exception as e:
            print(f"  ✗ Failed to add SIMD column operations to {file_path}: {e}")
            return False

    def add_performance_monitoring(self, file_path: Path) -> bool:
        """Add performance monitoring to the file."""
        try:
            content = file_path.read_text()
            
            # Add performance monitoring variables
            monitoring_vars = '''
// Performance monitoring
static S7TValidator* g_sql_validator = NULL;
'''
            
            # Add to global variables section
            lines = content.split('\n')
            insert_index = -1
            
            for i, line in enumerate(lines):
                if 'static struct' in line or 'static {' in line:
                    insert_index = i
                    break
            
            if insert_index >= 0:
                lines.insert(insert_index, monitoring_vars)
                content = '\n'.join(lines)
                file_path.write_text(content)
                print(f"  ✓ Added performance monitoring variables to {file_path}")
                return True
            else:
                print(f"  - Could not find suitable location for performance monitoring in {file_path}")
                return True
                
        except Exception as e:
            print(f"  ✗ Failed to add performance monitoring to {file_path}: {e}")
            return False

    def replace_matrix_operations(self, file_path: Path) -> bool:
        """Replace matrix operations with SIMD versions."""
        try:
            content = file_path.read_text()
            
            # Look for simple matrix multiplication patterns
            matrix_mult_pattern = r'for\s*\(\s*size_t\s+i\s*=\s*0;\s*i\s*<\s*rows_a;\s*i\+\+\)\s*\{[^}]*for\s*\(\s*size_t\s+j\s*=\s*0;\s*j\s*<\s*cols_b;\s*j\+\+\)\s*\{[^}]*double\s+sum\s*=\s*0\.0;[^}]*for\s*\(\s*size_t\s+k\s*=\s*0;\s*k\s*<\s*cols_a;\s*k\+\+\)\s*\{[^}]*sum\s*\+=\s*a\[i\s*\*\s*cols_a\s*\+\s*k\]\s*\*\s*b\[k\s*\*\s*cols_b\s*\+\s*j\];[^}]*\}[^}]*result\[i\s*\*\s*cols_b\s*\+\s*j\]\s*=\s*sum;[^}]*\}'
            
            if re.search(matrix_mult_pattern, content, re.DOTALL):
                # Replace with SIMD version
                simd_replacement = 'ml_matrix_multiply_simd(a, b, result, rows_a, cols_a, cols_b);'
                content = re.sub(matrix_mult_pattern, simd_replacement, content, flags=re.DOTALL)
                file_path.write_text(content)
                print(f"  ✓ Replaced matrix operations with SIMD version in {file_path}")
                return True
            else:
                print(f"  - No matrix multiplication pattern found to replace in {file_path}")
                return True
                
        except Exception as e:
            print(f"  ✗ Failed to replace matrix operations in {file_path}: {e}")
            return False

    def add_algorithm_registry(self, file_path: Path) -> bool:
        """Add algorithm registry to the file."""
        try:
            content = file_path.read_text()
            
            # Add algorithm registry variable
            registry_var = '''
// Algorithm registry
static MLAlgorithmRegistry* g_ml_registry = NULL;
'''
            
            # Add to global variables section
            lines = content.split('\n')
            insert_index = -1
            
            for i, line in enumerate(lines):
                if 'static struct' in line or 'static {' in line or 'Algorithm7T' in line:
                    insert_index = i
                    break
            
            if insert_index >= 0:
                lines.insert(insert_index, registry_var)
                content = '\n'.join(lines)
                file_path.write_text(content)
                print(f"  ✓ Added algorithm registry to {file_path}")
                return True
            else:
                print(f"  - Could not find suitable location for algorithm registry in {file_path}")
                return True
                
        except Exception as e:
            print(f"  ✗ Failed to add algorithm registry to {file_path}: {e}")
            return False

    def refactor_file(self, file_path: Path, config: Dict) -> bool:
        """Refactor a single file according to its configuration."""
        print(f"\n🔧 Refactoring {file_path} (Priority: {config['priority']})")
        
        # Backup the file
        if not self.backup_file(file_path):
            return False
        
        success = True
        
        # Apply refactoring steps based on configuration
        if config.get('include_helpers', False):
            success &= self.add_helper_include(file_path)
        
        if config.get('replace_structures', False):
            success &= self.replace_table_structure(file_path)
        
        if config.get('add_simd_operations', False):
            success &= self.add_simd_column_operations(file_path)
        
        if config.get('add_performance_monitoring', False):
            success &= self.add_performance_monitoring(file_path)
        
        if config.get('replace_matrix_operations', False):
            success &= self.replace_matrix_operations(file_path)
        
        if config.get('add_algorithm_registry', False):
            success &= self.add_algorithm_registry(file_path)
        
        if success:
            self.results["refactored_files"].append(str(file_path))
            print(f"✓ Successfully refactored {file_path}")
        else:
            self.results["errors"].append(f"Failed to refactor {file_path}")
            print(f"✗ Failed to refactor {file_path}")
        
        return success

    def run_tests(self, file_path: Path) -> bool:
        """Run tests for the refactored file."""
        try:
            print(f"\n🧪 Running tests for {file_path}")
            
            # Determine test command based on file
            if "sql" in str(file_path):
                test_cmd = ["make", "test-sql"]
            elif "ml" in str(file_path):
                test_cmd = ["make", "test-ml"]
            elif "cjinja" in str(file_path):
                test_cmd = ["make", "test-cjinja"]
            else:
                test_cmd = ["make", "test"]
            
            result = subprocess.run(test_cmd, cwd=self.project_root, 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"✓ Tests passed for {file_path}")
                return True
            else:
                print(f"✗ Tests failed for {file_path}")
                print(f"Error output: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"✗ Failed to run tests for {file_path}: {e}")
            return False

    def run_benchmarks(self, file_path: Path) -> bool:
        """Run benchmarks for the refactored file."""
        try:
            print(f"\n📊 Running benchmarks for {file_path}")
            
            # Determine benchmark command based on file
            if "sql" in str(file_path):
                bench_cmd = ["make", "bench-sql"]
            elif "ml" in str(file_path):
                bench_cmd = ["make", "bench-ml"]
            elif "cjinja" in str(file_path):
                bench_cmd = ["make", "bench-cjinja"]
            else:
                bench_cmd = ["make", "bench"]
            
            result = subprocess.run(bench_cmd, cwd=self.project_root, 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"✓ Benchmarks completed for {file_path}")
                # Parse benchmark results and store
                self.results["performance_improvements"][str(file_path)] = "Measured"
                return True
            else:
                print(f"✗ Benchmarks failed for {file_path}")
                print(f"Error output: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"✗ Failed to run benchmarks for {file_path}: {e}")
            return False

    def refactor_by_priority(self, priority: str) -> bool:
        """Refactor all files of a given priority."""
        print(f"\n🎯 Refactoring {priority} priority files")
        
        priority_files = {k: v for k, v in self.target_files.items() 
                         if v["priority"] == priority}
        
        if not priority_files:
            print(f"No files found with {priority} priority")
            return True
        
        success = True
        for file_path_str, config in priority_files.items():
            file_path = self.project_root / file_path_str
            
            if file_path.exists():
                success &= self.refactor_file(file_path, config)
                
                # Run tests after each file
                if success:
                    success &= self.run_tests(file_path)
                
                # Run benchmarks after each file
                if success:
                    success &= self.run_benchmarks(file_path)
            else:
                print(f"⚠ File not found: {file_path}")
                self.results["warnings"].append(f"File not found: {file_path}")
        
        return success

    def save_results(self):
        """Save refactoring results to JSON file."""
        try:
            with open(self.results_file, 'w') as f:
                json.dump(self.results, f, indent=2)
            print(f"\n💾 Results saved to {self.results_file}")
        except Exception as e:
            print(f"✗ Failed to save results: {e}")

    def generate_report(self):
        """Generate a refactoring report."""
        print("\n📋 REFACTORING REPORT")
        print("=" * 50)
        
        print(f"\nRefactored Files: {len(self.results['refactored_files'])}")
        for file in self.results['refactored_files']:
            print(f"  ✓ {file}")
        
        print(f"\nPerformance Improvements: {len(self.results['performance_improvements'])}")
        for file, status in self.results['performance_improvements'].items():
            print(f"  📊 {file}: {status}")
        
        if self.results['errors']:
            print(f"\nErrors: {len(self.results['errors'])}")
            for error in self.results['errors']:
                print(f"  ✗ {error}")
        
        if self.results['warnings']:
            print(f"\nWarnings: {len(self.results['warnings'])}")
            for warning in self.results['warnings']:
                print(f"  ⚠ {warning}")
        
        print(f"\nBackup Location: {self.backup_dir}")
        print(f"Results File: {self.results_file}")

def main():
    parser = argparse.ArgumentParser(description="CNS Refactor Helper")
    parser.add_argument("project_root", help="Path to the CNS project root")
    parser.add_argument("--priority", choices=["HIGH", "MEDIUM", "LOW", "ALL"], 
                       default="ALL", help="Priority level to refactor")
    parser.add_argument("--dry-run", action="store_true", 
                       help="Show what would be refactored without making changes")
    parser.add_argument("--file", help="Refactor a specific file")
    
    args = parser.parse_args()
    
    refactor_helper = CNSRefactorHelper(args.project_root)
    
    if args.dry_run:
        print("🔍 DRY RUN - No changes will be made")
        print("\nTarget files:")
        for file_path, config in refactor_helper.target_files.items():
            if args.priority == "ALL" or config["priority"] == args.priority:
                print(f"  {file_path} ({config['priority']} priority)")
        return
    
    if args.file:
        # Refactor specific file
        file_path = Path(args.project_root) / args.file
        if file_path.exists() and str(file_path) in refactor_helper.target_files:
            config = refactor_helper.target_files[str(file_path)]
            refactor_helper.refactor_file(file_path, config)
            refactor_helper.run_tests(file_path)
            refactor_helper.run_benchmarks(file_path)
        else:
            print(f"✗ File not found or not in target list: {args.file}")
            return
    else:
        # Refactor by priority
        if args.priority == "ALL":
            priorities = ["HIGH", "MEDIUM", "LOW"]
        else:
            priorities = [args.priority]
        
        for priority in priorities:
            refactor_helper.refactor_by_priority(priority)
    
    refactor_helper.save_results()
    refactor_helper.generate_report()

if __name__ == "__main__":
    main() 