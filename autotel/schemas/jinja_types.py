"""Jinja XML processor data types for AutoTel semantic execution pipeline."""

from dataclasses import dataclass, field
from typing import Dict, Any, List, Optional, Union
from enum import Enum


class JinjaTemplateType(Enum):
    """Types of Jinja templates supported by the processor."""
    XML = "xml"
    EMAIL = "email"
    DOCUMENT = "document"
    CONFIGURATION = "configuration"
    WORKFLOW = "workflow"


class JinjaVariableType(Enum):
    """Types of variables that can be used in Jinja templates."""
    STRING = "string"
    INTEGER = "integer"
    FLOAT = "float"
    BOOLEAN = "boolean"
    LIST = "list"
    DICT = "dict"
    OBJECT = "object"


@dataclass
class JinjaVariable:
    """Definition of a variable used in Jinja templates."""
    name: str
    description: str
    type: JinjaVariableType
    required: bool = True
    default_value: Optional[Any] = None
    validation_rules: Optional[Dict[str, Any]] = None
    example_value: Optional[Any] = None


@dataclass
class JinjaTemplate:
    """Definition of a Jinja template."""
    name: str
    description: str
    template_type: JinjaTemplateType
    template_content: str
    variables: List[JinjaVariable] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    version: str = "1.0"
    author: Optional[str] = None
    tags: List[str] = field(default_factory=list)


@dataclass
class JinjaTemplateDefinition:
    """Complete definition of Jinja templates from XML."""
    templates: List[JinjaTemplate] = field(default_factory=list)
    global_variables: List[JinjaVariable] = field(default_factory=list)
    includes: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)
    namespace: Optional[str] = None


@dataclass
class JinjaRenderingResult:
    """Result of rendering a Jinja template."""
    template_name: str
    rendered_content: str
    variables_used: Dict[str, Any]
    rendering_time_ms: float
    success: bool = True
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class JinjaValidationResult:
    """Result of validating Jinja template variables."""
    template_name: str
    valid: bool
    missing_variables: List[str] = field(default_factory=list)
    invalid_variables: List[str] = field(default_factory=list)
    type_errors: List[str] = field(default_factory=list)
    validation_errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)


@dataclass
class JinjaProcessingResult:
    """Complete result of Jinja template processing."""
    template_definitions: JinjaTemplateDefinition
    validation_results: List[JinjaValidationResult] = field(default_factory=list)
    rendering_results: List[JinjaRenderingResult] = field(default_factory=list)
    processing_time_ms: float = 0.0
    success: bool = True
    errors: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict) 