#!/usr/bin/env python3
"""
AOT (Ahead-of-Time) Compiler Production Pipeline
The Reasoner IS the Build System

This is the main production compiler that orchestrates the entire AOT pipeline.
It transforms semantic specifications into deterministic, optimized C code.
"""

import os
import sys
import json
import time
import logging
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple
from dataclasses import dataclass, field
from datetime import datetime

# Import our pipeline components
from owl_aot_compiler import OWLCompiler
from cjinja_aot_compiler import CJinjaCompiler
from shacl_validator import SHACLValidator
from reasoning_engine import ReasoningEngine

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class CompilationUnit:
    """Represents a unit of compilation through the AOT pipeline"""
    id: str
    source_path: Path
    target_path: Path
    ontology: Dict[str, Any] = field(default_factory=dict)
    constraints: Dict[str, Any] = field(default_factory=dict)
    reasoning_cycles: List[Dict[str, Any]] = field(default_factory=list)
    optimizations: Dict[str, Any] = field(default_factory=dict)
    generated_code: str = ""
    telemetry: Dict[str, Any] = field(default_factory=dict)
    status: str = "pending"
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    compilation_time: float = 0.0
    metadata: Dict[str, Any] = field(default_factory=dict)

@dataclass
class CompilationMetrics:
    """Metrics for the compilation process"""
    total_units: int = 0
    successful_units: int = 0
    failed_units: int = 0
    total_time: float = 0.0
    reasoning_cycles: int = 0
    optimizations_applied: int = 0
    constraints_validated: int = 0
    code_size: int = 0
    telemetry_points: int = 0

