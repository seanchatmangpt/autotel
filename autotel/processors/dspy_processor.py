"""
AutoTel DSPy Processor (Unified Architecture)

Processes DSPy XML into signature definitions and semantic context.
Implements happy-path parsing and extraction with telemetry and contracts.
"""

from typing import Any, Dict, List, Optional
from pathlib import Path
import xml.etree.ElementTree as ET
from pydantic import BaseModel, Field

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error
from autotel.schemas.dspy_types import DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration

# DSPy namespaces
NAMESPACES = {
    'dspy': 'http://autotel.ai/dspy',
    'owl': 'http://www.w3.org/2002/07/owl#',
    'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
    'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
    'shacl': 'http://www.w3.org/ns/shacl#'
}

class DSPyResult(BaseModel):
    """Result model for DSPy processing."""
    signatures: List[DSPySignatureDefinition] = Field(default_factory=list, description="Extracted signatures")
    total_signatures: int = Field(..., description="Total number of signatures found")

@processor_metadata(
    name="dspy_processor",
    version="1.0.0",
    capabilities=["dspy_parse", "signature_extraction"],
    supported_formats=["xml", "dspy"],
    author="AutoTel Team"
)
class DSPyProcessor(BaseProcessor):
    """
    Unified DSPy processor for AutoTel.
    Parses DSPy XML and extracts signatures (happy path).
    """
    def __init__(self, config: Optional[ProcessorConfig] = None):
        if config is None:
            config = ProcessorConfig(name="dspy_processor")
        super().__init__(config)

    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse DSPy XML string and extract signatures (happy path only).
        Args:
            data: XML string or file path
        Returns:
            ProcessorResult with DSPyResult data
        """
        with create_processor_span("parse", "dspy") as span:
            try:
                xml_content = self._get_xml_content(data)
                root = ET.fromstring(xml_content)
                signatures = self._extract_signatures(root)
                dspy_result = DSPyResult(
                    signatures=signatures,
                    total_signatures=len(signatures)
                )
                record_span_success(span, {
                    "signatures": len(signatures)
                })
                return ProcessorResult.success_result(
                    data=dspy_result,
                    metadata={"signatures": len(signatures)}
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

    def _extract_signatures(self, root: ET.Element) -> List[DSPySignatureDefinition]:
        """Extract DSPy signatures from XML (happy path)."""
        signatures = []
        for sig_elem in root.findall(".//dspy:signature", NAMESPACES):
            name = sig_elem.get("name")
            if not name:
                raise ValueError("DSPy signature is missing required 'name' attribute")
            description = sig_elem.get("description", "")
            inputs = {}
            outputs = {}
            for input_elem in sig_elem.findall(".//dspy:input", NAMESPACES):
                input_name = input_elem.get("name")
                if input_name:
                    inputs[input_name] = {
                        "description": input_elem.get("description", ""),
                        "optional": input_elem.get("optional", "false")
                    }
            for output_elem in sig_elem.findall(".//dspy:output", NAMESPACES):
                output_name = output_elem.get("name")
                if output_name:
                    outputs[output_name] = output_elem.get("description", "")
            examples = []
            for example_elem in sig_elem.findall(".//dspy:example", NAMESPACES):
                example = {}
                for child in example_elem:
                    example[child.tag] = child.text or ""
                examples.append(example)
            signatures.append(DSPySignatureDefinition(
                name=name,
                description=description,
                inputs=inputs,
                outputs=outputs,
                examples=examples
            ))
        return signatures

    def parse(self, xml_content: str) -> List[DSPySignatureDefinition]:
        """Parse DSPy XML content into signature definitions (for test compatibility)."""
        try:
            root = ET.fromstring(xml_content)
        except Exception:
            raise ValueError("Invalid XML format")
        return self._extract_signatures(root)

    def parse_file(self, file_path: str) -> List[DSPySignatureDefinition]:
        """Parse DSPy XML from file into signature definitions (for test compatibility)."""
        path = Path(file_path)
        if not path.exists():
            raise FileNotFoundError(f"File not found: {file_path}")
        xml_content = path.read_text(encoding="utf-8")
        return self.parse(xml_content)

    def find_signatures_in_bpmn(self, bpmn_xml: str) -> List[DSPySignatureDefinition]:
        """Find DSPy signatures embedded in BPMN XML (for test compatibility)."""
        try:
            root = ET.fromstring(bpmn_xml)
        except Exception:
            return []
        return self._extract_signatures(root) 