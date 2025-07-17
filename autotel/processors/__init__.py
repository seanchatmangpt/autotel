"""
AutoTel Processors Module

Unified processor architecture using helper infrastructure.
Provides base classes and utilities for all processor types.
"""

from .base import BaseProcessor, ProcessorConfig, ProcessorResult
from .registry import ProcessorRegistry
from .meta import ProcessorMeta, processor_metadata
from .bpmn_processor import BPMNProcessor
from .dmn_processor import DMNProcessor
from .dspy_processor import DSPyProcessor
from .shacl_processor import SHACLProcessor
from .owl_processor import OWLProcessor
from .otel_processor import OTELProcessor
from .jinja_processor import JinjaProcessor
from .sparql_processor import SPARQLProcessor

__all__ = [
    'BaseProcessor',
    'ProcessorConfig', 
    'ProcessorResult',
    'ProcessorRegistry',
    'ProcessorMeta',
    'processor_metadata',
    'BPMNProcessor',
    'DMNProcessor',
    'DSPyProcessor',
    'SHACLProcessor',
    'OWLProcessor',
    'OTELProcessor',
    'JinjaProcessor',
    'SPARQLProcessor'
] 