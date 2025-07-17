"""
DMN Processor - AutoTel Processor Implementation

Implements DMN 1.3 processing using the BaseProcessor architecture.
Provides DMN decision table parsing and execution capabilities.
"""

from typing import Any, Dict, List, Optional
from lxml import etree
from SpiffWorkflow.camunda.parser import CamundaParser

from .base import BaseProcessor, ProcessorConfig, ProcessorResult
from .meta import processor_metadata
from ..core.telemetry import create_telemetry_manager


@processor_metadata(
    name="dmn_processor",
    version="1.0.0",
    capabilities=["dmn_parsing", "decision_execution", "xml_processing"],
    supported_formats=["dmn", "xml"],
    author="AutoTel Team"
)
class DMNProcessor(BaseProcessor):
    """
    DMN processor that parses and executes DMN 1.3 decision tables.
    
    Uses SpiffWorkflow's CamundaParser for reliable DMN processing.
    """
    
    def __init__(self, config: ProcessorConfig):
        """Initialize the DMN processor with configuration."""
        super().__init__(config)
        self.parser = CamundaParser()
        self._decisions: Dict[str, Any] = {}
        self.telemetry = create_telemetry_manager(
            service_name="autotel-dmn-processor",
            require_linkml_validation=False
        )
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Process DMN data (XML string or file path).
        """
        with self.telemetry.start_span("dmn_process", "dmn_processing") as span:
            span.set_attribute("processor_name", "dmn_processor")
            span.set_attribute("input_type", type(data).__name__)
            try:
                if isinstance(data, str):
                    if data.strip().startswith('<?xml') or data.strip().startswith('<definitions'):
                        decisions = self._parse_xml_string(data)
                    else:
                        decisions = self._parse_file(data)
                else:
                    result = ProcessorResult.error_result(
                        error="Invalid input type. Expected string (XML or file path).",
                        metadata={"error_type": "TypeError"}
                    )
                    result.data = {"error": "Invalid input type. Expected string (XML or file path)."}
                    return result
                self._decisions = decisions
                span.set_attribute("decisions_parsed", len(decisions))
                span.set_attribute("parse_success", True)
                return ProcessorResult.success_result(
                    data={
                        "decisions": decisions,
                        "decision_count": len(decisions),
                        "decision_ids": list(decisions.keys())
                    }
                )
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status("ERROR", str(e))
                result = ProcessorResult.error_result(
                    error=str(e),
                    metadata={"error_type": type(e).__name__}
                )
                result.data = {"error": str(e)}
                return result
    
    def _parse_xml_string(self, xml_string: str) -> Dict[str, Any]:
        """Parse DMN XML string into decision objects."""
        with self.telemetry.start_span("dmn_parse_xml", "xml_parsing") as span:
            span.set_attribute("input_size_bytes", len(xml_string.encode('utf-8')))
            
            try:
                # Parse XML string into etree
                dmn_tree = etree.fromstring(xml_string.encode('utf-8'))
                
                # Add DMN XML to parser
                self.parser.add_dmn_xml(dmn_tree)
                
                # Extract decisions from the parser's dmn_parsers
                decisions = {}
                for dmn_parser in getattr(self.parser, 'dmn_parsers', {}).values():
                    if hasattr(dmn_parser, 'decision') and hasattr(dmn_parser.decision, 'id'):
                        decisions[dmn_parser.decision.id] = dmn_parser.decision
                
                span.set_attribute("decisions_found", len(decisions))
                return decisions
                
            except etree.XMLSyntaxError as e:
                span.set_attribute("error_type", "XMLSyntaxError")
                span.set_attribute("error_message", str(e))
                span.set_status("ERROR", str(e))
                raise ValueError(f"Invalid DMN XML: {e}")
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status("ERROR", str(e))
                raise
    
    def _parse_file(self, file_path: str) -> Dict[str, Any]:
        """Parse DMN file into decision objects."""
        with self.telemetry.start_span("dmn_parse_file", "file_parsing") as span:
            span.set_attribute("file_path", file_path)
            
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    xml_string = f.read()
                
                span.set_attribute("file_size_bytes", len(xml_string.encode('utf-8')))
                return self._parse_xml_string(xml_string)
                
            except FileNotFoundError:
                span.set_attribute("error_type", "FileNotFoundError")
                span.set_attribute("error_message", f"File not found: {file_path}")
                span.set_status("ERROR", f"File not found: {file_path}")
                raise FileNotFoundError(f"DMN file not found: {file_path}")
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status("ERROR", str(e))
                raise
    
    def get_decision(self, decision_id: str) -> Optional[Any]:
        """
        Get a specific decision by ID.
        
        Args:
            decision_id: ID of the decision to retrieve
            
        Returns:
            The requested decision object or None if not found
        """
        with self.telemetry.start_span("dmn_get_decision", "decision_retrieval") as span:
            span.set_attribute("decision_id", decision_id)
            
            decision = self._decisions.get(decision_id)
            span.set_attribute("decision_found", decision is not None)
            
            return decision
    
    def list_decisions(self) -> List[str]:
        """
        Get list of available decision IDs.
        
        Returns:
            List of decision IDs
        """
        with self.telemetry.start_span("dmn_list_decisions", "decision_listing") as span:
            decision_ids = list(self._decisions.keys())
            span.set_attribute("decision_count", len(decision_ids))
            return decision_ids
    
    def execute_decision(self, decision_id: str, context: Dict[str, Any]) -> Any:
        """
        Execute a DMN decision with given context.
        
        Args:
            decision_id: ID of the decision to execute
            context: Input context for decision execution
            
        Returns:
            Decision result
        """
        with self.telemetry.start_span("dmn_execute_decision", "decision_execution") as span:
            span.set_attribute("decision_id", decision_id)
            span.set_attribute("context_keys", list(context.keys()))
            
            decision = self.get_decision(decision_id)
            if decision is None:
                span.set_attribute("error_type", "DecisionNotFound")
                span.set_attribute("error_message", f"Decision '{decision_id}' not found")
                span.set_status("ERROR", f"Decision '{decision_id}' not found")
                raise ValueError(f"Decision '{decision_id}' not found")
            
            try:
                # Execute the decision using SpiffWorkflow
                result = decision.evaluate(context)
                span.set_attribute("execution_success", True)
                return result
                
            except Exception as e:
                span.set_attribute("execution_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status("ERROR", str(e))
                raise
    
    def get_capabilities(self) -> List[str]:
        """Get processor capabilities."""
        return ["dmn_parsing", "decision_execution", "xml_processing", "file_processing"]
    
    def get_supported_formats(self) -> List[str]:
        """Get supported input formats."""
        return ["dmn", "xml"] 

    def get_metadata(self) -> dict:
        """Get processor metadata, always returns the correct dict."""
        return getattr(self, '_processor_metadata', {
            "name": "dmn_processor",
            "version": "1.0.0",
            "capabilities": ["dmn_parsing", "decision_execution", "xml_processing"],
            "supported_formats": ["dmn", "xml"],
            "author": "AutoTel Team"
        }) 