"""
Seven Tick Semantic Web Python Interface

High-performance SPARQL, SHACL, and OWL operations.
"""

import ctypes
from pathlib import Path
from typing import List, Tuple, Optional, Set
from dataclasses import dataclass
from sparql7t import TriplePattern, S7TEngine
from shacl7t import CompiledShape, ShaclEngine

# Load additional C libraries
lib_path = Path(__file__).parent / "c_src"
joins_lib = ctypes.CDLL(str(lib_path / "libsparql_joins.so"))
owl_lib = ctypes.CDLL(str(lib_path / "libow17t.so"))


@dataclass
class JoinPattern:
    """Represents a join pattern with variable bindings."""
    pattern: TriplePattern
    var_bindings: List[Tuple[int, int]]  # (var_id, position)


class JoinResult(ctypes.Structure):
    """Result of a join operation."""
    _fields_ = [
        ("result_vector", ctypes.POINTER(ctypes.c_uint64)),
        ("cardinality", ctypes.c_size_t)
    ]


class SPARQLJoins:
    """High-performance SPARQL join operations."""
    
    def __init__(self, engine: S7TEngine):
        self.engine = engine
        self._setup_functions()
        
    def _setup_functions(self):
        """Configure ctypes function signatures."""
        # s7t_get_subject_vector
        self._get_subject_vector = joins_lib.s7t_get_subject_vector
        self._get_subject_vector.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32
        ]
        self._get_subject_vector.restype = ctypes.POINTER(JoinResult)
        
        # s7t_intersect
        self._intersect = joins_lib.s7t_intersect
        self._intersect.argtypes = [
            ctypes.POINTER(JoinResult), ctypes.POINTER(JoinResult)
        ]
        self._intersect.restype = ctypes.POINTER(JoinResult)
        
        # s7t_union
        self._union = joins_lib.s7t_union
        self._union.argtypes = [
            ctypes.POINTER(JoinResult), ctypes.POINTER(JoinResult)
        ]
        self._union.restype = ctypes.POINTER(JoinResult)
        
        # s7t_free_result
        self._free_result = joins_lib.s7t_free_result
        self._free_result.argtypes = [ctypes.POINTER(JoinResult)]
        
    def get_subjects_for_po(self, predicate: int, obj: int) -> Set[int]:
        """Get all subjects that have the given predicate-object pair."""
        result = self._get_subject_vector(self.engine._engine, predicate, obj)
        subjects = self._decode_bitvector(result)
        self._free_result(result)
        return subjects
        
    def join_patterns(self, patterns: List[TriplePattern]) -> List[int]:
        """Perform multi-way join on patterns."""
        if not patterns:
            return []
            
        # Start with first pattern
        first = patterns[0]
        current = self._get_subject_vector(
            self.engine._engine, first.p, first.o
        )
        
        # Intersect with remaining patterns
        for pattern in patterns[1:]:
            next_result = self._get_subject_vector(
                self.engine._engine, pattern.p, pattern.o
            )
            intersected = self._intersect(current, next_result)
            
            self._free_result(current)
            self._free_result(next_result)
            current = intersected
            
            # Early termination if empty
            if current.contents.cardinality == 0:
                break
                
        # Decode final result
        subjects = list(self._decode_bitvector(current))
        self._free_result(current)
        return subjects
        
    def _decode_bitvector(self, result: ctypes.POINTER(JoinResult)) -> Set[int]:
        """Decode bit-vector to subject IDs."""
        subjects = set()
        if not result:
            return subjects
            
        # Calculate number of chunks
        max_subjects = self.engine.max_subjects
        num_chunks = (max_subjects + 63) // 64
        
        for chunk_idx in range(num_chunks):
            chunk = result.contents.result_vector[chunk_idx]
            bit_idx = 0
            
            while chunk != 0:
                # Find next set bit
                trailing_zeros = (chunk & -chunk).bit_length() - 1
                subject_id = chunk_idx * 64 + trailing_zeros
                subjects.add(subject_id)
                
                # Clear the bit
                chunk &= chunk - 1
                
        return subjects