class AOTCompilerProduction:
    """Main AOT Compiler Production Pipeline"""
    
    def __init__(self, config: Optional[Dict[str, Any]] = None):
        self.config = config or self._default_config()
        self.metrics = CompilationMetrics()
        
        # Initialize pipeline components
        self.owl_compiler = OWLCompiler(self.config.get('owl', {}))
        self.shacl_validator = SHACLValidator(self.config.get('shacl', {}))
        self.reasoning_engine = ReasoningEngine(self.config.get('reasoning', {}))
        self.cjinja_compiler = CJinjaCompiler(self.config.get('cjinja', {}))
        
        # Pipeline state
        self.compilation_units: List[CompilationUnit] = []
        self.compilation_cache: Dict[str, Any] = {}
        self.session_id = f"aot_{int(time.time())}"
        
        logger.info(f"AOT Compiler Production initialized - Session: {self.session_id}")
    
    def _default_config(self) -> Dict[str, Any]:
        """Default configuration for the AOT pipeline"""
        return {
            'owl': {
                'strict_mode': True,
                'inference_enabled': True,
                'reasoning_depth': 3
            },
            'shacl': {
                'validation_level': 'strict',
                'constraint_optimization': True,
                'report_format': 'detailed'
            },
            'reasoning': {
                'max_cycles': 8,
                'parallel_reasoning': True,
                'proof_generation': True,
                'cognitive_model': '8H'
            },
            'cjinja': {
                'optimization_level': 3,
                'target_architecture': '8T',
                'memory_model': '8M',
                'telemetry_enabled': True
            },
            'pipeline': {
                'parallel_compilation': True,
                'cache_enabled': True,
                'incremental_builds': True,
                'deterministic_output': True
            }
        }
    
    def compile_specification(self, spec_path: Path, output_dir: Path) -> CompilationUnit:
        """Compile a single specification through the entire AOT pipeline"""
        start_time = time.time()
        
        # Create compilation unit
        unit = CompilationUnit(
            id=f"unit_{len(self.compilation_units)}",
            source_path=spec_path,
            target_path=output_dir / f"{spec_path.stem}.c"
        )
        
        try:
            # Phase 1: OWL/TTL Processing
            logger.info(f"Phase 1: Processing OWL/TTL specification: {spec_path}")
            unit.ontology = self.owl_compiler.compile(spec_path)
            unit.metadata['ontology_classes'] = len(unit.ontology.get('classes', []))
            unit.metadata['ontology_properties'] = len(unit.ontology.get('properties', []))
            
            # Phase 2: SHACL Constraint Validation
            logger.info("Phase 2: Validating SHACL constraints")
            validation_result = self.shacl_validator.validate(
                unit.ontology,
                self._extract_constraints(unit.ontology)
            )
            unit.constraints = validation_result['constraints']
            unit.warnings.extend(validation_result.get('warnings', []))
            self.metrics.constraints_validated += len(unit.constraints)
            
            # Phase 3: 8H Reasoning Cycles
            logger.info("Phase 3: Executing 8H reasoning cycles")
            reasoning_result = self.reasoning_engine.reason(
                unit.ontology,
                unit.constraints
            )
            unit.reasoning_cycles = reasoning_result['cycles']
            unit.metadata['reasoning_depth'] = reasoning_result['depth']
            unit.metadata['proofs_generated'] = len(reasoning_result.get('proofs', []))
            self.metrics.reasoning_cycles += len(unit.reasoning_cycles)
            
            # Phase 4: 8T/8M Optimization
            logger.info("Phase 4: Applying 8T/8M optimizations")
            unit.optimizations = self._apply_optimizations(
                unit.ontology,
                unit.constraints,
                unit.reasoning_cycles
            )
            self.metrics.optimizations_applied += len(unit.optimizations)
            
            # Phase 5: C Code Generation
            logger.info("Phase 5: Generating optimized C code")
            generation_context = {
                'ontology': unit.ontology,
                'constraints': unit.constraints,
                'reasoning': unit.reasoning_cycles,
                'optimizations': unit.optimizations,
                'target': self.config['cjinja']['target_architecture'],
                'memory_model': self.config['cjinja']['memory_model']
            }
            unit.generated_code = self.cjinja_compiler.generate(generation_context)
            unit.metadata['code_size'] = len(unit.generated_code)
            self.metrics.code_size += unit.metadata['code_size']
            
            # Phase 6: Telemetry Integration
            logger.info("Phase 6: Integrating observability and telemetry")
            unit.telemetry = self._integrate_telemetry(unit)
            self.metrics.telemetry_points += len(unit.telemetry.get('points', []))
            
            # Write generated code
            output_dir.mkdir(parents=True, exist_ok=True)
            unit.target_path.write_text(unit.generated_code)
            
            # Update status
            unit.status = "success"
            unit.compilation_time = time.time() - start_time
            self.metrics.successful_units += 1
            
            logger.info(f"Compilation successful: {unit.target_path} ({unit.compilation_time:.2f}s)")
            
        except Exception as e:
            unit.status = "failed"
            unit.errors.append(str(e))
            unit.compilation_time = time.time() - start_time
            self.metrics.failed_units += 1
            logger.error(f"Compilation failed: {str(e)}")
        
        # Update metrics
        self.metrics.total_units += 1
        self.metrics.total_time += unit.compilation_time
        
        # Store compilation unit
        self.compilation_units.append(unit)
        
        return unit
    
    def _extract_constraints(self, ontology: Dict[str, Any]) -> Dict[str, Any]:
        """Extract SHACL constraints from ontology"""
        constraints = {
            'shapes': [],
            'rules': [],
            'validations': []
        }
        
        # Extract from ontology annotations
        for cls in ontology.get('classes', []):
            if 'constraints' in cls:
                constraints['shapes'].append({
                    'target': cls['uri'],
                    'constraints': cls['constraints']
                })
        
        # Extract property constraints
        for prop in ontology.get('properties', []):
            if 'range' in prop or 'domain' in prop:
                constraints['rules'].append({
                    'property': prop['uri'],
                    'domain': prop.get('domain'),
                    'range': prop.get('range')
                })
        
        return constraints
    
    def _apply_optimizations(self, ontology: Dict, constraints: Dict, 
                           reasoning: List[Dict]) -> Dict[str, Any]:
        """Apply 8T/8M optimizations based on reasoning results"""
        optimizations = {
            'memory': [],
            'performance': [],
            'determinism': [],
            'hardware': []
        }
        
        # Memory optimizations (8M)
        # Analyze data structures and apply memory-efficient layouts
        for cls in ontology.get('classes', []):
            size_estimate = self._estimate_memory_size(cls)
            if size_estimate > 1024:  # Large structures
                optimizations['memory'].append({
                    'type': 'struct_packing',
                    'target': cls['uri'],
                    'optimization': 'aligned_layout',
                    'savings': f"{size_estimate * 0.2:.0f} bytes"
                })
        
        # Performance optimizations (8T)
        # Analyze reasoning cycles for hot paths
        hot_paths = self._identify_hot_paths(reasoning)
        for path in hot_paths:
            optimizations['performance'].append({
                'type': 'hot_path_optimization',
                'path': path['id'],
                'technique': 'branch_prediction',
                'expected_speedup': f"{path['frequency'] * 0.15:.1f}%"
            })
        
        # Determinism optimizations
        # Ensure all operations are deterministic
        for constraint in constraints.get('shapes', []):
            if 'ordering' not in constraint:
                optimizations['determinism'].append({
                    'type': 'ordering_constraint',
                    'target': constraint['target'],
                    'method': 'canonical_ordering'
                })
        
        # Hardware optimizations
        # Optimize for target architecture
        if self.config['cjinja']['target_architecture'] == '8T':
            optimizations['hardware'].append({
                'type': 'simd_vectorization',
                'operations': ['matrix_ops', 'graph_traversal'],
                'instruction_set': 'AVX2'
            })
        
        return optimizations
    
    def _estimate_memory_size(self, cls: Dict[str, Any]) -> int:
        """Estimate memory size for a class"""
        base_size = 8  # Base pointer size
        
        # Add property sizes
        for prop in cls.get('properties', []):
            prop_type = prop.get('type', 'object')
            if prop_type == 'string':
                base_size += 32  # Average string size
            elif prop_type == 'integer':
                base_size += 8
            elif prop_type == 'float':
                base_size += 8
            else:
                base_size += 16  # Reference size
        
        # Add overhead
        return base_size + 16
    
    def _identify_hot_paths(self, reasoning: List[Dict]) -> List[Dict[str, Any]]:
        """Identify hot paths in reasoning cycles"""
        path_frequency = {}
        
        for cycle in reasoning:
            for step in cycle.get('steps', []):
                path_id = step.get('path_id', 'unknown')
                path_frequency[path_id] = path_frequency.get(path_id, 0) + 1
        
        # Return paths executed more than average
        avg_frequency = sum(path_frequency.values()) / len(path_frequency) if path_frequency else 0
        hot_paths = []
        
        for path_id, frequency in path_frequency.items():
            if frequency > avg_frequency:
                hot_paths.append({
                    'id': path_id,
                    'frequency': frequency,
                    'percentage': (frequency / sum(path_frequency.values())) * 100
                })
        
        return sorted(hot_paths, key=lambda x: x['frequency'], reverse=True)
    
    def _integrate_telemetry(self, unit: CompilationUnit) -> Dict[str, Any]:
        """Integrate telemetry and observability"""
        telemetry = {
            'points': [],
            'metrics': [],
            'traces': [],
            'spans': []
        }
        
        # Add compilation metrics
        telemetry['metrics'].extend([
            {
                'name': 'compilation_time',
                'value': unit.compilation_time,
                'unit': 'seconds',
                'tags': {'unit_id': unit.id}
            },
            {
                'name': 'code_size',
                'value': unit.metadata.get('code_size', 0),
                'unit': 'bytes',
                'tags': {'unit_id': unit.id}
            },
            {
                'name': 'reasoning_cycles',
                'value': len(unit.reasoning_cycles),
                'unit': 'count',
                'tags': {'unit_id': unit.id}
            }
        ])
        
        # Add telemetry points for runtime monitoring
        for cls in unit.ontology.get('classes', []):
            telemetry['points'].append({
                'type': 'class_instantiation',
                'target': cls['uri'],
                'metrics': ['count', 'memory_usage', 'lifetime']
            })
        
        # Add tracing for reasoning paths
        for cycle in unit.reasoning_cycles:
            telemetry['traces'].append({
                'cycle_id': cycle.get('id'),
                'type': 'reasoning_trace',
                'steps': len(cycle.get('steps', [])),
                'depth': cycle.get('depth', 0)
            })
        
        return telemetry
    
    def compile_project(self, project_dir: Path, output_dir: Path) -> Dict[str, Any]:
        """Compile an entire project directory"""
        logger.info(f"Compiling project: {project_dir}")
        start_time = time.time()
        
        # Find all specification files
        spec_files = list(project_dir.glob("**/*.ttl")) + \
                    list(project_dir.glob("**/*.owl")) + \
                    list(project_dir.glob("**/*.n3"))
        
        logger.info(f"Found {len(spec_files)} specification files")
        
        # Compile each specification
        results = []
        for spec_file in spec_files:
            relative_path = spec_file.relative_to(project_dir)
            output_subdir = output_dir / relative_path.parent
            
            unit = self.compile_specification(spec_file, output_subdir)
            results.append({
                'source': str(spec_file),
                'target': str(unit.target_path),
                'status': unit.status,
                'time': unit.compilation_time,
                'errors': unit.errors,
                'warnings': unit.warnings
            })
        
        # Generate project summary
        total_time = time.time() - start_time
        summary = {
            'session_id': self.session_id,
            'project': str(project_dir),
            'output': str(output_dir),
            'total_files': len(spec_files),
            'successful': self.metrics.successful_units,
            'failed': self.metrics.failed_units,
            'total_time': total_time,
            'metrics': {
                'reasoning_cycles': self.metrics.reasoning_cycles,
                'optimizations': self.metrics.optimizations_applied,
                'constraints': self.metrics.constraints_validated,
                'code_size': self.metrics.code_size,
                'telemetry_points': self.metrics.telemetry_points
            },
            'results': results,
            'timestamp': datetime.now().isoformat()
        }
        
        # Write summary
        summary_path = output_dir / f"compilation_summary_{self.session_id}.json"
        summary_path.write_text(json.dumps(summary, indent=2))
        
        logger.info(f"Project compilation complete: {self.metrics.successful_units}/{len(spec_files)} successful")
        
        return summary
    
    def generate_build_script(self, output_dir: Path) -> Path:
        """Generate a build script for the compiled C code"""
        build_script = output_dir / "build.sh"
        
        script_content = f"""#!/bin/bash
# AOT Compiled Project Build Script
# Generated by AOT Compiler Production Pipeline
# Session: {self.session_id}

set -e

echo "Building AOT compiled project..."

# Configuration
CC="${{CC:-gcc}}"
CFLAGS="${{CFLAGS:--O3 -Wall -Wextra -std=c11}}"
LDFLAGS="${{LDFLAGS:--lpthread -lm}}"
OUTPUT_DIR="build"

# Create build directory
mkdir -p "$OUTPUT_DIR"

# Find all generated C files
C_FILES=$(find . -name "*.c" -type f)

echo "Found $(echo "$C_FILES" | wc -l) C files"

# Compile each file
for file in $C_FILES; do
    obj_file="$OUTPUT_DIR/$(basename "$file" .c).o"
    echo "Compiling $file -> $obj_file"
    $CC $CFLAGS -c "$file" -o "$obj_file"
done

# Link all object files
echo "Linking executable..."
$CC $OUTPUT_DIR/*.o $LDFLAGS -o "$OUTPUT_DIR/aot_executable"

echo "Build complete: $OUTPUT_DIR/aot_executable"
echo "Compilation metrics:"
echo "  - Total code size: {self.metrics.code_size} bytes"
echo "  - Optimizations applied: {self.metrics.optimizations_applied}"
echo "  - Telemetry points: {self.metrics.telemetry_points}"
"""
        
        build_script.write_text(script_content)
        build_script.chmod(0o755)
        
        logger.info(f"Generated build script: {build_script}")
        return build_script
    
    def export_compilation_graph(self, output_path: Path) -> None:
        """Export the compilation dependency graph"""
        graph = {
            'nodes': [],
            'edges': [],
            'metadata': {
                'session_id': self.session_id,
                'total_units': len(self.compilation_units),
                'timestamp': datetime.now().isoformat()
            }
        }
        
        # Add nodes for each compilation unit
        for unit in self.compilation_units:
            graph['nodes'].append({
                'id': unit.id,
                'label': unit.source_path.name,
                'type': 'compilation_unit',
                'status': unit.status,
                'metrics': {
                    'time': unit.compilation_time,
                    'code_size': unit.metadata.get('code_size', 0),
                    'reasoning_cycles': len(unit.reasoning_cycles)
                }
            })
        
        # Add edges based on dependencies (if any)
        # This would be extended with actual dependency analysis
        
        # Write graph
        output_path.write_text(json.dumps(graph, indent=2))
        logger.info(f"Exported compilation graph: {output_path}")


