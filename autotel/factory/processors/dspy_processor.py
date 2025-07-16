"""DSPy processor for AutoTel semantic execution pipeline."""

import xml.etree.ElementTree as ET
from typing import Dict, List, Any

from ...schemas.dspy_types import DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration
from ...core.telemetry import create_telemetry_manager
from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode


NAMESPACES = {'dspy': 'http://autotel.ai/dspy'}

class DSPyProcessor:
    """Processes DSPy XML into structured definitions."""

    def __init__(self):
        """Initialize DSPy processor with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-dspy-processor",
            require_linkml_validation=False  # Allow basic telemetry without schema validation
        )

    def parse(self, xml_content: str) -> List[DSPySignatureDefinition]:
        """Parse DSPy XML content into signature definitions."""
        with self.telemetry.start_span("dspy_parse_signatures", "schema_loading") as span:
            # Record input metrics
            span.set_attribute("input_size_bytes", len(xml_content.encode('utf-8')))
            span.set_attribute("input_format", "xml")
            
            try:
                root = ET.fromstring(xml_content)
                
                signatures = []
                signature_elements = root.findall(".//dspy:signature", NAMESPACES)
                
                # Record number of signatures found
                span.set_attribute("signatures_found", len(signature_elements))
                
                for sig_elem in signature_elements:
                    signature = DSPySignatureDefinition(
                        name=sig_elem.get("name", ""),
                        description=sig_elem.get("description", ""),
                        inputs=self._extract_inputs(sig_elem),
                        outputs=self._extract_outputs(sig_elem),
                        examples=self._extract_examples(sig_elem)
                    )
                    signatures.append(signature)
                
                # Record successful parsing metrics
                span.set_attribute("signatures_parsed", len(signatures))
                span.set_attribute("parse_success", True)
                
                # Record metric for successful parsing
                self.telemetry.record_metric("dspy_signatures_parse_success", 1)
                
                return signatures
                
            except Exception as e:
                # Record parsing failure
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for parsing failure
                self.telemetry.record_metric("dspy_signatures_parse_failure", 1)
                
                raise

    def parse_modules(self, xml_content: str) -> List[DSPyModuleDefinition]:
        """Parse DSPy modules from XML content."""
        with self.telemetry.start_span("dspy_parse_modules", "schema_loading") as span:
            # Record input metrics
            span.set_attribute("input_size_bytes", len(xml_content.encode('utf-8')))
            span.set_attribute("input_format", "xml")
            
            try:
                root = ET.fromstring(xml_content)
                
                modules = []
                module_elements = root.findall(".//dspy:module", NAMESPACES)
                
                # Record number of modules found
                span.set_attribute("modules_found", len(module_elements))
                
                for module_elem in module_elements:
                    module = DSPyModuleDefinition(
                        module_type=module_elem.get("type", ""),
                        configuration=self._extract_configuration(module_elem),
                        dependencies=self._extract_dependencies(module_elem)
                    )
                    modules.append(module)
                
                # Record successful parsing metrics
                span.set_attribute("modules_parsed", len(modules))
                span.set_attribute("parse_success", True)
                
                # Record metric for successful parsing
                self.telemetry.record_metric("dspy_modules_parse_success", 1)
                
                return modules
                
            except Exception as e:
                # Record parsing failure
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for parsing failure
                self.telemetry.record_metric("dspy_modules_parse_failure", 1)
                
                raise

    def parse_model_configuration(self, xml_content: str) -> DSPyModelConfiguration:
        """Parse DSPy model configuration from XML content."""
        with self.telemetry.start_span("dspy_parse_model_config", "schema_loading") as span:
            # Record input metrics
            span.set_attribute("input_size_bytes", len(xml_content.encode('utf-8')))
            span.set_attribute("input_format", "xml")
            
            try:
                root = ET.fromstring(xml_content)
                
                model_elem = root.find(".//dspy:model", NAMESPACES)
                if model_elem is None:
                    # Record default configuration creation
                    span.set_attribute("model_found", False)
                    span.set_attribute("using_default_config", True)
                    
                    config = DSPyModelConfiguration(
                        provider="default",
                        model_name="default",
                        parameters={},
                        telemetry_config={}
                    )
                else:
                    # Record model configuration parsing
                    span.set_attribute("model_found", True)
                    span.set_attribute("using_default_config", False)
                    span.set_attribute("provider", model_elem.get("provider", "default"))
                    span.set_attribute("model_name", model_elem.get("name", "default"))
                    
                    config = DSPyModelConfiguration(
                        provider=model_elem.get("provider", "default"),
                        model_name=model_elem.get("name", "default"),
                        parameters=self._extract_parameters(model_elem),
                        telemetry_config=self._extract_telemetry_config(model_elem)
                    )
                
                # Record successful parsing metrics
                span.set_attribute("parse_success", True)
                
                # Record metric for successful parsing
                self.telemetry.record_metric("dspy_model_config_parse_success", 1)
                
                return config
                
            except Exception as e:
                # Record parsing failure
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for parsing failure
                self.telemetry.record_metric("dspy_model_config_parse_failure", 1)
                
                raise

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

 