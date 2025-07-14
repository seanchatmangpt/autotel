"""
AutoTel Factory Processors

Isolated processors for each DSL (BPMN, DMN, DSPy, SHACL, OWL) that parse
their respective XML elements into trusted, executable objects.
"""

from .bpmn_processor import BPMNProcessor
from .dmn_processor import DMNProcessor
from .dspy_processor import DSPyProcessor, DSPySignatureDefinition
from .shacl_processor import SHACLProcessor
from .owl_processor import OWLProcessor

__all__ = [
    'BPMNProcessor',
    'DMNProcessor',
    'DSPyProcessor', 
    'DSPySignatureDefinition',
    'SHACLProcessor',
    'OWLProcessor'
] 