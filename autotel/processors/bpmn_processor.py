"""
BPMN Processor - Unified BPMN 2.0 Parser

Implements AUTOTEL-101: Parse standard BPMN 2.0 elements into SpiffWorkflow 
specification objects, completely ignoring all custom DSLs.

Integrates with AutoTel unified processor architecture using helper infrastructure.
"""

from typing import Any, Dict, Optional, Union
from pathlib import Path
import time

from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.bpmn.parser.ValidationException import ValidationException
from SpiffWorkflow.specs import WorkflowSpec
import lxml.etree as etree

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.telemetry.span import create_processor_span


@processor_metadata(
    name="bpmn_processor",
    version="2.1.0",
    capabilities=["parsing", "validation", "workflow_specification"],
    supported_formats=["bpmn", "xml"],
    author="AutoTel Team"
)
class BPMNProcessor(BaseProcessor):
    """
    Unified BPMN processor that exclusively uses the trusted SpiffWorkflow.camunda.parser.CamundaParser
    for parsing standard BPMN 2.0 elements, completely ignoring all custom DSLs.
    
    Integrates with AutoTel unified processor architecture using helper infrastructure.
    """
    
    # Class-level metadata
    CAPABILITIES = ["parsing", "validation", "workflow_specification", "file_processing"]
    SUPPORTED_FORMATS = ["bpmn", "xml", "bpmn20"]
    
    def __init__(self, config: ProcessorConfig):
        """
        Initialize the BPMN processor with configuration.
        
        Args:
            config: Processor configuration
        """
        super().__init__(config)
        self.parser = CamundaParser()
    
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Process BPMN data and return workflow specification.
        
        Args:
            data: Input data - can be XML string, file path, or dict with xml/path
            
        Returns:
            ProcessorResult with WorkflowSpec or error
        """
        with create_processor_span("process", "bpmn") as span:
            start_time = time.time()
            
            try:
                # Determine input type and extract parameters
                xml_content, process_id = self._extract_input_data(data)
                
                # Parse BPMN XML
                workflow_spec = self._parse_bpmn_xml(xml_content, process_id)
                
                # Calculate processing time
                duration_ms = (time.time() - start_time) * 1000
                
                span.set_attribute("bpmn.process_id", process_id)
                span.set_attribute("bpmn.parsing_success", True)
                span.set_attribute("bpmn.duration_ms", duration_ms)
                
                return ProcessorResult.success_result(
                    data=workflow_spec,
                    metadata={
                        "process_id": process_id,
                        "duration_ms": duration_ms,
                        "parser_type": "CamundaParser"
                    }
                )
                
            except Exception as e:
                duration_ms = (time.time() - start_time) * 1000
                
                span.set_attribute("bpmn.parsing_success", False)
                span.set_attribute("bpmn.error", str(e))
                span.set_attribute("bpmn.duration_ms", duration_ms)
                
                return ProcessorResult.error_result(
                    error=str(e),
                    metadata={
                        "error_type": type(e).__name__,
                        "duration_ms": duration_ms
                    }
                )
    
    def _extract_input_data(self, data: Any) -> tuple[str, str]:
        """
        Extract XML content and process ID from input data.
        
        Args:
            data: Input data in various formats
            
        Returns:
            Tuple of (xml_content, process_id)
            
        Raises:
            ValueError: If input format is invalid
        """
        if isinstance(data, str):
            # Check if it's a file path first
            if Path(data).exists():
                raise ValueError("File path input requires process_id parameter")
            else:
                # Assume it's XML content, need process_id
                raise ValueError("String input requires process_id parameter")
        
        elif isinstance(data, dict):
            # Extract from dictionary
            xml_content = data.get('xml')
            file_path = data.get('file_path')
            process_id = data.get('process_id')
            
            if not process_id:
                raise ValueError("process_id is required in input data")
            
            if xml_content:
                return xml_content, process_id
            elif file_path:
                return self._read_file_content(file_path), process_id
            else:
                raise ValueError("Either 'xml' or 'file_path' must be provided in input data")
        
        elif isinstance(data, Path):
            # Path object, need process_id
            raise ValueError("File path input requires process_id parameter")
        
        else:
            raise ValueError(f"Unsupported input format: {type(data)}")
    
    def _read_file_content(self, file_path: Union[str, Path]) -> str:
        """
        Read file content safely.
        
        Args:
            file_path: Path to file
            
        Returns:
            File content as string
            
        Raises:
            FileNotFoundError: If file doesn't exist
            ValueError: If file can't be read
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                return f.read()
        except FileNotFoundError:
            raise FileNotFoundError(f"BPMN file not found: {file_path}")
        except Exception as e:
            raise ValueError(f"Failed to read BPMN file '{file_path}': {str(e)}")
    
    def _parse_bpmn_xml(self, xml_string: str, process_id: str) -> WorkflowSpec:
        """
        Parse BPMN XML string and return WorkflowSpec.
        
        Args:
            xml_string: BPMN XML content as string
            process_id: ID of the process to extract
            
        Returns:
            WorkflowSpec: Valid SpiffWorkflow specification object
            
        Raises:
            ValueError: If XML is malformed or process_id doesn't exist
        """
        try:
            # Parse XML string into etree
            bpmn_tree = etree.fromstring(xml_string.encode('utf-8'))
            
            # Add BPMN XML to parser
            self.parser.add_bpmn_xml(bpmn_tree)
            
            # Get the workflow specification
            spec = self.parser.get_spec(process_id)
            
            if spec is None:
                raise ValueError(f"Process ID '{process_id}' not found in BPMN XML")
            
            return spec
            
        except (ValidationException, KeyError, etree.XMLSyntaxError) as e:
            raise ValueError(f"Failed to parse BPMN XML: {str(e)}")
        except Exception as e:
            # Check if it's a SpiffWorkflow-specific error about missing process
            if "was not found" in str(e):
                raise ValueError(f"Process ID '{process_id}' not found in BPMN XML")
            else:
                raise ValueError(f"Unexpected error parsing BPMN XML: {str(e)}")
    
    def parse_file(self, file_path: Union[str, Path], process_id: str) -> WorkflowSpec:
        """
        Parse BPMN file and return WorkflowSpec (convenience method).
        
        Args:
            file_path: Path to BPMN XML file
            process_id: ID of the process to extract
            
        Returns:
            WorkflowSpec: Valid SpiffWorkflow specification object
        """
        data = {
            'file_path': str(file_path),
            'process_id': process_id
        }
        
        result = self.process(data)
        
        if not result.success:
            raise ValueError(result.error)
        
        return result.data
    
    def parse_xml(self, xml_string: str, process_id: str) -> WorkflowSpec:
        """
        Parse BPMN XML string and return WorkflowSpec (convenience method).
        
        Args:
            xml_string: BPMN XML content as string
            process_id: ID of the process to extract
            
        Returns:
            WorkflowSpec: Valid SpiffWorkflow specification object
        """
        data = {
            'xml': xml_string,
            'process_id': process_id
        }
        
        result = self.process(data)
        
        if not result.success:
            raise ValueError(result.error)
        
        return result.data
    
    def get_parser_info(self) -> Dict[str, Any]:
        """
        Get information about the underlying parser.
        
        Returns:
            Dictionary with parser information
        """
        return {
            "parser_type": "CamundaParser",
            "parser_class": self.parser.__class__.__name__,
            "capabilities": self.CAPABILITIES,
            "supported_formats": self.SUPPORTED_FORMATS
        } 