@dataclass 
class OWLAxiom:
    """Represents an OWL axiom."""
    subject: int
    predicate: int
    object: int
    axiom_type: str


class OWLEngine:
    """OWL reasoning engine with bit-vector operations."""
    
    OWL_SUBCLASS_OF = 1 << 0
    OWL_EQUIVALENT_CLASS = 1 << 1
    OWL_DISJOINT_WITH = 1 << 2
    OWL_SUBPROPERTY_OF = 1 << 3
    OWL_TRANSITIVE = 1 << 7
    OWL_SYMMETRIC = 1 << 8
    OWL_FUNCTIONAL = 1 << 5
    
    def __init__(self, sparql_engine: S7TEngine, max_classes: int = 10000,
                 max_properties: int = 1000):
        self.sparql_engine = sparql_engine
        self.max_classes = max_classes
        self.max_properties = max_properties
        self._setup_engine()
        
    def _setup_engine(self):
        """Initialize OWL engine."""
        # Create OWL engine
        create_fn = owl_lib.owl_create
        create_fn.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t]
        create_fn.restype = ctypes.c_void_p
        
        self._engine = create_fn(
            self.sparql_engine._engine, self.max_classes, self.max_properties
        )
        
        # Setup other functions
        self._add_subclass = owl_lib.owl_add_subclass
        self._add_subclass.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
        
        self._set_transitive = owl_lib.owl_set_transitive
        self._set_transitive.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
        
        self._compute_closures = owl_lib.owl_compute_closures
        self._compute_closures.argtypes = [ctypes.c_void_p]
        
        self._ask_with_reasoning = owl_lib.owl_ask_with_reasoning
        self._ask_with_reasoning.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32
        ]
        self._ask_with_reasoning.restype = ctypes.c_int
        
    def add_subclass(self, subclass: int, superclass: int):
        """Add subclass relationship."""
        self._add_subclass(self._engine, subclass, superclass)
        
    def add_equivalent_class(self, class1: int, class2: int):
        """Add equivalent class relationship."""
        # Implement as mutual subclass
        self._add_subclass(self._engine, class1, class2)
        self._add_subclass(self._engine, class2, class1)
        
    def set_transitive_property(self, property_id: int):
        """Mark property as transitive."""
        self._set_transitive(self._engine, property_id)
        
    def compute_closures(self):
        """Compute transitive closures for reasoning."""
        self._compute_closures(self._engine)
        
    def ask_with_reasoning(self, subject: int, predicate: int, obj: int) -> bool:
        """Query with OWL reasoning."""
        return bool(self._ask_with_reasoning(self._engine, subject, predicate, obj))
        
    def get_all_superclasses(self, class_id: int) -> Set[int]:
        """Get all superclasses including inferred ones."""
        # Allocate bit-vector
        chunks = (self.max_classes + 63) // 64
        result_vector = (ctypes.c_uint64 * chunks)()
        
        get_fn = owl_lib.owl_get_all_superclasses
        get_fn.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint64)]
        get_fn(self._engine, class_id, result_vector)
        
        # Decode bit-vector
        superclasses = set()
        for chunk_idx in range(chunks):
            chunk = result_vector[chunk_idx]
            bit_idx = 0
            
            while chunk != 0:
                trailing_zeros = (chunk & -chunk).bit_length() - 1
                class_id = chunk_idx * 64 + trailing_zeros
                superclasses.add(class_id)
                chunk &= chunk - 1
                
        return superclasses
        
    def __del__(self):
        """Clean up OWL engine."""
        if hasattr(self, '_engine') and self._engine:
            destroy_fn = owl_lib.owl_destroy
            destroy_fn.argtypes = [ctypes.c_void_p]
            destroy_fn(self._engine)


