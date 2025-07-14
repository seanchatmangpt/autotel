"""
OWL (Web Ontology Language) Integration for AutoTel
Supports the Five Pillars architecture: BPMN + DMN + DSPy + SHACL + OWL

This module provides OWL ontology support including:
- OWL XML parsing and validation
- Ontology reasoning and inference
- Integration with SHACL validation
- Semantic workflow capabilities
- OWL-based data contracts
"""

from lxml import etree as ET
from typing import Dict, Any, Optional, List, Set, Tuple
from dataclasses import dataclass, field
from pathlib import Path
import logging
from rdflib import Graph as RDFGraph, URIRef, Literal, Namespace, RDF, RDFS, OWL, XSD
from rdflib.plugins.sparql import prepareQuery
import pyshacl
try:
    from owlready2 import *
    OWLREADY2_AVAILABLE = True
except ImportError:
    OWLREADY2_AVAILABLE = False
    print("Warning: OwlReady2 not available. OWL reasoning will be disabled.")

# Configure logging
logger = logging.getLogger(__name__)

@dataclass
class OWLOntologyDefinition:
    """Definition of an OWL ontology from XML"""
    ontology_uri: str
    prefix: str
    namespace: str
    classes: Dict[str, 'OWLClassDefinition'] = field(default_factory=dict)
    object_properties: Dict[str, 'OWLObjectPropertyDefinition'] = field(default_factory=dict)
    data_properties: Dict[str, 'OWLDataPropertyDefinition'] = field(default_factory=dict)
    individuals: Dict[str, 'OWLIndividualDefinition'] = field(default_factory=dict)
    axioms: List['OWLAxiom'] = field(default_factory=list)

@dataclass
class OWLClassDefinition:
    """Definition of an OWL class"""
    class_uri: str
    label: str
    comment: Optional[str] = None
    superclasses: List[str] = field(default_factory=list)
    equivalent_classes: List[str] = field(default_factory=list)
    disjoint_classes: List[str] = field(default_factory=list)
    restrictions: List['OWLRestriction'] = field(default_factory=list)

@dataclass
class OWLObjectPropertyDefinition:
    """Definition of an OWL object property"""
    property_uri: str
    label: str
    domain: Optional[str] = None
    range: Optional[str] = None
    functional: bool = False
    inverse_functional: bool = False
    symmetric: bool = False
    transitive: bool = False
    inverse_of: Optional[str] = None

@dataclass
class OWLDataPropertyDefinition:
    """Definition of an OWL data property"""
    property_uri: str
    label: str
    domain: Optional[str] = None
    range: Optional[str] = None
    functional: bool = False
    datatype: Optional[str] = None

@dataclass
class OWLIndividualDefinition:
    """Definition of an OWL individual"""
    individual_uri: str
    label: str
    class_types: List[str] = field(default_factory=list)
    object_property_values: Dict[str, List[str]] = field(default_factory=dict)
    data_property_values: Dict[str, List[str]] = field(default_factory=dict)

@dataclass
class OWLRestriction:
    """Definition of an OWL restriction"""
    restriction_type: str  # 'allValuesFrom', 'someValuesFrom', 'hasValue', 'minCardinality', 'maxCardinality', 'exactCardinality'
    property_uri: str
    value: Any
    cardinality: Optional[int] = None

@dataclass
class OWLAxiom:
    """Definition of an OWL axiom"""
    axiom_type: str  # 'SubClassOf', 'EquivalentClasses', 'DisjointClasses', etc.
    subject: str
    predicate: Optional[str] = None
    object_value: Optional[str] = None

