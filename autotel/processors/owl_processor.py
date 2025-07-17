"""
AutoTel OWL Processor (Unified Architecture)

Processes OWL/RDF XML into ontology definitions.
Implements happy-path parsing and extraction with telemetry and contracts.
"""

from typing import Any, Dict, List, Optional
from pathlib import Path
import xml.etree.ElementTree as ET
from pydantic import Field

from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.contract import contract_precondition, contract_postcondition
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error
from autotel.schemas.ontology_types import OWLOntologyDefinition
from autotel.core.telemetry import get_telemetry_manager_or_noop

@processor_metadata(
    name="owl_processor",
    version="1.0.0",
    capabilities=["owl_parse", "ontology_extraction"],
    supported_formats=["xml", "owl", "rdf"],
    author="AutoTel Team"
)
class OWLProcessor(BaseProcessor):
    """
    Unified OWL processor for AutoTel.
    Parses OWL XML and extracts ontology definition (happy path).
    """
    def __init__(self, config, telemetry=None):
        super().__init__(config)
        self.telemetry = telemetry or get_telemetry_manager_or_noop(service_name="autotel-owl-processor")

    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse OWL XML string and extract ontology definition (happy path only).
        Args:
            data: XML string or file path
        Returns:
            ProcessorResult with OWLOntologyDefinition data
        """
        with create_processor_span("parse", "owl") as span:
            try:
                xml_content = self._get_xml_content(data)
                root = ET.fromstring(xml_content)
                namespace = self._extract_namespace(root)
                ontology_uri = self._extract_ontology_uri(root)
                prefix = "owl"
                
                classes = self._extract_classes(root, namespace)
                object_properties = self._extract_object_properties(root, namespace)
                data_properties = self._extract_data_properties(root, namespace)
                individuals = self._extract_individuals(root, namespace)
                axioms = self._extract_axioms(root, namespace)
                
                owl_result = OWLOntologyDefinition(
                    ontology_uri=ontology_uri,
                    prefix=prefix,
                    namespace=namespace,
                    classes=classes,
                    object_properties=object_properties,
                    data_properties=data_properties,
                    individuals=individuals,
                    axioms=axioms
                )
                
                record_span_success(span, {
                    "classes": len(classes),
                    "object_properties": len(object_properties),
                    "data_properties": len(data_properties),
                    "individuals": len(individuals),
                    "axioms": len(axioms)
                })
                
                return ProcessorResult.success_result(
                    data=owl_result,
                    metadata={
                        "classes": len(classes),
                        "object_properties": len(object_properties),
                        "data_properties": len(data_properties),
                        "individuals": len(individuals),
                        "axioms": len(axioms)
                    }
                )
            except Exception as e:
                record_span_error(span, e, {"input_type": type(data).__name__})
                return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})

    def _get_xml_content(self, data: Any) -> str:
        """Get XML content from string or file path."""
        if isinstance(data, str):
            # Only treat as file path if it's short and exists
            if len(data) < 256 and Path(data).exists():
                return Path(data).read_text(encoding="utf-8")
            return data
        elif isinstance(data, Path):
            return data.read_text(encoding="utf-8")
        raise ValueError("Input must be XML string or file path")

    def _extract_namespace(self, root: ET.Element) -> str:
        """Extract default namespace from root element."""
        if 'xmlns' in root.attrib:
            return root.attrib['xmlns']
        if '{http://www.w3.org/XML/1998/namespace}base' in root.attrib:
            base = root.attrib['{http://www.w3.org/XML/1998/namespace}base']
            if not base.endswith('#'):
                base = base.rstrip('/') + '#'
            return base
        return "http://autotel.ai/ontology/default#"

    def _extract_ontology_uri(self, root: ET.Element) -> str:
        """Extract ontology URI from root element."""
        ns = {'owl': 'http://www.w3.org/2002/07/owl#', 'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'}
        ontology_elem = root.find('.//owl:Ontology', ns)
        if ontology_elem is not None:
            return ontology_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
        return "http://autotel.ai/ontology/workflow"

    def _expand_uri(self, uri: str, namespace: str) -> str:
        """Expand a relative URI (e.g., #ScrumProject) to a full URI using the namespace."""
        if uri.startswith("#"):
            return namespace.rstrip('#') + uri
        return uri

    def _extract_classes(self, root: ET.Element, namespace: str) -> Dict[str, Any]:
        ns = {'owl': 'http://www.w3.org/2002/07/owl#', 'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'rdfs': 'http://www.w3.org/2000/01/rdf-schema#'}
        classes = {}
        for class_elem in root.findall('.//owl:Class', ns):
            class_uri = class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            class_uri = self._expand_uri(class_uri, namespace)
            class_name = class_uri.split('#')[-1] if '#' in class_uri else class_uri.split('/')[-1]
            classes[class_name] = {
                'uri': class_uri,
                'name': class_name,
                'label': self._extract_label(class_elem, ns),
                'comment': self._extract_comment(class_elem, ns),
                'superclasses': self._extract_superclasses(class_elem, ns),
                'properties': {}
            }
        return classes

    def _extract_object_properties(self, root: ET.Element, namespace: str) -> Dict[str, Any]:
        ns = {'owl': 'http://www.w3.org/2002/07/owl#', 'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'rdfs': 'http://www.w3.org/2000/01/rdf-schema#'}
        properties = {}
        for prop_elem in root.findall('.//owl:ObjectProperty', ns):
            prop_uri = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            prop_uri = self._expand_uri(prop_uri, namespace)
            prop_name = prop_uri.split('#')[-1] if '#' in prop_uri else prop_uri.split('/')[-1]
            properties[prop_name] = {
                'uri': prop_uri,
                'name': prop_name,
                'type': 'object',
                'label': self._extract_label(prop_elem, ns),
                'comment': self._extract_comment(prop_elem, ns),
                'domain': self._extract_domain(prop_elem, ns),
                'range': self._extract_range(prop_elem, ns),
                'cardinality': self._extract_cardinality(prop_elem, ns)
            }
        return properties

    def _extract_data_properties(self, root: ET.Element, namespace: str) -> Dict[str, Any]:
        ns = {'owl': 'http://www.w3.org/2002/07/owl#', 'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#', 'rdfs': 'http://www.w3.org/2000/01/rdf-schema#'}
        properties = {}
        for prop_elem in root.findall('.//owl:DatatypeProperty', ns):
            prop_uri = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            prop_uri = self._expand_uri(prop_uri, namespace)
            prop_name = prop_uri.split('#')[-1] if '#' in prop_uri else prop_uri.split('/')[-1]
            properties[prop_name] = {
                'uri': prop_uri,
                'name': prop_name,
                'type': 'data',
                'label': self._extract_label(prop_elem, ns),
                'comment': self._extract_comment(prop_elem, ns),
                'domain': self._extract_domain(prop_elem, ns),
                'range': self._extract_range(prop_elem, ns),
                'cardinality': self._extract_cardinality(prop_elem, ns)
            }
        return properties

    def _extract_individuals(self, root: ET.Element, namespace: str) -> Dict[str, Any]:
        ns = {'owl': 'http://www.w3.org/2002/07/owl#', 'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'}
        individuals = {}
        for ind_elem in root.findall('.//owl:NamedIndividual', ns):
            ind_uri = ind_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            ind_uri = self._expand_uri(ind_uri, namespace)
            ind_name = ind_uri.split('#')[-1] if '#' in ind_uri else ind_uri.split('/')[-1]
            individuals[ind_name] = {
                'uri': ind_uri,
                'name': ind_name,
                'type': None,
                'label': self._extract_label(ind_elem, ns),
                'comment': self._extract_comment(ind_elem, ns),
                'properties': {}
            }
        return individuals

    def _extract_axioms(self, root: ET.Element, namespace: str) -> List[Dict[str, Any]]:
        # Happy path: skip detailed axiom parsing for now
        return []

    def _extract_label(self, elem: ET.Element, ns: Dict[str, str]) -> str:
        label_elem = elem.find('.//rdfs:label', ns)
        return label_elem.text if label_elem is not None else ""

    def _extract_comment(self, elem: ET.Element, ns: Dict[str, str]) -> str:
        comment_elem = elem.find('.//rdfs:comment', ns)
        return comment_elem.text if comment_elem is not None else ""

    def _extract_superclasses(self, elem: ET.Element, ns: Dict[str, str]) -> List[str]:
        supers = []
        for sub_elem in elem.findall('.//rdfs:subClassOf', ns):
            resource = sub_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            if resource:
                supers.append(resource)
        return supers

    def _extract_domain(self, elem: ET.Element, ns: Dict[str, str]) -> str:
        domain_elem = elem.find('.//rdfs:domain', ns)
        if domain_elem is not None:
            resource = domain_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            return resource
        return ""

    def _extract_range(self, elem: ET.Element, ns: Dict[str, str]) -> str:
        range_elem = elem.find('.//rdfs:range', ns)
        if range_elem is not None:
            resource = range_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            return resource
        return ""

    def _extract_cardinality(self, elem: ET.Element, ns: Dict[str, str]) -> str:
        card_elem = elem.find('.//owl:cardinality', ns)
        if card_elem is not None:
            return card_elem.text or ""
        return "" 
 