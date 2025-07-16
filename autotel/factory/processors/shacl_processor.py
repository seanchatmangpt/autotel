"""SHACL processor for AutoTel semantic execution pipeline."""

import xml.etree.ElementTree as ET
from typing import Dict, Any
from rdflib import Graph

from ...core.telemetry import create_telemetry_manager
from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode


class SHACLProcessor:
    """Processes SHACL/RDF XML into constraint graphs."""

    def __init__(self):
        """Initialize SHACL processor with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-shacl-processor",
            require_linkml_validation=False  # Allow basic telemetry without schema validation
        )

    def parse(self, xml_content: str) -> Graph:
        """Parse SHACL XML content into rdflib.Graph."""
        with self.telemetry.start_span("shacl_parse", "constraint_validation") as span:
            # Record input size for validation
            span.set_attribute("input_size_bytes", len(xml_content.encode('utf-8')))
            span.set_attribute("input_format", "xml")
            
            try:
                graph = Graph()
                graph.parse(data=xml_content, format='xml')
                
                # Record successful parsing metrics
                span.set_attribute("triples_count", len(graph))
                span.set_attribute("namespaces_count", len(list(graph.namespaces())))
                span.set_attribute("parse_success", True)
                
                # Record metric for successful parsing
                self.telemetry.record_metric("shacl_parse_success", 1)
                
                return graph
                
            except Exception as e:
                # Record parsing failure
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for parsing failure
                self.telemetry.record_metric("shacl_parse_failure", 1)
                
                raise

    def parse_file(self, file_path: str) -> Graph:
        """Parse SHACL XML file into rdflib.Graph."""
        with self.telemetry.start_span("shacl_parse_file", "constraint_validation") as span:
            # Record file information
            span.set_attribute("file_path", file_path)
            
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    xml_content = f.read()
                
                # Record file size
                span.set_attribute("file_size_bytes", len(xml_content.encode('utf-8')))
                
                # Parse the content
                return self.parse(xml_content)
                
            except FileNotFoundError:
                span.set_attribute("error_type", "FileNotFoundError")
                span.set_attribute("error_message", f"File not found: {file_path}")
                span.set_status(trace.Status(trace.StatusCode.ERROR, f"File not found: {file_path}"))
                
                # Record metric for file not found
                self.telemetry.record_metric("shacl_file_not_found", 1)
                
                raise
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for file parsing failure
                self.telemetry.record_metric("shacl_file_parse_failure", 1)
                
                raise 