class OWLXMLParser:
    """Parser for OWL XML files"""
    
    def __init__(self):
        self.ontologies: Dict[str, OWLOntologyDefinition] = {}
        self.owl_graph = RDFGraph()  # rdflib Graph
        self.owlready_ontology = None
    
    def parse_owl_xml(self, owl_xml_content: str, ontology_name: str = "default") -> OWLOntologyDefinition:
        """Parse OWL XML content and create ontology definition"""
        try:
            # Parse XML
            root = ET.fromstring(owl_xml_content)
            
            # Extract namespace information
            namespaces = {
                'owl': 'http://www.w3.org/2002/07/owl#',
                'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
                'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
                'xsd': 'http://www.w3.org/2001/XMLSchema#'
            }
            
            # Find ontology element
            ontology_elem = root.find('.//owl:Ontology', namespaces)
            if ontology_elem is None:
                raise ValueError("No owl:Ontology element found in XML")
            
            # Extract ontology URI
            ontology_uri = ontology_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            if not ontology_uri:
                ontology_uri = f"http://autotel.ai/ontology/{ontology_name}"
            
            # Create ontology definition
            ontology_def = OWLOntologyDefinition(
                ontology_uri=ontology_uri,
                prefix=ontology_name,
                namespace=ontology_uri + "#"
            )
            
            # Parse classes
            self._parse_classes(root, ontology_def, namespaces)
            
            # Parse object properties
            self._parse_object_properties(root, ontology_def, namespaces)
            
            # Parse data properties
            self._parse_data_properties(root, ontology_def, namespaces)
            
            # Parse individuals
            self._parse_individuals(root, ontology_def, namespaces)
            
            # Parse axioms
            self._parse_axioms(root, ontology_def, namespaces)
            
            # Store ontology
            self.ontologies[ontology_name] = ontology_def
            
            # Create RDF graph
            self._create_rdf_graph(ontology_def)
            
            logger.info(f"✅ Parsed OWL ontology: {ontology_name}")
            logger.info(f"   Classes: {len(ontology_def.classes)}")
            logger.info(f"   Object Properties: {len(ontology_def.object_properties)}")
            logger.info(f"   Data Properties: {len(ontology_def.data_properties)}")
            logger.info(f"   Individuals: {len(ontology_def.individuals)}")
            logger.info(f"   Axioms: {len(ontology_def.axioms)}")
            
            return ontology_def
            
        except Exception as e:
            logger.error(f"Failed to parse OWL XML: {e}")
            raise
    
    def _parse_classes(self, root: ET.Element, ontology_def: OWLOntologyDefinition, namespaces: Dict[str, str]):
        """Parse OWL classes from XML"""
        for class_elem in root.findall('.//owl:Class', namespaces):
            class_uri = class_uri = class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            if not class_uri:
                continue
            
            # Extract label and comment
            label = self._get_literal_value(class_elem, 'rdfs:label', namespaces)
            comment = self._get_literal_value(class_elem, 'rdfs:comment', namespaces)
            
            # Extract superclasses
            superclasses = []
            for sub_class_elem in class_elem.findall('.//rdfs:subClassOf', namespaces):
                superclass_uri = sub_class_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
                if superclass_uri:
                    superclasses.append(superclass_uri)
            
            # Extract restrictions
            restrictions = []
            for restriction_elem in class_elem.findall('.//owl:Restriction', namespaces):
                restriction = self._parse_restriction(restriction_elem, namespaces)
                if restriction:
                    restrictions.append(restriction)
            
            class_def = OWLClassDefinition(
                class_uri=class_uri,
                label=label or class_uri.split('#')[-1],
                comment=comment,
                superclasses=superclasses,
                restrictions=restrictions
            )
            
            ontology_def.classes[class_uri] = class_def
    
    def _parse_object_properties(self, root: ET.Element, ontology_def: OWLOntologyDefinition, namespaces: Dict[str, str]):
        """Parse OWL object properties from XML"""
        for prop_elem in root.findall('.//owl:ObjectProperty', namespaces):
            prop_uri = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            if not prop_uri:
                continue
            
            label = self._get_literal_value(prop_elem, 'rdfs:label', namespaces)
            domain = self._get_resource_value(prop_elem, 'rdfs:domain', namespaces)
            range_val = self._get_resource_value(prop_elem, 'rdfs:range', namespaces)
            
            # Check property characteristics
            functional = len(prop_elem.findall('.//rdf:type[@rdf:resource="http://www.w3.org/2002/07/owl#FunctionalProperty"]', namespaces)) > 0
            inverse_functional = len(prop_elem.findall('.//rdf:type[@rdf:resource="http://www.w3.org/2002/07/owl#InverseFunctionalProperty"]', namespaces)) > 0
            symmetric = len(prop_elem.findall('.//rdf:type[@rdf:resource="http://www.w3.org/2002/07/owl#SymmetricProperty"]', namespaces)) > 0
            transitive = len(prop_elem.findall('.//rdf:type[@rdf:resource="http://www.w3.org/2002/07/owl#TransitiveProperty"]', namespaces)) > 0
            
            prop_def = OWLObjectPropertyDefinition(
                property_uri=prop_uri,
                label=label or prop_uri.split('#')[-1],
                domain=domain,
                range=range_val,
                functional=functional,
                inverse_functional=inverse_functional,
                symmetric=symmetric,
                transitive=transitive
            )
            
            ontology_def.object_properties[prop_uri] = prop_def
    
    def _parse_data_properties(self, root: ET.Element, ontology_def: OWLOntologyDefinition, namespaces: Dict[str, str]):
        """Parse OWL data properties from XML"""
        for prop_elem in root.findall('.//owl:DatatypeProperty', namespaces):
            prop_uri = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            if not prop_uri:
                continue
            
            label = self._get_literal_value(prop_elem, 'rdfs:label', namespaces)
            domain = self._get_resource_value(prop_elem, 'rdfs:domain', namespaces)
            range_val = self._get_resource_value(prop_elem, 'rdfs:range', namespaces)
            functional = len(prop_elem.findall('.//rdf:type[@rdf:resource="http://www.w3.org/2002/07/owl#FunctionalProperty"]', namespaces)) > 0
            
            prop_def = OWLDataPropertyDefinition(
                property_uri=prop_uri,
                label=label or prop_uri.split('#')[-1],
                domain=domain,
                range=range_val,
                functional=functional,
                datatype=range_val
            )
            
            ontology_def.data_properties[prop_uri] = prop_def
    
    def _parse_individuals(self, root: ET.Element, ontology_def: OWLOntologyDefinition, namespaces: Dict[str, str]):
        """Parse OWL individuals from XML"""
        for ind_elem in root.findall('.//owl:NamedIndividual', namespaces):
            ind_uri = ind_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            if not ind_uri:
                continue
            
            label = self._get_literal_value(ind_elem, 'rdfs:label', namespaces)
            
            # Extract class types
            class_types = []
            for type_elem in ind_elem.findall('.//rdf:type', namespaces):
                class_uri = type_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
                if class_uri:
                    class_types.append(class_uri)
            
            # Extract property values
            object_property_values = {}
            data_property_values = {}
            
            for prop_elem in ind_elem.findall('.//*'):
                if prop_elem.tag.endswith('}') and '#' in prop_elem.tag:
                    prop_uri = prop_elem.tag.split('}')[1]
                    if prop_uri in ontology_def.object_properties:
                        values = object_property_values.get(prop_uri, [])
                        resource = prop_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
                        if resource:
                            values.append(resource)
                        object_property_values[prop_uri] = values
                    elif prop_uri in ontology_def.data_properties:
                        values = data_property_values.get(prop_uri, [])
                        literal = prop_elem.text
                        if literal:
                            values.append(literal)
                        data_property_values[prop_uri] = values
            
            ind_def = OWLIndividualDefinition(
                individual_uri=ind_uri,
                label=label or ind_uri.split('#')[-1],
                class_types=class_types,
                object_property_values=object_property_values,
                data_property_values=data_property_values
            )
            
            ontology_def.individuals[ind_uri] = ind_def
    
    def _parse_axioms(self, root: ET.Element, ontology_def: OWLOntologyDefinition, namespaces: Dict[str, str]):
        """Parse OWL axioms from XML"""
        # Parse SubClassOf axioms
        for axiom_elem in root.findall('.//rdfs:subClassOf', namespaces):
            subject = axiom_elem.getparent().get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            object_val = axiom_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
            if subject and object_val:
                axiom = OWLAxiom(
                    axiom_type='SubClassOf',
                    subject=subject,
                    object_value=object_val
                )
                ontology_def.axioms.append(axiom)
        
        # Parse EquivalentClasses axioms
        for axiom_elem in root.findall('.//owl:equivalentClass', namespaces):
            subject = axiom_elem.getparent().get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}about')
            object_val = axiom_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
            if subject and object_val:
                axiom = OWLAxiom(
                    axiom_type='EquivalentClasses',
                    subject=subject,
                    object_value=object_val
                )
                ontology_def.axioms.append(axiom)
    
    def _parse_restriction(self, restriction_elem: ET.Element, namespaces: Dict[str, str]) -> Optional[OWLRestriction]:
        """Parse OWL restriction from XML"""
        # Find restriction type and property
        for child in restriction_elem:
            if child.tag.endswith('}onProperty'):
                property_uri = child.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource')
                if not property_uri:
                    continue
                
                # Find restriction type
                for restriction_type in ['allValuesFrom', 'someValuesFrom', 'hasValue', 'minCardinality', 'maxCardinality', 'exactCardinality']:
                    type_elem = restriction_elem.find(f'.//owl:{restriction_type}', namespaces)
                    if type_elem is not None:
                        if restriction_type in ['minCardinality', 'maxCardinality', 'exactCardinality']:
                            cardinality = int(type_elem.text) if type_elem.text else None
                            return OWLRestriction(
                                restriction_type=restriction_type,
                                property_uri=property_uri,
                                value=None,
                                cardinality=cardinality
                            )
                        else:
                            value = type_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource') or type_elem.text
                            return OWLRestriction(
                                restriction_type=restriction_type,
                                property_uri=property_uri,
                                value=value
                            )
        return None
    
    def _get_literal_value(self, elem: ET.Element, tag: str, namespaces: Dict[str, str]) -> Optional[str]:
        """Get literal value from element"""
        tag_elem = elem.find(f'.//{tag}', namespaces)
        return tag_elem.text if tag_elem is not None else None
    
    def _get_resource_value(self, elem: ET.Element, tag: str, namespaces: Dict[str, str]) -> Optional[str]:
        """Get resource value from element"""
        tag_elem = elem.find(f'.//{tag}', namespaces)
        return tag_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource') if tag_elem is not None else None
    
    def _create_rdf_graph(self, ontology_def: OWLOntologyDefinition):
        """Create RDF graph from ontology definition"""
        # Add ontology declaration
        ontology_uri = URIRef(ontology_def.ontology_uri)
        self.owl_graph.add((ontology_uri, RDF.type, OWL.Ontology))
        
        # Add classes
        for class_uri, class_def in ontology_def.classes.items():
            class_ref = URIRef(class_uri)
            self.owl_graph.add((class_ref, RDF.type, OWL.Class))
            if class_def.label:
                self.owl_graph.add((class_ref, RDFS.label, Literal(class_def.label)))
            if class_def.comment:
                self.owl_graph.add((class_ref, RDFS.comment, Literal(class_def.comment)))
            
            # Add superclasses
            for superclass in class_def.superclasses:
                self.owl_graph.add((class_ref, RDFS.subClassOf, URIRef(superclass)))
        
        # Add object properties
        for prop_uri, prop_def in ontology_def.object_properties.items():
            prop_ref = URIRef(prop_uri)
            self.owl_graph.add((prop_ref, RDF.type, OWL.ObjectProperty))
            if prop_def.label:
                self.owl_graph.add((prop_ref, RDFS.label, Literal(prop_def.label)))
            if prop_def.domain:
                self.owl_graph.add((prop_ref, RDFS.domain, URIRef(prop_def.domain)))
            if prop_def.range:
                self.owl_graph.add((prop_ref, RDFS.range, URIRef(prop_def.range)))
        
        # Add data properties
        for prop_uri, prop_def in ontology_def.data_properties.items():
            prop_ref = URIRef(prop_uri)
            self.owl_graph.add((prop_ref, RDF.type, OWL.DatatypeProperty))
            if prop_def.label:
                self.owl_graph.add((prop_ref, RDFS.label, Literal(prop_def.label)))
            if prop_def.domain:
                self.owl_graph.add((prop_ref, RDFS.domain, URIRef(prop_def.domain)))
            if prop_def.range:
                self.owl_graph.add((prop_ref, RDFS.range, URIRef(prop_def.range)))
        
        # Add individuals
        for ind_uri, ind_def in ontology_def.individuals.items():
            ind_ref = URIRef(ind_uri)
            self.owl_graph.add((ind_ref, RDF.type, OWL.NamedIndividual))
            if ind_def.label:
                self.owl_graph.add((ind_ref, RDFS.label, Literal(ind_def.label)))
            
            # Add class types
            for class_type in ind_def.class_types:
                self.owl_graph.add((ind_ref, RDF.type, URIRef(class_type)))