def main():
    """Main entry point for the AOT compiler"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="AOT Compiler Production Pipeline - The Reasoner IS the Build System"
    )
    parser.add_argument("input", type=Path, help="Input specification file or directory")
    parser.add_argument("output", type=Path, help="Output directory for generated code")
    parser.add_argument("--config", type=Path, help="Configuration file (JSON)")
    parser.add_argument("--parallel", action="store_true", help="Enable parallel compilation")
    parser.add_argument("--cache", action="store_true", help="Enable compilation cache")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    
    args = parser.parse_args()
    
    # Set logging level
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    # Load configuration
    config = None
    if args.config and args.config.exists():
        config = json.loads(args.config.read_text())
    
    # Create compiler
    compiler = AOTCompilerProduction(config)
    
    # Enable options
    if args.parallel:
        compiler.config['pipeline']['parallel_compilation'] = True
    if args.cache:
        compiler.config['pipeline']['cache_enabled'] = True
    
    # Compile based on input type
    if args.input.is_file():
        # Single file compilation
        unit = compiler.compile_specification(args.input, args.output)
        if unit.status == "success":
            print(f"✓ Compilation successful: {unit.target_path}")
        else:
            print(f"✗ Compilation failed: {', '.join(unit.errors)}")
            sys.exit(1)
    else:
        # Project compilation
        summary = compiler.compile_project(args.input, args.output)
        
        # Generate build script
        compiler.generate_build_script(args.output)
        
        # Export compilation graph
        compiler.export_compilation_graph(args.output / "compilation_graph.json")
        
        # Print summary
        print(f"\nCompilation Summary:")
        print(f"  Session: {summary['session_id']}")
        print(f"  Total files: {summary['total_files']}")
        print(f"  Successful: {summary['successful']}")
        print(f"  Failed: {summary['failed']}")
        print(f"  Total time: {summary['total_time']:.2f}s")
        print(f"\nMetrics:")
        for key, value in summary['metrics'].items():
            print(f"  {key}: {value}")
        
        if summary['failed'] > 0:
            sys.exit(1)


if __name__ == "__main__":
    main()