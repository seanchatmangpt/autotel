"""
AutoTel Factory - Semantic Factory Pipeline

This module provides the core "compiler" that safely transforms multi-pillar XML 
into a trusted, executable object. The factory implements isolated processors for
each DSL (BPMN, DMN, DSPy, SHACL, OWL) and a semantic linker that combines them.
"""

from .pipeline import PipelineOrchestrator
from autotel.processors.bpmn_processor import BPMNProcessor
from autotel.processors.dmn_processor import DMNProcessor
from autotel.processors.dspy_processor import DSPyProcessor
from autotel.processors.shacl_processor import SHACLProcessor
from autotel.processors.owl_processor import OWLProcessor
from autotel.processors.otel_processor import OTELProcessor
from .linker import SemanticLinker
from .dspy_compiler import DSPyCompiler, DSPySignature

__all__ = [
    'PipelineOrchestrator',
    'BPMNProcessor',
    'DMNProcessor', 
    'DSPyProcessor',
    'SHACLProcessor',
    'OWLProcessor',
    'OTELProcessor',
    'SemanticLinker',
    'DSPyCompiler',
    'DSPySignature'
] 