class OWLReasoner:
    """OWL reasoning engine using OwlReady2"""
    
    def __init__(self):
        self.ontologies = {}
        self.reasoners = {}
    
    def load_ontology(self, ontology_def: OWLOntologyDefinition, ontology_name: str = "default"):
        """Load ontology into OwlReady2 for reasoning"""
        if not OWLREADY2_AVAILABLE:
            logger.warning("OwlReady2 not available. Ontology loading skipped.")
            return None
            
        try:
            # Create ontology in OwlReady2
            onto = get_ontology(ontology_def.ontology_uri)
            
            # Add classes
            for class_uri, class_def in ontology_def.classes.items():
                class_name = class_uri.split('#')[-1]
                with onto:
                    # Create class
                    new_class = types.new_class(class_name, (Thing,))
                    
                    # Add label and comment
                    if class_def.label:
                        new_class.label = [class_def.label]
                    if class_def.comment:
                        new_class.comment = [class_def.comment]
                    
                    # Add superclasses
                    for superclass_uri in class_def.superclasses:
                        superclass_name = superclass_uri.split('#')[-1]
                        if hasattr(onto, superclass_name):
                            new_class.is_a.append(getattr(onto, superclass_name))
            
            # Add object properties
            for prop_uri, prop_def in ontology_def.object_properties.items():
                prop_name = prop_uri.split('#')[-1]
                with onto:
                    new_prop = types.new_class(prop_name, (ObjectProperty,))
                    if prop_def.label:
                        new_prop.label = [prop_def.label]
                    if prop_def.domain:
                        domain_name = prop_def.domain.split('#')[-1]
                        if hasattr(onto, domain_name):
                            new_prop.domain = [getattr(onto, domain_name)]
                    if prop_def.range:
                        range_name = prop_def.range.split('#')[-1]
                        if hasattr(onto, range_name):
                            new_prop.range = [getattr(onto, range_name)]
            
            # Add data properties
            for prop_uri, prop_def in ontology_def.data_properties.items():
                prop_name = prop_uri.split('#')[-1]
                with onto:
                    new_prop = types.new_class(prop_name, (DataProperty,))
                    if prop_def.label:
                        new_prop.label = [prop_def.label]
                    if prop_def.domain:
                        domain_name = prop_def.domain.split('#')[-1]
                        if hasattr(onto, domain_name):
                            new_prop.domain = [getattr(onto, domain_name)]
            
            # Add individuals
            for ind_uri, ind_def in ontology_def.individuals.items():
                ind_name = ind_uri.split('#')[-1]
                with onto:
                    new_ind = types.new_class(ind_name, (Thing,))
                    if ind_def.label:
                        new_ind.label = [ind_def.label]
                    
                    # Add class types
                    for class_type_uri in ind_def.class_types:
                        class_type_name = class_type_uri.split('#')[-1]
                        if hasattr(onto, class_type_name):
                            new_ind.is_a.append(getattr(onto, class_type_name))
            
            # Store ontology
            self.ontologies[ontology_name] = onto
            
            # Create reasoner
            reasoner = onto.get_world().new_ontology_reasoner()
            self.reasoners[ontology_name] = reasoner
            
            logger.info(f"✅ Loaded OWL ontology for reasoning: {ontology_name}")
            return onto
            
        except Exception as e:
            logger.error(f"Failed to load ontology for reasoning: {e}")
            raise
    
    def reason(self, ontology_name: str = "default") -> Dict[str, Any]:
        """Perform reasoning on ontology"""
        if not OWLREADY2_AVAILABLE:
            return {
                'consistency': True,
                'inferred_classes': [],
                'inferred_properties': [],
                'inferred_individuals': [],
                'entailments': [],
                'warning': 'OwlReady2 not available for reasoning'
            }
            
        if ontology_name not in self.reasoners:
            raise ValueError(f"Ontology '{ontology_name}' not loaded for reasoning")
        
        reasoner = self.reasoners[ontology_name]
        onto = self.ontologies[ontology_name]
        
        try:
            # Perform reasoning
            reasoner.sync()
            
            # Collect reasoning results
            results = {
                'inferred_classes': [],
                'inferred_properties': [],
                'inferred_individuals': [],
                'consistency': True,
                'entailments': []
            }
            
            # Check for inferred classes
            for cls in onto.classes():
                if hasattr(cls, 'is_a') and cls.is_a:
                    for parent in cls.is_a:
                        if parent != Thing:
                            results['inferred_classes'].append({
                                'class': cls.name,
                                'inferred_parent': parent.name if hasattr(parent, 'name') else str(parent)
                            })
            
            # Check for inferred properties
            for prop in onto.object_properties():
                if hasattr(prop, 'domain') and prop.domain:
                    results['inferred_properties'].append({
                        'property': prop.name,
                        'domain': [d.name if hasattr(d, 'name') else str(d) for d in prop.domain]
                    })
            
            # Check for inferred individuals
            for ind in onto.individuals():
                if hasattr(ind, 'is_a') and ind.is_a:
                    for class_type in ind.is_a:
                        if class_type != Thing:
                            results['inferred_individuals'].append({
                                'individual': ind.name,
                                'inferred_type': class_type.name if hasattr(class_type, 'name') else str(class_type)
                            })
            
            logger.info(f"✅ OWL reasoning completed for: {ontology_name}")
            logger.info(f"   Inferred classes: {len(results['inferred_classes'])}")
            logger.info(f"   Inferred properties: {len(results['inferred_properties'])}")
            logger.info(f"   Inferred individuals: {len(results['inferred_individuals'])}")
            
            return results
            
        except Exception as e:
            logger.error(f"OWL reasoning failed: {e}")
            results['consistency'] = False
            results['error'] = str(e)
            return results

