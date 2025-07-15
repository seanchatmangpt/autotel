"""
BPMN Processor - Isolated BPMN 2.0 Parser

Implements AUTOTEL-101: Parse standard BPMN 2.0 elements into SpiffWorkflow 
specification objects, completely ignoring all custom DSLs.
"""

from typing import Optional
from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.bpmn.parser.ValidationException import ValidationException
from SpiffWorkflow.specs import WorkflowSpec
import lxml.etree as etree


class BPMNProcessor:
    """
    Isolated BPMN processor that exclusively uses the trusted SpiffWorkflow.camunda.parser.CamundaParser
    for parsing standard BPMN 2.0 elements, completely ignoring all custom DSLs.
    """
    
    def __init__(self, parser: Optional[CamundaParser] = None):
        """
        Initialize the BPMN processor with a CamundaParser instance.
        
        Args:
            parser: Optional shared CamundaParser instance. If None, creates a new one.
        """
        self.parser = parser if parser is not None else CamundaParser()
    
    def parse(self, xml_string: str, process_id: str) -> WorkflowSpec:
        """
        Parse BPMN XML string and return a valid SpiffWorkflow.specs.WorkflowSpec object.
        
        Args:
            xml_string: BPMN XML content as string
            process_id: ID of the process to extract from the XML
            
        Returns:
            WorkflowSpec: Valid SpiffWorkflow specification object
            
        Raises:
            ValueError: If XML is malformed or process_id does not exist
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
            # Re-raise as ValueError with clear error message
            raise ValueError(f"Failed to parse BPMN XML: {str(e)}")
        except Exception as e:
            # Catch any other unexpected errors
            raise ValueError(f"Unexpected error parsing BPMN XML: {str(e)}")
    
    def parse_file(self, file_path: str, process_id: str) -> WorkflowSpec:
        """
        Parse BPMN file and return a valid SpiffWorkflow.specs.WorkflowSpec object.
        
        Args:
            file_path: Path to BPMN XML file
            process_id: ID of the process to extract from the file
            
        Returns:
            WorkflowSpec: Valid SpiffWorkflow specification object
            
        Raises:
            ValueError: If file is malformed or process_id does not exist
            FileNotFoundError: If file does not exist
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                xml_string = f.read()
            
            return self.parse(xml_string, process_id)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"BPMN file not found: {file_path}")
        except Exception as e:
            raise ValueError(f"Failed to read BPMN file '{file_path}': {str(e)}") 