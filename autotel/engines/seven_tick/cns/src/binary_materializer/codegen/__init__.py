"""
CNS AOT Compiler Pipeline
"The Reasoner IS the Build System"

A comprehensive Ahead-of-Time compiler that transforms semantic specifications
into optimized, deterministic C code using Eightfold Path reasoning methodology.
"""

__version__ = "1.0.0"
__author__ = "CNS Development Team"
__description__ = "AOT Compiler Pipeline with 8H Reasoning"

# Import main components
from .aot_compiler_production import AOTCompilerProduction, CompilationUnit, CompilationMetrics
from .owl_aot_compiler import OWLCompiler, OWLClass, OWLProperty
from .cjinja_aot_compiler import CJinjaCompiler, CCodeBlock, GenerationContext
from .shacl_validator import SHACLValidator, ValidationRule, ValidationResult
from .reasoning_engine import ReasoningEngine, ReasoningCycle, ReasoningStep, EightfoldStage

# Export public API
__all__ = [
    # Main compiler
    'AOTCompilerProduction',
    'CompilationUnit', 
    'CompilationMetrics',
    
    # OWL processing
    'OWLCompiler',
    'OWLClass',
    'OWLProperty',
    
    # C code generation
    'CJinjaCompiler',
    'CCodeBlock',
    'GenerationContext',
    
    # SHACL validation
    'SHACLValidator',
    'ValidationRule',
    'ValidationResult',
    
    # 8H reasoning
    'ReasoningEngine',
    'ReasoningCycle',
    'ReasoningStep',
    'EightfoldStage',
    
    # Version info
    '__version__',
    '__author__',
    '__description__'
]

# Pipeline configuration defaults
DEFAULT_CONFIG = {
    'owl': {
        'strict_mode': True,
        'inference_enabled': True,
        'reasoning_depth': 3,
        'eightfold_integration': True
    },
    'shacl': {
        'validation_level': 'strict',
        'constraint_optimization': True,
        'generate_validation_code': True
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
        'deterministic_output': True
    }
}

def create_pipeline(config=None):
    """Create a configured AOT compiler pipeline"""
    if config is None:
        config = DEFAULT_CONFIG.copy()
    else:
        # Merge with defaults
        merged_config = DEFAULT_CONFIG.copy()
        for section, settings in config.items():
            if section in merged_config:
                merged_config[section].update(settings)
            else:
                merged_config[section] = settings
        config = merged_config
    
    return AOTCompilerProduction(config)

def quick_compile(input_path, output_path, **kwargs):
    """Quick compilation utility function"""
    from pathlib import Path
    
    pipeline = create_pipeline(kwargs.get('config'))
    
    input_path = Path(input_path)
    output_path = Path(output_path)
    
    if input_path.is_file():
        # Single file compilation
        unit = pipeline.compile_specification(input_path, output_path)
        return {'status': unit.status, 'unit': unit}
    else:
        # Project compilation
        summary = pipeline.compile_project(input_path, output_path)
        return {'status': 'success' if summary['failed'] == 0 else 'partial', 'summary': summary}

# Convenience functions
def compile_ontology(ttl_file, output_dir, **kwargs):
    """Compile a single TTL/OWL ontology file"""
    return quick_compile(ttl_file, output_dir, **kwargs)

def compile_project(project_dir, build_dir, **kwargs):
    """Compile an entire project directory"""
    return quick_compile(project_dir, build_dir, **kwargs)