class OWLWorkflowTask:
    """OWL-aware workflow task for BPMN integration"""
    
    def __init__(self, owl_parser: OWLXMLParser, owl_reasoner: OWLReasoner):
        self.owl_parser = owl_parser
        self.owl_reasoner = owl_reasoner
    
    def validate_with_ontology(self, data: Dict[str, Any], ontology_name: str = "default") -> Dict[str, Any]:
        """Validate data against OWL ontology"""
        if ontology_name not in self.owl_parser.ontologies:
            raise ValueError(f"Ontology '{ontology_name}' not found")
        
        ontology_def = self.owl_parser.ontologies[ontology_name]
        validation_results = {
            'valid': True,
            'violations': [],
            'inferences': [],
            'suggestions': []
        }
        
        # Validate data against ontology classes
        for key, value in data.items():
            # Find matching class in ontology
            for class_uri, class_def in ontology_def.classes.items():
                if key.lower() == class_def.label.lower() or key == class_uri.split('#')[-1]:
                    # Validate against class restrictions
                    for restriction in class_def.restrictions:
                        violation = self._validate_restriction(key, value, restriction, ontology_def)
                        if violation:
                            validation_results['violations'].append(violation)
                            validation_results['valid'] = False
        
        # Perform reasoning to get inferences
        if ontology_name in self.owl_reasoner.ontologies:
            reasoning_results = self.owl_reasoner.reason(ontology_name)
            validation_results['inferences'] = reasoning_results.get('entailments', [])
        
        return validation_results
    
    def _validate_restriction(self, key: str, value: Any, restriction: OWLRestriction, ontology_def: OWLOntologyDefinition) -> Optional[str]:
        """Validate data against OWL restriction"""
        if restriction.restriction_type == 'hasValue':
            if str(value) != str(restriction.value):
                return f"Value for '{key}' must be '{restriction.value}' (found: '{value}')"
        elif restriction.restriction_type == 'minCardinality':
            if isinstance(value, (list, tuple)) and len(value) < restriction.cardinality:
                return f"'{key}' must have at least {restriction.cardinality} values (found: {len(value)})"
        elif restriction.restriction_type == 'maxCardinality':
            if isinstance(value, (list, tuple)) and len(value) > restriction.cardinality:
                return f"'{key}' must have at most {restriction.cardinality} values (found: {len(value)})"
        elif restriction.restriction_type == 'exactCardinality':
            if isinstance(value, (list, tuple)) and len(value) != restriction.cardinality:
                return f"'{key}' must have exactly {restriction.cardinality} values (found: {len(value)})"
        
        return None