class SemanticEngine:
    """Integrated semantic web engine combining SPARQL, SHACL, and OWL."""
    
    def __init__(self, max_subjects: int = 1000000):
        self.sparql = S7TEngine(max_subjects, 100, max_subjects)
        self.shacl = ShaclEngine(max_subjects, 64)
        self.owl = OWLEngine(self.sparql)
        self.joins = SPARQLJoins(self.sparql)
        
        # Namespace mappings
        self.namespaces = {
            'rdf': 0,
            'rdfs': 100,
            'owl': 200,
            'sh': 300
        }
        
        # Predicate mappings
        self.predicates = {
            'rdf:type': 0,
            'rdfs:subClassOf': 101,
            'owl:equivalentClass': 201,
            'owl:TransitiveProperty': 202
        }
        
    def add_triple(self, s: int, p: int, o: int):
        """Add triple to the knowledge base."""
        self.sparql.add_triple(s, p, o)
        
        # Handle special predicates
        if p == self.predicates['rdfs:subClassOf']:
            self.owl.add_subclass(s, o)
        elif p == self.predicates['owl:equivalentClass']:
            self.owl.add_equivalent_class(s, o)
            
    def add_shape(self, shape_id: int, target_class: int, 
                  required_properties: List[int]):
        """Add SHACL shape."""
        shape = CompiledShape()
        shape.target_class_mask = 1 << target_class
        
        prop_mask = 0
        for prop in required_properties:
            prop_mask |= 1 << prop
        shape.property_mask = prop_mask
        
        shape.constraint_flags = (
            self.shacl.SHACL_TARGET_CLASS | self.shacl.SHACL_PROPERTY
        )
        
        self.shacl.add_shape(shape_id, shape)
        
    def query_with_reasoning(self, patterns: List[TriplePattern]) -> List[int]:
        """Execute SPARQL query with OWL reasoning."""
        # First compute OWL closures
        self.owl.compute_closures()
        
        # Then execute join with reasoning
        results = []
        for pattern in patterns:
            if pattern.p == 0:  # rdf:type with reasoning
                subjects = set()
                for s in range(self.sparql.max_subjects):
                    if self.owl.ask_with_reasoning(s, pattern.p, pattern.o):
                        subjects.add(s)
                results.append(subjects)
            else:
                # Regular pattern
                subjects = self.joins.get_subjects_for_po(pattern.p, pattern.o)
                results.append(subjects)
                
        # Intersect all results
        if results:
            final = results[0]
            for r in results[1:]:
                final = final.intersection(r)
            return list(final)
        return []
        
    def validate_graph(self) -> Tuple[int, int]:
        """Validate entire graph with SHACL."""
        valid_count = 0
        total_count = 0
        
        for node in range(self.sparql.max_subjects):
            # Check all shapes
            for shape_id in range(self.shacl.max_shapes):
                if self.shacl.validate_node(node, shape_id):
                    valid_count += 1
                total_count += 1
                
        return valid_count, total_count


# Example usage
if __name__ == "__main__":
    # Create semantic engine
    engine = SemanticEngine(max_subjects=10000)
    
    # Define classes and properties
    EMPLOYEE = 100
    MANAGER = 101
    DIRECTOR = 102
    HAS_DEPT = 10
    HAS_SALARY = 11
    
    # Add ontology
    engine.add_triple(MANAGER, engine.predicates['rdfs:subClassOf'], EMPLOYEE)
    engine.add_triple(DIRECTOR, engine.predicates['rdfs:subClassOf'], MANAGER)
    
    # Add instance data
    for i in range(1000):
        engine.add_triple(i, engine.predicates['rdf:type'], EMPLOYEE)
        engine.add_triple(i, HAS_DEPT, 200 + (i % 10))
        engine.add_triple(i, HAS_SALARY, 50000 + i * 1000)
        
        if i % 100 == 0:
            engine.add_triple(i, engine.predicates['rdf:type'], MANAGER)
        if i % 500 == 0:
            engine.add_triple(i, engine.predicates['rdf:type'], DIRECTOR)
            
    # Add SHACL constraint
    engine.add_shape(0, EMPLOYEE, [HAS_DEPT, HAS_SALARY])
    
    # Query with reasoning
    patterns = [
        TriplePattern(0, engine.predicates['rdf:type'], EMPLOYEE),
        TriplePattern(0, HAS_DEPT, 205)
    ]
    
    results = engine.query_with_reasoning(patterns)
    print(f"Found {len(results)} employees in department 205")
    
    # Validate
    valid, total = engine.validate_graph()
    print(f"Validation: {valid}/{total} valid")