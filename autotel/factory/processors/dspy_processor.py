"""
DSPy Processor - Enhanced DSPy Module Parser

Implements AUTOTEL-103: Parse custom <dspy:signature> blocks and <dspy:predict>/<dspy:chain-of-thought> 
modules from XML into simple, non-executable data objects.
"""

from typing import List, Dict, Optional, Any
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


@dataclass
class DSPyModuleDefinition:
    """
    Data structure representing a DSPy module configuration in a BPMN service task.
    
    This captures the module type, signature, parameters, and result configuration
    for executable DSPy modules like Predict and ChainOfThought.
    """
    module_type: str  # 'predict' or 'chain-of-thought'
    signature_name: str
    result_variable: str
    rationale_field: Optional[str] = None  # Only for chain-of-thought
    parameters: Dict[str, str] = None  # param_name -> variable_name mapping


@dataclass
class DSPyModelConfiguration:
    """
    Data structure representing DSPy model configuration from BPMN.
    
    This captures the model provider, model name, and configuration parameters
    for DSPy language model setup.
    """
    provider: str  # e.g., 'ollama', 'openai', 'anthropic'
    model_name: str  # e.g., 'qwen3:latest', 'gpt-4o', 'claude-3'
    temperature: float = 0.7
    max_tokens: int = 4000
    model_type: str = "chat"  # 'chat' or 'text'
    cache: bool = True


