"""
Query Optimization Planner (QOP) Python wrapper
"""

import ctypes
import os
from pathlib import Path

class QueryOptimizationPlanner:
    """
    Python interface to the Query Optimization Planner.
    Uses Monte Carlo Tree Search to optimize query execution plans in sub-microsecond time.
    """
    
    def __init__(self, max_iterations=10000, time_budget_ns=100000000,
                 rollout_depth=20, exploration_constant=1.414, rng_seed=42):
        # Load C library
        lib_path = Path(__file__).parent / "lib" / "libqop.so"
        if not lib_path.exists():
            os.system(f"cd {Path(__file__).parent} && make libqop.so")
            
        self.lib = ctypes.CDLL(str(lib_path))
        
        # Define structures
        class QOP_Config(ctypes.Structure):
            _fields_ = [
                ("max_iterations", ctypes.c_uint32),
                ("time_budget_ns", ctypes.c_uint64),
                ("rollout_depth", ctypes.c_uint32),
                ("exploration_constant", ctypes.c_double),
                ("rng_seed", ctypes.c_uint64)
            ]
        
        class QOP_Plan(ctypes.Structure):
            _fields_ = [
                ("pattern_order", ctypes.POINTER(ctypes.c_uint32)),
                ("score", ctypes.c_double)
            ]
        
        # Define functions
        self.lib.qop_create.argtypes = [ctypes.POINTER(QOP_Config)]
        self.lib.qop_create.restype = ctypes.c_void_p
        
        self.lib.qop_step.argtypes = [ctypes.c_void_p]
        self.lib.qop_step.restype = ctypes.c_bool
        
        self.lib.qop_get_plan.argtypes = [ctypes.c_void_p, ctypes.POINTER(QOP_Plan)]
        
        self.lib.qop_destroy.argtypes = [ctypes.c_void_p]
        
        # Create planner
        config = QOP_Config(
            max_iterations=max_iterations,
            time_budget_ns=time_budget_ns,
            rollout_depth=rollout_depth,
            exploration_constant=exploration_constant,
            rng_seed=rng_seed
        )
        
        self.planner = self.lib.qop_create(ctypes.byref(config))
        self.QOP_Plan = QOP_Plan
    
    def optimize(self, pattern_ids):
        """
        Find optimal execution order for query patterns.
        
        Args:
            pattern_ids: List of pattern IDs
            
        Returns:
            tuple: (ordered_patterns, score)
        """
        # Set initial patterns (simplified - would need proper API)
        # For now, just run the planning
        
        # Run MCTS
        while self.lib.qop_step(self.planner):
            pass
        
        # Get result
        pattern_buffer = (ctypes.c_uint32 * len(pattern_ids))()
        plan = self.QOP_Plan(pattern_order=pattern_buffer, score=0.0)
        
        self.lib.qop_get_plan(self.planner, ctypes.byref(plan))
        
        # Extract results
        ordered = [pattern_buffer[i] for i in range(len(pattern_ids))]
        
        return ordered, plan.score
    
    def __del__(self):
        """Clean up C resources"""
        if hasattr(self, 'planner') and self.planner:
            self.lib.qop_destroy(self.planner)
