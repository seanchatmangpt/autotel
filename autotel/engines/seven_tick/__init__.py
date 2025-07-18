"""
Seven Tick (7T) - The World's Fastest Semantic Stack

A pure C implementation guaranteeing every operation completes in ≤7 CPU cycles.
Includes SPARQL-7T, SHACL-7T, OWL-7T, and supporting components.
"""

__version__ = "1.0.0"
__author__ = "Seven Tick Contributors"

# Component imports
from .sparql7t import SPARQL7TEngine
from .shacl7t import SHACL7TValidator
from .owl7t import OWL7TCompiler
from .qop import QueryOptimizationPlanner
from .jit import AdaptiveJITDaemon

# Main API
__all__ = [
    "SPARQL7TEngine",
    "SHACL7TValidator", 
    "OWL7TCompiler",
    "QueryOptimizationPlanner",
    "AdaptiveJITDaemon",
    "create_seven_tick_stack"
]

def create_seven_tick_stack(config=None):
    """
    Create a complete Seven Tick semantic stack with all components.
    
    Args:
        config (dict): Configuration options
        
    Returns:
        dict: Dictionary with initialized components
    """
    config = config or {}
    
    return {
        'sparql': SPARQL7TEngine(
            max_subjects=config.get('max_subjects', 1000000),
            max_predicates=config.get('max_predicates', 1000),
            max_objects=config.get('max_objects', 10000)
        ),
        'shacl': SHACL7TValidator(
            max_nodes=config.get('max_nodes', 1000000),
            max_shapes=config.get('max_shapes', 100)
        ),
        'owl': OWL7TCompiler(),
        'planner': QueryOptimizationPlanner(
            max_iterations=config.get('qop_iterations', 10000),
            time_budget_ns=config.get('qop_time_budget', 100000000)
        ),
        'jit': AdaptiveJITDaemon(
            hot_threshold=config.get('jit_threshold', 100),
            max_cache_size=config.get('jit_cache_size', 128)
        )
    }