class DSPyProcessor:
    """
    Enhanced DSPy processor that parses both signature definitions and executable modules.
    
    Supports:
    - <dspy:signature> blocks for signature definitions
    - <dspy:predict> modules in service tasks
    - <dspy:chain-of-thought> modules in service tasks
    - <dspy:model> configuration for language model setup
    """
    
    def __init__(self):
        """Initialize the DSPy processor."""
        self.dspy_ns = {'dspy': 'http://autotel.ai/dspy'}
    
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
            signature_elements = root.xpath('.//dspy:signature', namespaces=self.dspy_ns)
            
            signatures = []
            for element in signature_elements:
                signature = self._parse_signature_element(element)
                signatures.append(signature)
            
            return signatures
            
        except etree.XMLSyntaxError as e:
            raise ValueError(f"Invalid XML format: {str(e)}")
        except Exception as e:
            raise ValueError(f"Failed to parse DSPy signatures: {str(e)}")
    
    def parse_model_configuration(self, bpmn_xml: str) -> Optional[DSPyModelConfiguration]:
        """
        Parse DSPy model configuration from BPMN XML.
        
        Args:
            bpmn_xml: BPMN XML string that may contain DSPy model configuration
            
        Returns:
            DSPyModelConfiguration: Parsed model configuration or None if not found
        """
        try:
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            # Find dspy:model element
            model_elements = root.xpath('.//dspy:model', namespaces=self.dspy_ns)
            
            if not model_elements:
                return None
            
            # Use the first model configuration found
            model_elem = model_elements[0]
            return self._parse_model_element(model_elem)
            
        except etree.XMLSyntaxError:
            return None
        except Exception:
            return None
    
    def parse_modules_in_bpmn(self, bpmn_xml: str) -> List[DSPyModuleDefinition]:
        """
        Parse DSPy modules (predict, chain-of-thought) from BPMN service tasks.
        
        Args:
            bpmn_xml: BPMN XML string containing service tasks with DSPy modules
            
        Returns:
            List[DSPyModuleDefinition]: List of parsed module definitions
        """
        try:
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            modules = []
            
            # Find all service tasks with DSPy modules
            service_tasks = root.xpath('.//bpmn:serviceTask', namespaces={'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'})
            
            for task in service_tasks:
                # Look for dspy:predict or dspy:chain-of-thought in extensionElements
                extension_elements = task.xpath('.//bpmn:extensionElements', namespaces={'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'})
                
                for ext_elem in extension_elements:
                    # Check for dspy:predict
                    predict_elements = ext_elem.xpath('.//dspy:predict', namespaces=self.dspy_ns)
                    for pred_elem in predict_elements:
                        module = self._parse_predict_module(pred_elem, task.get('id'))
                        if module:
                            modules.append(module)
                    
                    # Check for dspy:chain-of-thought
                    cot_elements = ext_elem.xpath('.//dspy:chain-of-thought', namespaces=self.dspy_ns)
                    for cot_elem in cot_elements:
                        module = self._parse_chain_of_thought_module(cot_elem, task.get('id'))
                        if module:
                            modules.append(module)
            
            return modules
            
        except etree.XMLSyntaxError:
            # If BPMN XML is malformed, return empty list
            return []
        except Exception as e:
            # Handle any other errors gracefully
            return []
    
    def _parse_signature_element(self, element: etree._Element) -> DSPySignatureDefinition:
        """
        Parse a single dspy:signature element into a DSPySignatureDefinition.
        
        Args:
            element: The dspy:signature XML element
            
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
        for input_elem in element.xpath('./dspy:input', namespaces=self.dspy_ns):
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
        for output_elem in element.xpath('./dspy:output', namespaces=self.dspy_ns):
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
    
    def _parse_model_element(self, element: etree._Element) -> DSPyModelConfiguration:
        """
        Parse a dspy:model element into a DSPyModelConfiguration.
        
        Args:
            element: The dspy:model XML element
            
        Returns:
            DSPyModelConfiguration: Parsed model configuration
        """
        # Extract model specification (e.g., "ollama/qwen3:latest")
        model_spec = element.get('model', '')
        if not model_spec:
            raise ValueError("DSPy model is missing required 'model' attribute")
        
        # Parse provider and model name
        if '/' in model_spec:
            provider, model_name = model_spec.split('/', 1)
        else:
            provider = 'openai'  # Default provider
            model_name = model_spec
        
        # Extract configuration parameters
        temperature = float(element.get('temperature', '0.7'))
        max_tokens = int(element.get('max_tokens', '4000'))
        model_type = element.get('model_type', 'chat')
        cache = element.get('cache', 'true').lower() == 'true'
        
        return DSPyModelConfiguration(
            provider=provider,
            model_name=model_name,
            temperature=temperature,
            max_tokens=max_tokens,
            model_type=model_type,
            cache=cache
        )
    
    def _parse_predict_module(self, element: etree._Element, task_id: str) -> Optional[DSPyModuleDefinition]:
        """
        Parse a dspy:predict module from a service task.
        
        Args:
            element: The dspy:predict XML element
            task_id: The ID of the service task
            
        Returns:
            DSPyModuleDefinition: Parsed module definition
        """
        signature_name = element.get('signature')
        result_variable = element.get('result')
        
        if not signature_name or not result_variable:
            return None
        
        # Parse parameters
        parameters = {}
        for param_elem in element.xpath('./dspy:param', namespaces=self.dspy_ns):
            param_name = param_elem.get('name')
            param_value = param_elem.get('value')
            if param_name and param_value:
                parameters[param_name] = param_value
        
        return DSPyModuleDefinition(
            module_type='predict',
            signature_name=signature_name,
            result_variable=result_variable,
            parameters=parameters
        )
    
    def _parse_chain_of_thought_module(self, element: etree._Element, task_id: str) -> Optional[DSPyModuleDefinition]:
        """
        Parse a dspy:chain-of-thought module from a service task.
        
        Args:
            element: The dspy:chain-of-thought XML element
            task_id: The ID of the service task
            
        Returns:
            DSPyModuleDefinition: Parsed module definition
        """
        signature_name = element.get('signature')
        result_variable = element.get('result')
        rationale_field = element.get('rationale_field')
        
        if not signature_name or not result_variable:
            return None
        
        if not rationale_field:
            raise ValueError(f"Chain-of-thought module in task '{task_id}' is missing required 'rationale_field' attribute")
        
        # Parse parameters
        parameters = {}
        for param_elem in element.xpath('./dspy:param', namespaces=self.dspy_ns):
            param_name = param_elem.get('name')
            param_value = param_elem.get('value')
            if param_name and param_value:
                parameters[param_name] = param_value
        
        return DSPyModuleDefinition(
            module_type='chain-of-thought',
            signature_name=signature_name,
            result_variable=result_variable,
            rationale_field=rationale_field,
            parameters=parameters
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
            signature_elements = root.xpath('.//dspy:signature', namespaces=self.dspy_ns)
            
            signatures = []
            for element in signature_elements:
                signature = self._parse_signature_element(element)
                signatures.append(signature)
            
            return signatures
            
        except etree.XMLSyntaxError:
            # If BPMN XML is malformed, return empty list
            return []
        except Exception:
            # Handle any other errors gracefully
            return [] 