"""Ontology data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional


@dataclass
class PropertySchema:
    """Schema for an OWL property."""
    name: str
    uri: str
    data_type: str
    domain: str
    range: str
    cardinality: Optional[str] = None


@dataclass
class ClassSchema:
    """Schema for an OWL class with semantic classification."""
    name: str
    uri: str
    semantic_type: str  # user_input, recommendation, decision, analysis, reasoning, general
    properties: Dict[str, PropertySchema]
    superclasses: List[str]
    description: str


@dataclass
class OntologySchema:
    """Compiled ontology schema with semantic context."""
    ontology_uri: str
    namespace: str
    prefix: str
    classes: Dict[str, ClassSchema]
    semantic_context: Dict[str, Any]
    examples: List[Dict[str, Any]]


@dataclass
class OWLOntologyDefinition:
    """Raw OWL ontology definition from processor."""
    ontology_uri: str
    prefix: str
    namespace: str
    classes: Dict[str, Any]
    object_properties: Dict[str, Any]
    data_properties: Dict[str, Any]
    individuals: Dict[str, Any]
    axioms: List[Dict[str, Any]] 