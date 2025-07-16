"""OWL processor for AutoTel semantic execution pipeline."""

import xml.etree.ElementTree as ET
from typing import Dict, Any, List, Optional
import re

from ...schemas.ontology_types import OWLOntologyDefinition
from ...core.telemetry import get_telemetry_manager_or_noop, TelemetryManager, NoOpTelemetryManager
from opentelemetry import trace


class OWLProcessor:
    """Processes OWL/RDF XML into structured ontology definitions."""

    def __init__(self, telemetry: Optional[TelemetryManager] = None, force_noop: bool = False):
        """Initialize OWL processor with telemetry.
        
        Args:
            telemetry: Optional telemetry manager. If None, creates one with fallback.
            force_noop: If True, forces no-op telemetry mode.
        """
        if telemetry is not None:
            self.telemetry = telemetry
        else:
            self.telemetry = get_telemetry_manager_or_noop(
                service_name="autotel-owl-processor",
                require_linkml_validation=False,  # Allow basic telemetry without schema validation
                force_noop=force_noop,
                fallback_to_noop=True,
                log_telemetry_failures=True
            )
        
        # Define OWL namespaces
        self.namespaces = {
            'owl': 'http://www.w3.org/2002/07/owl#',
            'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
            'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
            'xml': 'http://www.w3.org/XML/1998/namespace'
        }

    def parse_ontology_definition(self, xml_content: str, prefix: str = "owl") -> OWLOntologyDefinition:
        """Parse OWL XML content into structured ontology definition."""
        with self.telemetry.start_span("owl_parse_ontology", "schema_loading") as span:
            try:
                # Parse XML
                root = ET.fromstring(xml_content)
                
                # Extract basic ontology info
                ontology_uri = self._extract_ontology_uri(root)
                namespace = self._extract_namespace(root)
                
                # Extract components
                classes = self._extract_classes(root)
                object_properties = self._extract_object_properties(root)
                data_properties = self._extract_data_properties(root)
                individuals = self._extract_individuals(root)
                axioms = self._extract_axioms(root)
                
                # Set span attributes
                span.set_attribute("input_size_bytes", len(xml_content))
                span.set_attribute("input_format", "xml")
                span.set_attribute("prefix", prefix)
                span.set_attribute("ontology_uri", ontology_uri)
                span.set_attribute("namespace", namespace)
                span.set_attribute("classes_count", len(classes))
                span.set_attribute("object_properties_count", len(object_properties))
                span.set_attribute("data_properties_count", len(data_properties))
                span.set_attribute("individuals_count", len(individuals))
                span.set_attribute("axioms_count", len(axioms))
                span.set_attribute("parse_success", True)
                
                # Record success metric
                try:
                    self.telemetry.record_metric("owl_ontology_parsed", 1)
                except Exception:
                    pass  # Ignore metric recording failures
                
                return OWLOntologyDefinition(
                    ontology_uri=ontology_uri,
                    prefix=prefix,
                    namespace=namespace,
                    classes=classes,
                    object_properties=object_properties,
                    data_properties=data_properties,
                    individuals=individuals,
                    axioms=axioms
                )
                
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                
                # Record failure metric
                try:
                    self.telemetry.record_metric("owl_parse_failure", 1)
                except Exception:
                    pass  # Ignore metric recording failures
                
                raise

    def _extract_ontology_uri(self, root: ET.Element) -> str:
        """Extract ontology URI from root element."""
        # Look for ontology element
        ontology_elem = root.find('.//owl:Ontology', self.namespaces)
        if ontology_elem is not None:
            return ontology_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
        
        # Fallback: extract from namespace
        for prefix, uri in root.nsmap.items() if hasattr(root, 'nsmap') else []:
            if prefix == '' or prefix is None:
                return uri
        
        return "http://autotel.ai/ontology/workflow"

    def _extract_namespace(self, root: ET.Element) -> str:
        """Extract default namespace."""
        # Try to extract from XML namespace declarations
        for key, value in root.attrib.items():
            if key.startswith('xmlns') and not key.endswith(':'):
                return value
        
        return "http://autotel.ai/ontology/default#"

    def _extract_classes(self, root: ET.Element) -> Dict[str, Any]:
        """Extract OWL classes from XML."""
        with self.telemetry.start_span("owl_extract_classes", "class_analysis") as span:
            try:
                classes = {}
                
                # Find all class elements
                class_elements = root.findall('.//owl:Class', self.namespaces)
                
                for class_elem in class_elements:
                    class_data = self._extract_class_data(class_elem)
                    if class_data:
                        classes[class_data['name']] = class_data
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("classes_count", len(classes))
                
                return classes
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_class_data(self, class_elem: ET.Element) -> Dict[str, Any]:
        """Extract data from a single class element."""
        with self.telemetry.start_span("owl_extract_class_data", "class_analysis") as span:
            try:
                # Get class URI/name
                class_uri = class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
                class_name = self._extract_name_from_uri(class_uri)
                
                if not class_name:
                    return None
                
                class_data = {
                    'uri': class_uri,
                    'name': class_name,
                    'label': self._extract_label(class_elem),
                    'comment': self._extract_comment(class_elem),
                    'superclasses': self._extract_superclasses(class_elem),
                    'properties': {}
                }
                
                span.set_attribute("extraction_success", True)
                
                return class_data
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_object_properties(self, root: ET.Element) -> Dict[str, Any]:
        """Extract OWL object properties from XML."""
        with self.telemetry.start_span("owl_extract_object_properties", "slot_analysis") as span:
            try:
                properties = {}
                
                # Find all object property elements
                prop_elements = root.findall('.//owl:ObjectProperty', self.namespaces)
                
                for prop_elem in prop_elements:
                    prop_data = self._extract_property_data(prop_elem, "object")
                    if prop_data:
                        properties[prop_data['name']] = prop_data
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("properties_count", len(properties))
                
                return properties
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_data_properties(self, root: ET.Element) -> Dict[str, Any]:
        """Extract OWL data properties from XML."""
        with self.telemetry.start_span("owl_extract_data_properties", "slot_analysis") as span:
            try:
                properties = {}
                
                # Find all data property elements
                prop_elements = root.findall('.//owl:DatatypeProperty', self.namespaces)
                
                for prop_elem in prop_elements:
                    prop_data = self._extract_property_data(prop_elem, "data")
                    if prop_data:
                        properties[prop_data['name']] = prop_data
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("properties_count", len(properties))
                
                return properties
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_property_data(self, prop_elem: ET.Element, prop_type: str) -> Dict[str, Any]:
        """Extract data from a single property element."""
        with self.telemetry.start_span("owl_extract_property_data", "slot_analysis") as span:
            try:
                # Get property URI/name
                prop_uri = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
                prop_name = self._extract_name_from_uri(prop_uri)
                
                if not prop_name:
                    return None
                
                prop_data = {
                    'uri': prop_uri,
                    'name': prop_name,
                    'type': prop_type,
                    'label': self._extract_label(prop_elem),
                    'comment': self._extract_comment(prop_elem),
                    'domain': self._extract_domain(prop_elem),
                    'range': self._extract_range(prop_elem),
                    'cardinality': self._extract_cardinality(prop_elem)
                }
                
                span.set_attribute("extraction_success", True)
                
                return prop_data
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_individuals(self, root: ET.Element) -> Dict[str, Any]:
        """Extract OWL individuals from XML."""
        with self.telemetry.start_span("owl_extract_individuals", "class_analysis") as span:
            try:
                individuals = {}
                
                # Find all individual elements
                individual_elements = root.findall('.//owl:NamedIndividual', self.namespaces)
                
                for individual_elem in individual_elements:
                    individual_data = self._extract_individual_data(individual_elem)
                    if individual_data:
                        individuals[individual_data['name']] = individual_data
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("individuals_count", len(individuals))
                
                return individuals
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_individual_data(self, individual_elem: ET.Element) -> Dict[str, Any]:
        """Extract data from a single individual element."""
        with self.telemetry.start_span("owl_extract_individual_data", "class_analysis") as span:
            try:
                # Get individual URI/name
                individual_uri = individual_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
                individual_name = self._extract_name_from_uri(individual_uri)
                
                if not individual_name:
                    return None
                
                individual_data = {
                    'uri': individual_uri,
                    'name': individual_name,
                    'type': self._extract_type(individual_elem),
                    'label': self._extract_label(individual_elem),
                    'comment': self._extract_comment(individual_elem),
                    'properties': self._extract_individual_properties(individual_elem)
                }
                
                span.set_attribute("extraction_success", True)
                
                return individual_data
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_axioms(self, root: ET.Element) -> List[Dict[str, Any]]:
        """Extract OWL axioms from XML."""
        with self.telemetry.start_span("owl_extract_axioms", "constraint_validation") as span:
            try:
                axioms = []
                
                # Extract property restrictions
                restrictions = root.findall('.//owl:Restriction', self.namespaces)
                for restriction_elem in restrictions:
                    restriction_data = self._extract_restriction(restriction_elem)
                    if restriction_data:
                        axioms.append(restriction_data)
                
                # Extract class assertions
                class_assertions = root.findall('.//owl:ClassAssertion', self.namespaces)
                for assertion_elem in class_assertions:
                    assertion_data = self._extract_class_assertion(assertion_elem)
                    if assertion_data:
                        axioms.append(assertion_data)
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("axioms_count", len(axioms))
                
                return axioms
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_name_from_uri(self, uri: str) -> str:
        """Extract name from URI."""
        if not uri:
            return ""
        
        # Extract the last part after # or /
        if '#' in uri:
            return uri.split('#')[-1]
        elif '/' in uri:
            return uri.split('/')[-1]
        else:
            return uri

    def _extract_label(self, elem: ET.Element) -> str:
        """Extract label from element."""
        with self.telemetry.start_span("owl_extract_label", "schema_introspection") as span:
            try:
                label_elem = elem.find('.//rdfs:label', self.namespaces)
                if label_elem is not None:
                    return label_elem.text or ""
                
                span.set_attribute("extraction_success", True)
                return ""
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                return ""

    def _extract_comment(self, elem: ET.Element) -> str:
        """Extract comment from element."""
        with self.telemetry.start_span("owl_extract_comment", "schema_introspection") as span:
            try:
                comment_elem = elem.find('.//rdfs:comment', self.namespaces)
                if comment_elem is not None:
                    return comment_elem.text or ""
                
                span.set_attribute("extraction_success", True)
                return ""
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                return ""

    def _extract_type(self, elem: ET.Element) -> str:
        """Extract type from element."""
        with self.telemetry.start_span("owl_extract_type", "class_analysis") as span:
            try:
                type_elem = elem.find('.//rdf:type', self.namespaces)
                if type_elem is not None:
                    type_uri = type_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
                    return self._extract_name_from_uri(type_uri)
                
                span.set_attribute("extraction_success", True)
                return ""
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                return ""

    def _extract_superclasses(self, elem: ET.Element) -> List[str]:
        """Extract superclasses from element."""
        superclasses = []
        sub_class_elements = elem.findall('.//rdfs:subClassOf', self.namespaces)
        
        for sub_class_elem in sub_class_elements:
            resource = sub_class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            if resource:
                superclasses.append(self._extract_name_from_uri(resource))
        
        return superclasses

    def _extract_domain(self, elem: ET.Element) -> str:
        """Extract domain from element."""
        domain_elem = elem.find('.//rdfs:domain', self.namespaces)
        if domain_elem is not None:
            resource = domain_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            return self._extract_name_from_uri(resource)
        return ""

    def _extract_range(self, elem: ET.Element) -> str:
        """Extract range from element."""
        range_elem = elem.find('.//rdfs:range', self.namespaces)
        if range_elem is not None:
            resource = range_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            return self._extract_name_from_uri(resource)
        return ""

    def _extract_cardinality(self, elem: ET.Element) -> str:
        """Extract cardinality from element."""
        # Look for cardinality restrictions
        cardinality_elements = elem.findall('.//owl:cardinality', self.namespaces)
        if cardinality_elements:
            return cardinality_elements[0].text or ""
        return ""

    def _extract_individual_properties(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract properties from individual element."""
        with self.telemetry.start_span("owl_extract_individual_properties", "slot_analysis") as span:
            try:
                properties = {}
                
                # Find all property assertions
                property_elements = elem.findall('.//*[contains(local-name(), "Property")]', self.namespaces)
                
                for prop_elem in property_elements:
                    prop_name = prop_elem.tag.split('}')[-1] if '}' in prop_elem.tag else prop_elem.tag
                    prop_value = prop_elem.text or ""
                    properties[prop_name] = prop_value
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("properties_count", len(properties))
                
                return properties
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                return {}

    def _extract_restriction(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract restriction data."""
        return {
            'type': 'restriction',
            'property': self._extract_restriction_property(elem),
            'value': self._extract_restriction_value(elem)
        }

    def _extract_restriction_property(self, elem: ET.Element) -> str:
        """Extract property from restriction."""
        property_elem = elem.find('.//owl:onProperty', self.namespaces)
        if property_elem is not None:
            resource = property_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource', '')
            return self._extract_name_from_uri(resource)
        return ""

    def _extract_restriction_value(self, elem: ET.Element) -> str:
        """Extract value from restriction."""
        # Look for various restriction types
        for restriction_type in ['cardinality', 'minCardinality', 'maxCardinality', 'hasValue']:
            value_elem = elem.find(f'.//owl:{restriction_type}', self.namespaces)
            if value_elem is not None:
                return value_elem.text or ""
        return ""

    def _extract_class_assertion(self, elem: ET.Element) -> Dict[str, Any]:
        """Extract class assertion data."""
        return {
            'type': 'class_assertion',
            'class': self._extract_assertion_class(elem),
            'individual': self._extract_assertion_individual(elem)
        }

    def _extract_assertion_class(self, elem: ET.Element) -> str:
        """Extract class from assertion."""
        class_elem = elem.find('.//owl:Class', self.namespaces)
        if class_elem is not None:
            resource = class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            return self._extract_name_from_uri(resource)
        return ""

    def _extract_assertion_individual(self, elem: ET.Element) -> str:
        """Extract individual from assertion."""
        individual_elem = elem.find('.//owl:NamedIndividual', self.namespaces)
        if individual_elem is not None:
            resource = individual_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about', '')
            return self._extract_name_from_uri(resource)
        return ""

    def _get_parent_uri(self, elem: ET.Element) -> str:
        """Get the URI of the parent element."""
        # xml.etree.ElementTree doesn't have getparent(), so we need to track parent manually
        # For now, return empty string as this is complex to implement without parent tracking
        return '' 