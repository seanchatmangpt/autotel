"""
AutoTel Processors Module

Unified processor architecture using helper infrastructure.
Provides base classes and utilities for all processor types.
"""

from .base import BaseProcessor, ProcessorConfig, ProcessorResult
from .registry import ProcessorRegistry
from .meta import ProcessorMeta, processor_metadata

__all__ = [
    'BaseProcessor',
    'ProcessorConfig', 
    'ProcessorResult',
    'ProcessorRegistry',
    'ProcessorMeta',
    'processor_metadata'
] 