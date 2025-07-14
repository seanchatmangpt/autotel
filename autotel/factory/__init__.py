"""
AutoTel Factory - Semantic Factory Pipeline

This module provides the core "compiler" that safely transforms multi-pillar XML 
into a trusted, executable object. The factory implements isolated processors for
each DSL (BPMN, DMN, DSPy, SHACL, OWL) and a semantic linker that combines them.
"""

from .pipeline import AutoTelFactory
from .processors.bpmn_processor import BPMNProcessor
from .processors.dmn_processor import DMNProcessor
from .processors.dspy_processor import DSPyProcessor, DSPySignatureDefinition
from .processors.shacl_processor import SHACLProcessor
from .processors.owl_processor import OWLProcessor
from .linker import SemanticLinker, SemanticLinkerError

__all__ = [
    'AutoTelFactory',
    'BPMNProcessor',
    'DMNProcessor', 
    'DSPyProcessor',
    'DSPySignatureDefinition',
    'SHACLProcessor',
    'OWLProcessor',
    'SemanticLinker',
    'SemanticLinkerError'
] 