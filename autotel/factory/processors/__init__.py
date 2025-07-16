"""
AutoTel Factory Processors

Isolated processors for each DSL (BPMN, DMN, DSPy, SHACL, OWL, Jinja) that parse
their respective XML elements into trusted, executable objects.
"""

from .bpmn_processor import BPMNProcessor
from .dmn_processor import DMNProcessor
from .dspy_processor import DSPyProcessor, DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration
from .shacl_processor import SHACLProcessor
from .owl_processor import OWLProcessor
from .jinja_processor import JinjaProcessor

__all__ = [
    'BPMNProcessor',
    'DMNProcessor',
    'DSPyProcessor', 
    'DSPySignatureDefinition',
    'DSPyModuleDefinition',
    'DSPyModelConfiguration',
    'SHACLProcessor',
    'OWLProcessor',
    'JinjaProcessor'
] 