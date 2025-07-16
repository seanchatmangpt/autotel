"""DSPy processor for AutoTel semantic execution pipeline."""

import xml.etree.ElementTree as ET
from typing import Dict, List, Any

from ...schemas.dspy_types import DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration


NAMESPACES = {'dspy': 'http://autotel.ai/dspy'}

class DSPyProcessor:
    """Processes DSPy XML into structured definitions."""

    def parse(self, xml_content: str) -> List[DSPySignatureDefinition]:
        """Parse DSPy XML content into signature definitions."""
        root = ET.fromstring(xml_content)
        
        signatures = []
        for sig_elem in root.findall(".//dspy:signature", NAMESPACES):
            signature = DSPySignatureDefinition(
                name=sig_elem.get("name", ""),
                description=sig_elem.get("description", ""),
                inputs=self._extract_inputs(sig_elem),
                outputs=self._extract_outputs(sig_elem),
                examples=self._extract_examples(sig_elem)
            )
            signatures.append(signature)
        
        return signatures

    def parse_modules(self, xml_content: str) -> List[DSPyModuleDefinition]:
        """Parse DSPy modules from XML content."""
        root = ET.fromstring(xml_content)
        
        modules = []
        for module_elem in root.findall(".//dspy:module", NAMESPACES):
            module = DSPyModuleDefinition(
                module_type=module_elem.get("type", ""),
                configuration=self._extract_configuration(module_elem),
                dependencies=self._extract_dependencies(module_elem)
            )
            modules.append(module)
        
        return modules

    def parse_model_configuration(self, xml_content: str) -> DSPyModelConfiguration:
        """Parse DSPy model configuration from XML content."""
        root = ET.fromstring(xml_content)
        
        model_elem = root.find(".//dspy:model", NAMESPACES)
        if model_elem is None:
            return DSPyModelConfiguration(
                provider="default",
                model_name="default",
                parameters={},
                telemetry_config={}
            )
        
        return DSPyModelConfiguration(
            provider=model_elem.get("provider", "default"),
            model_name=model_elem.get("name", "default"),
            parameters=self._extract_parameters(model_elem),
            telemetry_config=self._extract_telemetry_config(model_elem)
        )

    def _extract_inputs(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract inputs from signature element."""
        inputs = {}
        for input_elem in elem.findall(".//dspy:input", NAMESPACES):
            name = input_elem.get("name", "")
            inputs[name] = {
                "type": input_elem.get("type", "string"),
                "description": input_elem.get("description", "")
            }
        return inputs

    def _extract_outputs(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract outputs from signature element."""
        outputs = {}
        for output_elem in elem.findall(".//dspy:output", NAMESPACES):
            name = output_elem.get("name", "")
            outputs[name] = {
                "type": output_elem.get("type", "string"),
                "description": output_elem.get("description", "")
            }
        return outputs

    def _extract_examples(self, elem: ET.Element) -> List[Dict[str, Any]]:
        """Extract examples from signature element."""
        examples = []
        for example_elem in elem.findall(".//dspy:example", NAMESPACES):
            example = {}
            for child in example_elem:
                example[child.tag] = child.text or ""
            examples.append(example)
        return examples

    def _extract_configuration(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract configuration from module element."""
        config = {}
        for config_elem in elem.findall(".//dspy:config", NAMESPACES):
            key = config_elem.get("key", "")
            value = config_elem.get("value", "")
            config[key] = value
        return config

    def _extract_dependencies(self, elem: ET.Element) -> List[str]:
        """Extract dependencies from module element."""
        dependencies = []
        for dep_elem in elem.findall(".//dspy:dependency", NAMESPACES):
            dependencies.append(dep_elem.get("name", ""))
        return dependencies

    def _extract_parameters(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract parameters from model element."""
        parameters = {}
        for param_elem in elem.findall(".//dspy:parameter", NAMESPACES):
            key = param_elem.get("key", "")
            value = param_elem.get("value", "")
            parameters[key] = value
        return parameters

    def _extract_telemetry_config(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract telemetry configuration from model element."""
        telemetry = {}
        for telemetry_elem in elem.findall(".//dspy:telemetry", NAMESPACES):
            key = telemetry_elem.get("key", "")
            value = telemetry_elem.get("value", "")
            telemetry[key] = value
        return telemetry 

 