"""
DSPy Processor - Isolated DSPy Signature Parser

Implements AUTOTEL-103: Parse custom <dspy:signature> blocks from XML into 
simple, non-executable DSPySignatureDefinition data objects.
"""

from typing import List, Dict, Optional
from dataclasses import dataclass
import lxml.etree as etree


@dataclass
class DSPySignatureDefinition:
    """
    Simple, non-executable data structure representing a DSPy signature definition.
    
    This is a language-agnostic representation of AI contracts that can be
    used by the semantic linker to inject executable capabilities.
    """
    name: str
    description: str
    inputs: Dict[str, Dict[str, str]]  # input_name -> {description, optional}
    outputs: Dict[str, str]  # output_name -> description


class DSPyProcessor:
    """
    Isolated DSPy processor that parses custom <dspy:signature> blocks from XML
    into simple, non-executable DSPySignatureDefinition data objects.
    """
    
    def __init__(self):
        """Initialize the DSPy processor."""
        pass
    
    def parse(self, xml_fragment: str) -> List[DSPySignatureDefinition]:
        """
        Parse DSPy signature definitions from XML fragment.
        
        Args:
            xml_fragment: XML string containing DSPy signature definitions
            
        Returns:
            List[DSPySignatureDefinition]: List of parsed signature definitions
            
        Raises:
            ValueError: If a signature is missing a required name attribute
        """
        try:
            # Parse XML fragment
            root = etree.fromstring(xml_fragment.encode('utf-8'))
            
            # Find all dspy:signature elements
            dspy_ns = {'dspy': 'http://autotel.ai/dspy'}
            signature_elements = root.xpath('.//dspy:signature', namespaces=dspy_ns)
            
            signatures = []
            for element in signature_elements:
                signature = self._parse_signature_element(element, dspy_ns)
                signatures.append(signature)
            
            return signatures
            
        except etree.XMLSyntaxError as e:
            raise ValueError(f"Invalid XML format: {str(e)}")
        except Exception as e:
            raise ValueError(f"Failed to parse DSPy signatures: {str(e)}")
    
    def _parse_signature_element(self, element: etree._Element, namespaces: Dict[str, str]) -> DSPySignatureDefinition:
        """
        Parse a single dspy:signature element into a DSPySignatureDefinition.
        
        Args:
            element: The dspy:signature XML element
            namespaces: XML namespaces dictionary
            
        Returns:
            DSPySignatureDefinition: Parsed signature definition
            
        Raises:
            ValueError: If required name attribute is missing
        """
        # Extract required name attribute
        name = element.get('name')
        if not name:
            raise ValueError("DSPy signature is missing required 'name' attribute")
        
        # Extract description (optional)
        description = element.get('description', '')
        
        # Parse inputs
        inputs = {}
        for input_elem in element.xpath('./dspy:input', namespaces=namespaces):
            input_name = input_elem.get('name')
            if input_name:
                input_desc = input_elem.get('description', '')
                optional = input_elem.get('optional', 'false').lower() == 'true'
                inputs[input_name] = {
                    'description': input_desc,
                    'optional': str(optional)
                }
        
        # Parse outputs
        outputs = {}
        for output_elem in element.xpath('./dspy:output', namespaces=namespaces):
            output_name = output_elem.get('name')
            if output_name:
                output_desc = output_elem.get('description', '')
                outputs[output_name] = output_desc
        
        return DSPySignatureDefinition(
            name=name,
            description=description,
            inputs=inputs,
            outputs=outputs
        )
    
    def parse_file(self, file_path: str) -> List[DSPySignatureDefinition]:
        """
        Parse DSPy signature definitions from file.
        
        Args:
            file_path: Path to XML file containing DSPy signatures
            
        Returns:
            List[DSPySignatureDefinition]: List of parsed signature definitions
            
        Raises:
            FileNotFoundError: If file does not exist
            ValueError: If parsing fails
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                xml_content = f.read()
            
            return self.parse(xml_content)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"DSPy file not found: {file_path}")
        except Exception as e:
            raise ValueError(f"Failed to read DSPy file '{file_path}': {str(e)}")
    
    def find_signatures_in_bpmn(self, bpmn_xml: str) -> List[DSPySignatureDefinition]:
        """
        Find and parse DSPy signatures embedded in BPMN XML.
        
        Args:
            bpmn_xml: BPMN XML string that may contain embedded DSPy signatures
            
        Returns:
            List[DSPySignatureDefinition]: List of parsed signature definitions
        """
        try:
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            # Find all dspy:signature elements within BPMN
            dspy_ns = {'dspy': 'http://autotel.ai/dspy'}
            signature_elements = root.xpath('.//dspy:signature', namespaces=dspy_ns)
            
            signatures = []
            for element in signature_elements:
                signature = self._parse_signature_element(element, dspy_ns)
                signatures.append(signature)
            
            return signatures
            
        except etree.XMLSyntaxError:
            # If BPMN XML is malformed, return empty list
            return []
        except Exception:
            # Handle any other errors gracefully
            return [] 