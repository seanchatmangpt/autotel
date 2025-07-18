"""
Adaptive JIT Daemon Python wrapper
"""

import ctypes
import os
from pathlib import Path

class AdaptiveJITDaemon:
    """
    Python interface to the Adaptive JIT Daemon.
    Automatically detects hot query patterns and generates optimized kernels.
    """
    
    def __init__(self, hot_threshold=100, max_cache_size=128,
                 max_code_bytes=10*1024*1024, compile_threads=2):
        # Load C library
        lib_path = Path(__file__).parent / "lib" / "libpico_jit.so"
        if not lib_path.exists():
            os.system(f"cd {Path(__file__).parent} && make libpico_jit.so")
            
        self.lib = ctypes.CDLL(str(lib_path))
        
        # Define structures
        class QueryShape(ctypes.Structure):
            _fields_ = [
                ("pattern_ids", ctypes.c_uint32 * 16),
                ("pattern_count", ctypes.c_uint8),
                ("hash", ctypes.c_uint64)
            ]
        
        class JitConfig(ctypes.Structure):
            _fields_ = [
                ("hot_threshold", ctypes.c_uint32),
                ("max_cache_size", ctypes.c_size_t),
                ("max_code_bytes", ctypes.c_size_t),
                ("compile_threads", ctypes.c_uint32),
                ("compiler_flags", ctypes.c_char * 256)
            ]
        
        # Define functions
        self.lib.jit_create.argtypes = [ctypes.POINTER(JitConfig)]
        self.lib.jit_create.restype = ctypes.c_void_p
        
        self.lib.jit_record_execution.argtypes = [ctypes.c_void_p,
                                                  ctypes.POINTER(QueryShape),
                                                  ctypes.c_uint64]
        
        self.lib.jit_get_stats.argtypes = [ctypes.c_void_p, ctypes.c_char_p,
                                           ctypes.c_size_t]
        
        self.lib.jit_destroy.argtypes = [ctypes.c_void_p]
        
        # Create daemon
        config = JitConfig(
            hot_threshold=hot_threshold,
            max_cache_size=max_cache_size,
            max_code_bytes=max_code_bytes,
            compile_threads=compile_threads
        )
        
        # Set compiler flags
        flags = b"-O3 -march=native -fPIC -shared"
        config.compiler_flags[:len(flags)] = flags
        
        self.daemon = self.lib.jit_create(ctypes.byref(config))
        self.QueryShape = QueryShape
    
    def record_execution(self, pattern_ids, cycles):
        """
        Record execution of a query shape.
        
        Args:
            pattern_ids: List of pattern IDs in the query
            cycles: Execution time in cycles
        """
        shape = self.QueryShape()
        shape.pattern_count = len(pattern_ids)
        for i, pid in enumerate(pattern_ids[:16]):
            shape.pattern_ids[i] = pid
        
        self.lib.jit_record_execution(self.daemon, ctypes.byref(shape), cycles)
    
    def get_stats(self):
        """Get JIT daemon statistics"""
        buf = ctypes.create_string_buffer(1024)
        self.lib.jit_get_stats(self.daemon, buf, 1024)
        return buf.value.decode('utf-8')
    
    def __del__(self):
        """Clean up C resources"""
        if hasattr(self, 'daemon') and self.daemon:
            self.lib.jit_destroy(self.daemon)