# Example OWL XML content
EXAMPLE_OWL_XML = '''<?xml version="1.0"?>
<rdf:RDF xmlns="http://autotel.ai/ontology/workflow#"
     xml:base="http://autotel.ai/ontology/workflow"
     xmlns:owl="http://www.w3.org/2002/07/owl#"
     xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
     xmlns:xml="http://www.w3.org/XML/1998/namespace"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema#"
     xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Ontology rdf:about="http://autotel.ai/ontology/workflow"/>
    
    <!-- Classes -->
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#Workflow">
        <rdfs:label>Workflow</rdfs:label>
        <rdfs:comment>A business process workflow</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#Task">
        <rdfs:label>Task</rdfs:label>
        <rdfs:comment>A task within a workflow</rdfs:comment>
        <rdfs:subClassOf rdf:resource="http://autotel.ai/ontology/workflow#WorkflowElement"/>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#WorkflowElement">
        <rdfs:label>WorkflowElement</rdfs:label>
        <rdfs:comment>Base class for workflow elements</rdfs:comment>
    </owl:Class>
    
    <owl:Class rdf:about="http://autotel.ai/ontology/workflow#Customer">
        <rdfs:label>Customer</rdfs:label>
        <rdfs:comment>A customer entity</rdfs:comment>
        <rdfs:subClassOf>
            <owl:Restriction>
                <owl:onProperty rdf:resource="http://autotel.ai/ontology/workflow#hasCustomerId"/>
                <owl:minCardinality rdf:datatype="http://www.w3.org/2001/XMLSchema#nonNegativeInteger">1</owl:minCardinality>
            </owl:Restriction>
        </rdfs:subClassOf>
    </owl:Class>
    
    <!-- Object Properties -->
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/workflow#hasTask">
        <rdfs:label>hasTask</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#Workflow"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/workflow#Task"/>
    </owl:ObjectProperty>
    
    <owl:ObjectProperty rdf:about="http://autotel.ai/ontology/workflow#hasCustomer">
        <rdfs:label>hasCustomer</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#Workflow"/>
        <rdfs:range rdf:resource="http://autotel.ai/ontology/workflow#Customer"/>
    </owl:ObjectProperty>
    
    <!-- Data Properties -->
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/workflow#hasCustomerId">
        <rdfs:label>hasCustomerId</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#Customer"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
    <owl:DatatypeProperty rdf:about="http://autotel.ai/ontology/workflow#hasStatus">
        <rdfs:label>hasStatus</rdfs:label>
        <rdfs:domain rdf:resource="http://autotel.ai/ontology/workflow#Workflow"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
    </owl:DatatypeProperty>
    
    <!-- Individuals -->
    <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/workflow#ApprovedStatus">
        <rdf:type rdf:resource="http://autotel.ai/ontology/workflow#Status"/>
        <rdfs:label>Approved</rdfs:label>
    </owl:NamedIndividual>
    
    <owl:NamedIndividual rdf:about="http://autotel.ai/ontology/workflow#RejectedStatus">
        <rdf:type rdf:resource="http://autotel.ai/ontology/workflow#Status"/>
        <rdfs:label>Rejected</rdfs:label>
    </owl:NamedIndividual>
    
</rdf:RDF>'''

