"""
AutoTel SPARQL Processor (Unified Architecture)

Processes SPARQL queries and executes them against RDF endpoints.
Implements happy-path query parsing and execution with telemetry and contracts.
"""

from typing import Any, Dict, List, Optional
from pathlib import Path
import xml.etree.ElementTree as ET
from pydantic import BaseModel, Field
import re
import time

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error
from autotel.schemas.sparql_types import SPARQLQueryDefinition, SPARQLResult, SPARQLQueryTemplate, SPARQLConfiguration

# SPARQL namespaces
NAMESPACES = {
    'sparql': 'http://autotel.ai/sparql',
    'owl': 'http://www.w3.org/2002/07/owl#',
    'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
    'rdfs': 'http://www.w3.org/2000/01/rdf-schema#'
}

class SPARQLQueryResult(BaseModel):
    """Result model for SPARQL processing."""
    queries: List[SPARQLQueryDefinition] = Field(default_factory=list, description="Extracted queries")
    templates: List[SPARQLQueryTemplate] = Field(default_factory=list, description="Extracted templates")
    total_queries: int = Field(..., description="Total number of queries found")
    total_templates: int = Field(..., description="Total number of templates found")

@processor_metadata(
    name="sparql_processor",
    version="1.0.0",
    capabilities=["sparql_parse", "query_extraction", "template_extraction"],
    supported_formats=["xml", "sparql", "rq"],
    author="AutoTel Team"
)
class SPARQLProcessor(BaseProcessor):
    """
    Unified SPARQL processor for AutoTel.
    Parses SPARQL XML and extracts queries and templates (happy path).
    """
    def __init__(self, config: Optional[ProcessorConfig] = None):
        if config is None:
            config = ProcessorConfig(name="sparql_processor")
        super().__init__(config)

    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse SPARQL XML string and extract queries and templates (happy path only).
        Args:
            data: XML string or file path
        Returns:
            ProcessorResult with SPARQLQueryResult data
        """
        with create_processor_span("parse", "sparql") as span:
            try:
                xml_content = self._get_xml_content(data)
                root = ET.fromstring(xml_content)
                queries = self._extract_queries(root)
                templates = self._extract_templates(root)
                sparql_result = SPARQLQueryResult(
                    queries=queries,
                    templates=templates,
                    total_queries=len(queries),
                    total_templates=len(templates)
                )
                record_span_success(span, {
                    "queries": len(queries),
                    "templates": len(templates)
                })
                return ProcessorResult.success_result(
                    data=sparql_result,
                    metadata={"queries": len(queries), "templates": len(templates)}
                )
            except Exception as e:
                record_span_error(span, e, {"input_type": type(data).__name__})
                return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})

    def _get_xml_content(self, data: Any) -> str:
        """Get XML content from string or file path."""
        if isinstance(data, str):
            if Path(data).exists():
                return Path(data).read_text(encoding="utf-8")
            return data
        elif isinstance(data, Path):
            return data.read_text(encoding="utf-8")
        raise ValueError("Input must be XML string or file path")

    def _extract_queries(self, root: ET.Element) -> List[SPARQLQueryDefinition]:
        """Extract SPARQL queries from XML (happy path)."""
        queries = []
        for query_elem in root.findall(".//sparql:query", NAMESPACES):
            name = query_elem.get("name")
            if not name:
                raise ValueError("SPARQL query is missing required 'name' attribute")
            description = query_elem.get("description", "")
            query_text = query_elem.text or ""
            query_type = self._detect_query_type(query_text)
            parameters = self._extract_parameters(query_elem)
            prefixes = self._extract_prefixes(query_elem)
            queries.append(SPARQLQueryDefinition(
                name=name,
                description=description,
                query=query_text,
                query_type=query_type,
                parameters=parameters,
                prefixes=prefixes
            ))
        return queries

    def _extract_templates(self, root: ET.Element) -> List[SPARQLQueryTemplate]:
        """Extract SPARQL query templates from XML (happy path)."""
        templates = []
        for template_elem in root.findall(".//sparql:template", NAMESPACES):
            name = template_elem.get("name")
            if not name:
                raise ValueError("SPARQL template is missing required 'name' attribute")
            description = template_elem.get("description", "")
            template_text = template_elem.text or ""
            parameters = self._extract_template_parameters(template_text)
            validation_rules = self._extract_validation_rules(template_elem)
            examples = self._extract_examples(template_elem)
            templates.append(SPARQLQueryTemplate(
                name=name,
                description=description,
                template=template_text,
                parameters=parameters,
                validation_rules=validation_rules,
                examples=examples
            ))
        return templates

    def _detect_query_type(self, query_text: str) -> str:
        """Detect SPARQL query type from query text."""
        query_upper = query_text.strip().upper()
        if query_upper.startswith("SELECT"):
            return "SELECT"
        elif query_upper.startswith("ASK"):
            return "ASK"
        elif query_upper.startswith("CONSTRUCT"):
            return "CONSTRUCT"
        elif query_upper.startswith("DESCRIBE"):
            return "DESCRIBE"
        else:
            return "UNKNOWN"

    def _extract_parameters(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract parameters from query element."""
        parameters = {}
        for param_elem in elem.findall(".//sparql:parameter", NAMESPACES):
            name = param_elem.get("name")
            if name:
                parameters[name] = {
                    "type": param_elem.get("type", "string"),
                    "required": param_elem.get("required", "false").lower() == "true",
                    "default": param_elem.get("default")
                }
        return parameters

    def _extract_prefixes(self, elem: ET.Element) -> Dict[str, str]:
        """Extract prefixes from query element."""
        prefixes = {}
        for prefix_elem in elem.findall(".//sparql:prefix", NAMESPACES):
            name = prefix_elem.get("name")
            uri = prefix_elem.get("uri")
            if name and uri:
                prefixes[name] = uri
        return prefixes

    def _extract_template_parameters(self, template_text: str) -> List[str]:
        """Extract parameter names from template text."""
        # Look for patterns like ?param or $param and extract just the parameter name
        param_pattern = r'[?$](\w+)'
        params = re.findall(param_pattern, template_text)
        return list(set(params))  # Remove duplicates

    def _extract_validation_rules(self, elem: ET.Element) -> List[Dict[str, Any]]:
        """Extract validation rules from template element."""
        rules = []
        for rule_elem in elem.findall(".//sparql:validation", NAMESPACES):
            rule = {
                "type": rule_elem.get("type", "regex"),
                "pattern": rule_elem.get("pattern", ""),
                "message": rule_elem.get("message", "")
            }
            rules.append(rule)
        return rules

    def _extract_examples(self, elem: ET.Element) -> List[Dict[str, Any]]:
        """Extract examples from template element."""
        examples = []
        for example_elem in elem.findall(".//sparql:example", NAMESPACES):
            example = {}
            for child in example_elem:
                example[child.tag] = child.text or ""
            examples.append(example)
        return examples

    def parse(self, xml_content: str) -> Dict[str, Any]:
        """Parse SPARQL XML content into query definitions (for test compatibility)."""
        try:
            root = ET.fromstring(xml_content)
            queries = self._extract_queries(root)
            templates = self._extract_templates(root)
            return {
                "queries": queries,
                "templates": templates
            }
        except ValueError as e:
            # Re-raise ValueError with original message
            raise e
        except Exception:
            raise ValueError("Invalid XML format")

    def parse_file(self, file_path: str) -> Dict[str, Any]:
        """Parse SPARQL XML from file into query definitions (for test compatibility)."""
        path = Path(file_path)
        if not path.exists():
            raise FileNotFoundError(f"File not found: {file_path}")
        xml_content = path.read_text(encoding="utf-8")
        return self.parse(xml_content)

    def validate_query(self, query_text: str) -> bool:
        """Validate SPARQL query syntax (basic validation)."""
        query_upper = query_text.strip().upper()
        valid_types = ["SELECT", "ASK", "CONSTRUCT", "DESCRIBE"]
        
        # Check if query starts with a valid type
        if not any(query_upper.startswith(qt) for qt in valid_types):
            return False
        
        # Additional validation: SELECT queries should have WHERE clause
        if query_upper.startswith("SELECT") and "WHERE" not in query_upper:
            return False
            
        return True

    def extract_variables(self, query_text: str) -> List[str]:
        """Extract variable names from SPARQL query."""
        var_pattern = r'[?$](\w+)'
        return list(set(re.findall(var_pattern, query_text))) 