"""Ontology compiler for AutoTel semantic execution pipeline."""

from typing import Dict, Any, List
import re

from ..schemas.ontology_types import OWLOntologyDefinition, OntologySchema, ClassSchema, PropertySchema
from ..core.telemetry import get_telemetry_manager_or_noop
from opentelemetry import trace


class OntologyCompiler:
    """Compiles OWL ontology definitions into executable ontology schemas."""

    def __init__(self, telemetry=None):
        """Initialize ontology compiler with telemetry."""
        self.telemetry = telemetry or get_telemetry_manager_or_noop(
            service_name="autotel-ontology-compiler",
            require_linkml_validation=False
        )

    def compile(self, ontology_def: OWLOntologyDefinition) -> OntologySchema:
        """Compile ontology definition into schema."""
        with self.telemetry.start_span("ontology_compile", "schema_compilation") as span:
            try:
                span.set_attribute("input_ontology_uri", ontology_def.ontology_uri)
                span.set_attribute("input_classes_count", len(ontology_def.classes))
                span.set_attribute("input_properties_count", len(ontology_def.object_properties) + len(ontology_def.data_properties))
                
                # Extract components
                classes = self._extract_classes(ontology_def)
                properties = self._extract_properties(ontology_def)
                individuals = self._extract_individuals(ontology_def)
                axioms = self._extract_axioms(ontology_def)
                
                # Create semantic context
                semantic_context = self._create_semantic_context(ontology_def, classes, properties)
                
                # Create examples from individuals
                examples = self._create_examples(individuals)
                
                # Build class dictionary
                class_dict = {cls.name: cls for cls in classes}
                
                span.set_attribute("compilation_success", True)
                span.set_attribute("output_classes_count", len(class_dict))
                span.set_attribute("output_properties_count", len(properties))
                
                return OntologySchema(
                    ontology_uri=ontology_def.ontology_uri,
                    namespace=ontology_def.namespace,
                    prefix=ontology_def.prefix,
                    classes=class_dict,
                    semantic_context=semantic_context,
                    examples=examples
                )
                
            except Exception as e:
                span.set_attribute("compilation_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_classes(self, ontology_def: OWLOntologyDefinition) -> List[ClassSchema]:
        """Extract classes from ontology definition."""
        with self.telemetry.start_span("ontology_extract_classes", "class_analysis") as span:
            try:
                classes = []
                
                for class_name, class_data in ontology_def.classes.items():
                    # Determine semantic type based on class name and properties
                    semantic_type = self._determine_semantic_type(class_name, class_data)
                    
                    # Extract properties for this class
                    class_properties = {}
                    for prop_name, prop_data in ontology_def.object_properties.items():
                        if prop_data.get('domain') == class_name:
                            class_properties[prop_name] = PropertySchema(
                                name=prop_name,
                                uri=prop_data.get('uri', ''),
                                data_type='object',
                                domain=prop_data.get('domain', ''),
                                range=prop_data.get('range', ''),
                                cardinality=prop_data.get('cardinality')
                            )
                    
                    for prop_name, prop_data in ontology_def.data_properties.items():
                        if prop_data.get('domain') == class_name:
                            class_properties[prop_name] = PropertySchema(
                                name=prop_name,
                                uri=prop_data.get('uri', ''),
                                data_type='data',
                                domain=prop_data.get('domain', ''),
                                range=prop_data.get('range', ''),
                                cardinality=prop_data.get('cardinality')
                            )
                    
                    # Extract superclasses
                    superclasses = class_data.get('superclasses', [])
                    
                    # Create class schema
                    class_schema = ClassSchema(
                        name=class_name,
                        uri=class_data.get('uri', ''),
                        semantic_type=semantic_type,
                        properties=class_properties,
                        superclasses=superclasses,
                        description=class_data.get('label', class_data.get('comment', ''))
                    )
                    
                    classes.append(class_schema)
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("classes_count", len(classes))
                
                return classes
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_properties(self, ontology_def: OWLOntologyDefinition) -> List[PropertySchema]:
        """Extract properties from ontology definition."""
        with self.telemetry.start_span("ontology_extract_properties", "slot_analysis") as span:
            try:
                properties = []
                
                # Extract object properties
                for prop_name, prop_data in ontology_def.object_properties.items():
                    property_schema = PropertySchema(
                        name=prop_name,
                        uri=prop_data.get('uri', ''),
                        data_type='object',
                        domain=prop_data.get('domain', ''),
                        range=prop_data.get('range', ''),
                        cardinality=prop_data.get('cardinality')
                    )
                    properties.append(property_schema)
                
                # Extract data properties
                for prop_name, prop_data in ontology_def.data_properties.items():
                    property_schema = PropertySchema(
                        name=prop_name,
                        uri=prop_data.get('uri', ''),
                        data_type='data',
                        domain=prop_data.get('domain', ''),
                        range=prop_data.get('range', ''),
                        cardinality=prop_data.get('cardinality')
                    )
                    properties.append(property_schema)
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("properties_count", len(properties))
                
                return properties
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_individuals(self, ontology_def: OWLOntologyDefinition) -> List[Dict[str, Any]]:
        """Extract individuals from ontology definition."""
        with self.telemetry.start_span("ontology_extract_individuals", "instance_analysis") as span:
            try:
                individuals = []
                
                for individual_name, individual_data in ontology_def.individuals.items():
                    individual_dict = {
                        'name': individual_name,
                        'uri': individual_data.get('uri', ''),
                        'type': individual_data.get('type', ''),
                        'properties': individual_data.get('properties', {}),
                        'label': individual_data.get('label', ''),
                        'comment': individual_data.get('comment', '')
                    }
                    individuals.append(individual_dict)
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("individuals_count", len(individuals))
                
                return individuals
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _extract_axioms(self, ontology_def: OWLOntologyDefinition) -> List[Dict[str, Any]]:
        """Extract axioms from ontology definition."""
        with self.telemetry.start_span("ontology_extract_axioms", "constraint_validation") as span:
            try:
                # Return axioms as-is since they're already in the right format
                axioms = ontology_def.axioms
                
                span.set_attribute("extraction_success", True)
                span.set_attribute("axioms_count", len(axioms))
                
                return axioms
                
            except Exception as e:
                span.set_attribute("extraction_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _create_ontology_class(self, class_data: Dict[str, Any]) -> ClassSchema:
        """Create ontology class from class data."""
        with self.telemetry.start_span("ontology_create_class", "class_analysis") as span:
            try:
                semantic_type = self._determine_semantic_type(class_data.get('name', ''), class_data)
                
                class_schema = ClassSchema(
                    name=class_data.get('name', ''),
                    uri=class_data.get('uri', ''),
                    semantic_type=semantic_type,
                    properties=class_data.get('properties', {}),
                    superclasses=class_data.get('superclasses', []),
                    description=class_data.get('label', class_data.get('comment', ''))
                )
                
                span.set_attribute("creation_success", True)
                
                return class_schema
                
            except Exception as e:
                span.set_attribute("creation_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _create_ontology_property(self, property_data: Dict[str, Any]) -> PropertySchema:
        """Create ontology property from property data."""
        with self.telemetry.start_span("ontology_create_property", "slot_analysis") as span:
            try:
                property_schema = PropertySchema(
                    name=property_data.get('name', ''),
                    uri=property_data.get('uri', ''),
                    data_type=property_data.get('data_type', 'object'),
                    domain=property_data.get('domain', ''),
                    range=property_data.get('range', ''),
                    cardinality=property_data.get('cardinality')
                )
                
                span.set_attribute("creation_success", True)
                
                return property_schema
                
            except Exception as e:
                span.set_attribute("creation_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                raise

    def _determine_semantic_type(self, class_name: str, class_data: Dict[str, Any]) -> str:
        """Determine semantic type based on class name and properties."""
        name_lower = class_name.lower()
        
        # Check for user input patterns
        if any(pattern in name_lower for pattern in ['input', 'request', 'query', 'user', 'form']):
            return 'user_input'
        
        # Check for recommendation patterns
        if any(pattern in name_lower for pattern in ['recommendation', 'suggestion', 'proposal', 'option']):
            return 'recommendation'
        
        # Check for decision patterns
        if any(pattern in name_lower for pattern in ['decision', 'choice', 'selection', 'judgment']):
            return 'decision'
        
        # Check for analysis patterns
        if any(pattern in name_lower for pattern in ['analysis', 'report', 'result', 'outcome']):
            return 'analysis'
        
        # Check for reasoning patterns
        if any(pattern in name_lower for pattern in ['reasoning', 'logic', 'inference', 'conclusion']):
            return 'reasoning'
        
        # Default to general
        return 'general'

    def _create_semantic_context(self, ontology_def: OWLOntologyDefinition, 
                                classes: List[ClassSchema], 
                                properties: List[PropertySchema]) -> Dict[str, Any]:
        """Create semantic context for the ontology."""
        context = {
            'domain': self._extract_domain(ontology_def.ontology_uri),
            'semantic_types': {
                'user_input': [cls.name for cls in classes if cls.semantic_type == 'user_input'],
                'recommendation': [cls.name for cls in classes if cls.semantic_type == 'recommendation'],
                'decision': [cls.name for cls in classes if cls.semantic_type == 'decision'],
                'analysis': [cls.name for cls in classes if cls.semantic_type == 'analysis'],
                'reasoning': [cls.name for cls in classes if cls.semantic_type == 'reasoning'],
                'general': [cls.name for cls in classes if cls.semantic_type == 'general']
            },
            'property_types': {
                'object': [prop.name for prop in properties if prop.data_type == 'object'],
                'data': [prop.name for prop in properties if prop.data_type == 'data']
            }
        }
        return context

    def _extract_domain(self, ontology_uri: str) -> str:
        """Extract domain from ontology URI."""
        # Simple domain extraction from URI
        if 'workflow' in ontology_uri.lower():
            return 'workflow'
        elif 'business' in ontology_uri.lower():
            return 'business'
        elif 'process' in ontology_uri.lower():
            return 'process'
        else:
            return 'general'

    def _create_examples(self, individuals: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Create examples from individuals."""
        examples = []
        for individual in individuals:
            example = {
                'name': individual['name'],
                'type': individual['type'],
                'properties': individual['properties'],
                'description': individual.get('label', individual.get('comment', ''))
            }
            examples.append(example)
        return examples 