def create_owl_workflow_example():
    """Create an example OWL workflow integration"""
    
    # Create OWL parser and reasoner
    owl_parser = OWLXMLParser()
    owl_reasoner = OWLReasoner()
    
    # Parse OWL XML
    ontology_def = owl_parser.parse_owl_xml(EXAMPLE_OWL_XML, "workflow")
    
    # Load for reasoning
    onto = owl_reasoner.load_ontology(ontology_def, "workflow")
    
    # Create workflow task
    workflow_task = OWLWorkflowTask(owl_parser, owl_reasoner)
    
    # Example data validation
    test_data = {
        'Customer': {
            'hasCustomerId': 'CUST_12345',
            'hasName': 'John Doe'
        },
        'Workflow': {
            'hasStatus': 'Approved',
            'hasCustomer': 'CUST_12345'
        }
    }
    
    # Validate against ontology
    validation_results = workflow_task.validate_with_ontology(test_data, "workflow")
    
    print("🔍 OWL Workflow Validation Results:")
    print(f"   Valid: {validation_results['valid']}")
    print(f"   Violations: {len(validation_results['violations'])}")
    print(f"   Inferences: {len(validation_results['inferences'])}")
    
    if validation_results['violations']:
        print("   Violations:")
        for violation in validation_results['violations']:
            print(f"     - {violation}")
    
    return {
        'parser': owl_parser,
        'reasoner': owl_reasoner,
        'workflow_task': workflow_task,
        'validation_results': validation_results
    }

if __name__ == "__main__":
    create_owl_workflow_example() 