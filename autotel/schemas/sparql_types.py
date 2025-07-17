"""SPARQL data structures for AutoTel semantic execution pipeline."""

from dataclasses import dataclass
from typing import Dict, List, Any, Optional


@dataclass
class SPARQLQueryDefinition:
    """SPARQL query definition."""
    name: str
    description: str
    query: str
    query_type: str  # SELECT, ASK, CONSTRUCT, DESCRIBE
    parameters: Dict[str, Any]
    prefixes: Dict[str, str]


@dataclass
class SPARQLResult:
    """SPARQL query result."""
    query_name: str
    result_type: str  # results, boolean, graph
    data: Any
    variables: List[str]
    row_count: int
    execution_time_ms: Optional[float] = None


@dataclass
class SPARQLQueryTemplate:
    """SPARQL query template with parameterization."""
    name: str
    description: str
    template: str
    parameters: List[str]
    validation_rules: List[Dict[str, Any]]
    examples: List[Dict[str, Any]]


@dataclass
class SPARQLConfiguration:
    """SPARQL processor configuration."""
    endpoint_url: str
    default_graph_uri: Optional[str] = None
    named_graph_uri: Optional[str] = None
    timeout: int = 30
    max_results: int = 1000
    authentication: Optional[Dict[str, str]] = None
    headers: Optional[Dict[str, str]] = None 