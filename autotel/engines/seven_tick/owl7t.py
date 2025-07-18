"""
OWL-7T Python wrapper - Compile OWL ontologies to 7-tick operations
"""

import ctypes
import os
from pathlib import Path

class OWL7TCompiler:
    """
    Python interface to the OWL-7T compiler.
    Compiles OWL axioms to bit-mask operations that execute in ≤7 CPU cycles.
    """
    
    def __init__(self):
        # Load C library
        lib_path = Path(__file__).parent / "lib" / "libow17t.so"
        if not lib_path.exists():
            os.system(f"cd {Path(__file__).parent} && make libow17t.so")
            
        self.lib = ctypes.CDLL(str(lib_path))
        
        # Define C structures
        class CompiledAxiom(ctypes.Structure):
            _fields_ = [
                ("type", ctypes.c_int),
                ("subject_id", ctypes.c_uint32),
                ("object_id", ctypes.c_uint32),
                ("mask", ctypes.c_uint64),
                ("tick_cost", ctypes.c_uint8)
            ]
        
        class OWL7T_CompileResult(ctypes.Structure):
            _fields_ = [
                ("axioms", ctypes.POINTER(CompiledAxiom)),
                ("axiom_count", ctypes.c_size_t),
                ("class_masks", ctypes.POINTER(ctypes.c_uint64)),
                ("property_masks", ctypes.POINTER(ctypes.c_uint64)),
                ("class_count", ctypes.c_size_t),
                ("property_count", ctypes.c_size_t),
                ("tick_compliant_count", ctypes.c_uint32),
                ("materialized_count", ctypes.c_uint32),
                ("rejected_count", ctypes.c_uint32)
            ]
        
        self.CompiledAxiom = CompiledAxiom
        self.OWL7T_CompileResult = OWL7T_CompileResult
        
        # Define functions
        self.lib.owl7t_compile.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.lib.owl7t_compile.restype = ctypes.POINTER(OWL7T_CompileResult)
        
        self.lib.owl7t_check_subclass.argtypes = [ctypes.POINTER(OWL7T_CompileResult),
                                                  ctypes.c_uint32, ctypes.c_uint32]
        self.lib.owl7t_check_subclass.restype = ctypes.c_bool
        
        self.lib.owl7t_generate_c_code.argtypes = [ctypes.POINTER(OWL7T_CompileResult),
                                                   ctypes.c_char_p]
        self.lib.owl7t_generate_c_code.restype = ctypes.c_int
        
        self.lib.owl7t_free_result.argtypes = [ctypes.POINTER(OWL7T_CompileResult)]
        
        self.current_result = None
    
    def compile(self, ontology_path, output_dir="."):
        """
        Compile an OWL ontology to 7-tick operations.
        
        Args:
            ontology_path (str): Path to OWL file (Turtle format)
            output_dir (str): Directory for generated C code
            
        Returns:
            dict: Compilation statistics
        """
        # Free previous result if any
        if self.current_result:
            self.lib.owl7t_free_result(self.current_result)
        
        # Compile
        self.current_result = self.lib.owl7t_compile(
            ontology_path.encode('utf-8'),
            output_dir.encode('utf-8')
        )
        
        if not self.current_result:
            raise RuntimeError("Compilation failed")
        
        result = self.current_result.contents
        
        return {
            'axiom_count': result.axiom_count,
            'tick_compliant_count': result.tick_compliant_count,
            'materialized_count': result.materialized_count,
            'rejected_count': result.rejected_count,
            'class_count': result.class_count,
            'property_count': result.property_count
        }
    
    def check_subclass(self, child_id, parent_id):
        """
        Check if child is a subclass of parent.
        Executes in ≤7 CPU cycles.
        
        Args:
            child_id (int): Child class ID
            parent_id (int): Parent class ID
            
        Returns:
            bool: True if subclass relationship exists
        """
        if not self.current_result:
            raise RuntimeError("No ontology compiled")
            
        return self.lib.owl7t_check_subclass(self.current_result, child_id, parent_id)
    
    def generate_c_code(self, output_dir="."):
        """
        Generate C code for the compiled ontology.
        
        Args:
            output_dir (str): Output directory
            
        Returns:
            bool: True if successful
        """
        if not self.current_result:
            raise RuntimeError("No ontology compiled")
            
        result = self.lib.owl7t_generate_c_code(
            self.current_result,
            output_dir.encode('utf-8')
        )
        
        return result == 0
    
    def __del__(self):
        """Clean up C resources"""
        if hasattr(self, 'current_result') and self.current_result:
            self.lib.owl7t_free_result(self.current_result)
