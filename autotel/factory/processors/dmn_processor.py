"""
DMN Processor - Isolated DMN 1.3 Parser

Implements AUTOTEL-102: Parse DMN 1.3 elements from XML into executable 
SpiffWorkflow decision objects, ignoring all non-DMN elements.
"""

from typing import Dict, Any
from SpiffWorkflow.camunda.parser import CamundaParser
import lxml.etree as etree

from ...core.telemetry import create_telemetry_manager
from opentelemetry import trace


class DMNProcessor:
    """
    Isolated DMN processor that exclusively uses the trusted SpiffWorkflow.camunda.parser.CamundaParser
    for parsing DMN 1.3 elements, completely ignoring all non-DMN elements.
    """
    
    def __init__(self):
        """Initialize the DMN processor with a CamundaParser instance and telemetry."""
        self.parser = CamundaParser()
        self.telemetry = create_telemetry_manager(
            service_name="autotel-dmn-processor",
            require_linkml_validation=False  # Allow basic telemetry without schema validation
        )
    
    def parse(self, xml_string: str) -> Dict[str, Any]:
        """
        Parse DMN XML string and return executable DMN decision objects.
        
        Args:
            xml_string: DMN XML content as string
            
        Returns:
            Dict[str, Any]: Dictionary mapping decision IDs to executable DMN decision objects.
                           Returns empty dict if no DMN definitions found.
        """
        with self.telemetry.start_span("dmn_parse", "constraint_validation") as span:
            # Record input metrics
            span.set_attribute("input_size_bytes", len(xml_string.encode('utf-8')))
            span.set_attribute("input_format", "xml")
            
            try:
                # Parse XML string into etree
                dmn_tree = etree.fromstring(xml_string.encode('utf-8'))
                
                # Add DMN XML to parser
                self.parser.add_dmn_xml(dmn_tree)
                
                # Return the parser's decisions dictionary
                decisions = self.parser.decisions
                self._last_decisions = decisions  # Store for get_decision
                
                # Record successful parsing metrics
                span.set_attribute("decisions_found", len(decisions))
                span.set_attribute("parse_success", True)
                
                # Record metric for successful parsing
                self.telemetry.record_metric("dmn_parse_success", 1)
                self.telemetry.record_metric("dmn_decisions_parsed", len(decisions))
                
                return decisions
                
            except etree.XMLSyntaxError as e:
                # Handle XML parsing errors gracefully
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", "XMLSyntaxError")
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for XML syntax errors
                self.telemetry.record_metric("dmn_xml_syntax_error", 1)
                
                return {}
            except Exception as e:
                # Handle any other errors gracefully
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for parsing errors
                self.telemetry.record_metric("dmn_parse_error", 1)
                
                return {}
    
    def parse_file(self, file_path: str) -> Dict[str, Any]:
        """
        Parse DMN file and return executable SpiffWorkflow.dmn.specs.DmnDecision objects.
        
        Args:
            file_path: Path to DMN XML file
            
        Returns:
            Dict[str, Any]: Dictionary mapping decision IDs to executable DMN decision objects.
                           Returns empty dict if no DMN definitions found.
                                   
        Raises:
            FileNotFoundError: If file does not exist
        """
        with self.telemetry.start_span("dmn_parse_file", "dmn_processing") as span:
            # Record file information
            span.set_attribute("file_path", file_path)
            
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    xml_string = f.read()
                
                # Record file size
                span.set_attribute("file_size_bytes", len(xml_string.encode('utf-8')))
                
                # Parse the content
                return self.parse(xml_string)
                
            except FileNotFoundError:
                span.set_attribute("error_type", "FileNotFoundError")
                span.set_attribute("error_message", f"File not found: {file_path}")
                span.set_status(trace.Status(trace.StatusCode.ERROR, f"File not found: {file_path}"))
                
                # Record metric for file not found
                self.telemetry.record_metric("dmn_file_not_found", 1)
                
                raise FileNotFoundError(f"DMN file not found: {file_path}")
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for file parsing errors
                self.telemetry.record_metric("dmn_file_parse_error", 1)
                
                return {}
    
    def get_decision(self, decision_id: str) -> Any:
        """
        Get a specific decision by ID.
        
        Args:
            decision_id: ID of the decision to retrieve
            
        Returns:
            Any: The requested decision object
            
        Raises:
            KeyError: If decision_id is not found
        """
        with self.telemetry.start_span("dmn_get_decision", "dmn_processing") as span:
            span.set_attribute("decision_id", decision_id)
            
            try:
                if not hasattr(self, '_last_decisions'):
                    span.set_attribute("error_type", "KeyError")
                    span.set_attribute("error_message", f"Decision ID '{decision_id}' not found")
                    span.set_status(trace.Status(trace.StatusCode.ERROR, f"Decision ID '{decision_id}' not found"))
                    
                    # Record metric for decision not found
                    self.telemetry.record_metric("dmn_decision_not_found", 1)
                    
                    raise KeyError(f"Decision ID '{decision_id}' not found")
                    
                if decision_id not in self._last_decisions:
                    span.set_attribute("error_type", "KeyError")
                    span.set_attribute("error_message", f"Decision ID '{decision_id}' not found")
                    span.set_status(trace.Status(trace.StatusCode.ERROR, f"Decision ID '{decision_id}' not found"))
                    
                    # Record metric for decision not found
                    self.telemetry.record_metric("dmn_decision_not_found", 1)
                    
                    raise KeyError(f"Decision ID '{decision_id}' not found")
                
                # Record successful retrieval
                span.set_attribute("decision_found", True)
                
                # Record metric for successful decision retrieval
                self.telemetry.record_metric("dmn_decision_retrieved", 1)
                
                return self._last_decisions[decision_id]
                
            except KeyError:
                # Re-raise KeyError as expected
                raise
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for unexpected errors
                self.telemetry.record_metric("dmn_unexpected_error", 1)
                
                raise 