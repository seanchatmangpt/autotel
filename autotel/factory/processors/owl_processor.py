"""
OWL Processor - Isolated OWL Ontology Parser

Implements AUTOTEL-104: Parse custom <owl:Ontology> blocks from XML into 
standard rdflib.Graph objects for knowledge representation.
"""

from typing import Optional, List, Dict, Any
from dataclasses import dataclass, field
from rdflib import Graph, URIRef, Literal, Namespace
from rdflib.namespace import RDF, RDFS, OWL, XSD
import lxml.etree as etree
import re


@dataclass
class OWLClassDefinition:
    """
    Data structure representing an OWL class definition.
    
    This captures class metadata including labels, comments, properties,
    and semantic type information for DSPy signature generation.
    """
    uri: str
    name: str
    label: str = ""
    comment: str = ""
    semantic_type: str = "general"  # recommendation, reasoning, user_input, decision, analysis, general
    properties: List[str] = field(default_factory=list)
    superclasses: List[str] = field(default_factory=list)
    restrictions: List[Dict[str, Any]] = field(default_factory=list)


@dataclass
class OWLPropertyDefinition:
    """
    Data structure representing an OWL property definition.
    
    This captures property metadata including domain, range, and type
    information for DSPy signature validation.
    """
    uri: str
    name: str
    label: str = ""
    comment: str = ""
    property_type: str = "object"  # object, datatype
    domain: Optional[str] = None
    range: Optional[str] = None
    cardinality: Optional[str] = None  # min, max, exact


@dataclass
class OWLIndividualDefinition:
    """
    Data structure representing an OWL individual definition.
    
    This captures individual metadata for DSPy signature examples.
    """
    uri: str
    name: str
    label: str = ""
    comment: str = ""
    class_types: List[str] = field(default_factory=list)
    property_values: Dict[str, List[str]] = field(default_factory=dict)


@dataclass
class OWLOntologyDefinition:
    """
    Data structure representing a complete OWL ontology definition.
    
    This is the main data structure that DSPy can consume to understand
    the semantic structure of inputs and outputs for signature generation.
    """
    ontology_uri: str
    prefix: str
    namespace: str
    label: str = ""
    comment: str = ""
    classes: Dict[str, OWLClassDefinition] = field(default_factory=dict)
    object_properties: Dict[str, OWLPropertyDefinition] = field(default_factory=dict)
    data_properties: Dict[str, OWLPropertyDefinition] = field(default_factory=dict)
    individuals: Dict[str, OWLIndividualDefinition] = field(default_factory=dict)
    axioms: List[Dict[str, Any]] = field(default_factory=list)


class OWLProcessor:
    """
    Enhanced OWL processor that parses custom <owl:Ontology> blocks from XML
    into structured dataclasses for DSPy signature generation.
    
    Uses rdflib's built-in XML parser for reliable OWL/RDF parsing.
    """
    
    def __init__(self):
        """Initialize the OWL processor."""
        self.owl_ns = {'owl': 'http://www.w3.org/2002/07/owl#'}
        self.rdf_ns = {'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'}
        self.rdfs_ns = {'rdfs': 'http://www.w3.org/2000/01/rdf-schema#'}
    
    def parse(self, xml_fragment: str) -> Graph:
        """
        Parse OWL ontologies from XML fragment into rdflib.Graph.
        
        Args:
            xml_fragment: XML string containing OWL ontologies
            
        Returns:
            Graph: rdflib.Graph containing OWL ontologies. Returns empty graph if no ontologies found.
        """
        graph = Graph()
        graph.parse(data=xml_fragment, format='xml')
        return graph
            
    def parse_ontology_definition(self, xml_fragment: str, ontology_name: str = "default") -> OWLOntologyDefinition:
        """
        Parse OWL ontologies from XML fragment into structured OWLOntologyDefinition.
        
        Args:
            xml_fragment: XML string containing OWL ontologies
            ontology_name: Name for the ontology
            
        Returns:
            OWLOntologyDefinition: Structured ontology definition for DSPy consumption
        """
        # Use rdflib to parse the XML
        graph = self.parse(xml_fragment)
        
        if len(graph) == 0:
            # No OWL ontologies found, return empty definition
            return OWLOntologyDefinition(
                ontology_uri=f"http://autotel.ai/ontology/{ontology_name}",
                prefix=ontology_name,
                namespace=f"http://autotel.ai/ontology/{ontology_name}#"
            )
        
        # Extract ontology URI from graph
        ontology_uri = self._extract_ontology_uri(graph, ontology_name)
        
        # Create ontology definition
        ontology_def = OWLOntologyDefinition(
            ontology_uri=ontology_uri,
            prefix=ontology_name,
            namespace=ontology_uri + "#"
        )
        
        # Extract structured data from the graph
        self._extract_classes_from_graph(graph, ontology_def)
        self._extract_properties_from_graph(graph, ontology_def)
        self._extract_individuals_from_graph(graph, ontology_def)
        self._extract_axioms_from_graph(graph, ontology_def)
        
        return ontology_def
    
    def _extract_ontology_uri(self, graph: Graph, ontology_name: str) -> str:
        """Extract ontology URI from rdflib graph."""
        # Look for ontology declarations
        for s, p, o in graph.triples((None, RDF.type, OWL.Ontology)):
            if isinstance(s, URIRef):
                return str(s)
        
        # Fallback to default URI
        return f"http://autotel.ai/ontology/{ontology_name}"
    
    def _extract_classes_from_graph(self, graph: Graph, ontology_def: OWLOntologyDefinition):
        """Extract OWL classes from rdflib graph."""
        for s, p, o in graph.triples((None, RDF.type, OWL.Class)):
            if isinstance(s, URIRef):
                class_uri = str(s)
                class_name = class_uri.split('#')[-1] if '#' in class_uri else class_uri.split('/')[-1]
                
                # Extract label
                label = ""
                for _, _, label_literal in graph.triples((s, RDFS.label, None)):
                    if isinstance(label_literal, Literal):
                        label = str(label_literal)
                        break
                
                # Extract comment
                comment = ""
                for _, _, comment_literal in graph.triples((s, RDFS.comment, None)):
                    if isinstance(comment_literal, Literal):
                        comment = str(comment_literal)
                        break
                
                # Extract superclasses
                superclasses = []
                for _, _, superclass in graph.triples((s, RDFS.subClassOf, None)):
                    if isinstance(superclass, URIRef):
                        superclasses.append(str(superclass))
                
                # Determine semantic type
                semantic_type = self._determine_semantic_type(class_name, label, comment)
                
                class_def = OWLClassDefinition(
                    uri=class_uri,
                    name=class_name,
                    label=label,
                    comment=comment,
                    semantic_type=semantic_type,
                    superclasses=superclasses
                )
                
                ontology_def.classes[class_uri] = class_def
    
    def _extract_properties_from_graph(self, graph: Graph, ontology_def: OWLOntologyDefinition):
        """Extract OWL properties from rdflib graph."""
        # Extract object properties
        for s, p, o in graph.triples((None, RDF.type, OWL.ObjectProperty)):
            if isinstance(s, URIRef):
                prop_uri = str(s)
                prop_name = prop_uri.split('#')[-1] if '#' in prop_uri else prop_uri.split('/')[-1]
                
                # Extract label
                label = ""
                for _, _, label_literal in graph.triples((s, RDFS.label, None)):
                    if isinstance(label_literal, Literal):
                        label = str(label_literal)
                        break
                
                # Extract domain and range
                domain = None
                range_val = None
                
                for _, _, domain_val in graph.triples((s, RDFS.domain, None)):
                    if isinstance(domain_val, URIRef):
                        domain = str(domain_val)
                        break
                
                for _, _, range_literal in graph.triples((s, RDFS.range, None)):
                    if isinstance(range_literal, URIRef):
                        range_val = str(range_literal)
                        break
                
                prop_def = OWLPropertyDefinition(
                    uri=prop_uri,
                    name=prop_name,
                    label=label,
                    property_type="object",
                    domain=domain,
                    range=range_val
                )
                
                ontology_def.object_properties[prop_uri] = prop_def
        
        # Extract data properties
        for s, p, o in graph.triples((None, RDF.type, OWL.DatatypeProperty)):
            if isinstance(s, URIRef):
                prop_uri = str(s)
                prop_name = prop_uri.split('#')[-1] if '#' in prop_uri else prop_uri.split('/')[-1]
                
                # Extract label
                label = ""
                for _, _, label_literal in graph.triples((s, RDFS.label, None)):
                    if isinstance(label_literal, Literal):
                        label = str(label_literal)
                        break
                
                # Extract domain and range
                domain = None
                range_val = None
                
                for _, _, domain_val in graph.triples((s, RDFS.domain, None)):
                    if isinstance(domain_val, URIRef):
                        domain = str(domain_val)
                        break
                
                for _, _, range_literal in graph.triples((s, RDFS.range, None)):
                    if isinstance(range_literal, URIRef):
                        range_val = str(range_literal)
                        break
                
                prop_def = OWLPropertyDefinition(
                    uri=prop_uri,
                    name=prop_name,
                    label=label,
                    property_type="datatype",
                    domain=domain,
                    range=range_val
                )
                
                ontology_def.data_properties[prop_uri] = prop_def
    
    def _extract_individuals_from_graph(self, graph: Graph, ontology_def: OWLOntologyDefinition):
        """Extract OWL individuals from rdflib graph."""
        for s, p, o in graph.triples((None, RDF.type, OWL.NamedIndividual)):
            if isinstance(s, URIRef):
                ind_uri = str(s)
                ind_name = ind_uri.split('#')[-1] if '#' in ind_uri else ind_uri.split('/')[-1]
                
                # Extract label
                label = ""
                for _, _, label_literal in graph.triples((s, RDFS.label, None)):
                    if isinstance(label_literal, Literal):
                        label = str(label_literal)
                        break
                
                # Extract class types
                class_types = []
                for _, _, class_type in graph.triples((s, RDF.type, None)):
                    if isinstance(class_type, URIRef) and class_type != OWL.NamedIndividual:
                        class_types.append(str(class_type))
                
                # Extract property values
                property_values = {}
                for _, prop, value in graph.triples((s, None, None)):
                    if isinstance(prop, URIRef):
                        prop_uri = str(prop)
                        if prop_uri not in property_values:
                            property_values[prop_uri] = []
                        
                        if isinstance(value, Literal):
                            property_values[prop_uri].append(str(value))
                        elif isinstance(value, URIRef):
                            property_values[prop_uri].append(str(value))
                
                ind_def = OWLIndividualDefinition(
                    uri=ind_uri,
                    name=ind_name,
                    label=label,
                    class_types=class_types,
                    property_values=property_values
                )
                
                ontology_def.individuals[ind_uri] = ind_def
    
    def _extract_axioms_from_graph(self, graph: Graph, ontology_def: OWLOntologyDefinition):
        """Extract OWL axioms from rdflib graph."""
        # Extract SubClassOf axioms
        for s, p, o in graph.triples((None, RDFS.subClassOf, None)):
            if isinstance(s, URIRef) and isinstance(o, URIRef):
                axiom = {
                    'type': 'SubClassOf',
                    'subject': str(s),
                    'object': str(o)
                }
                ontology_def.axioms.append(axiom)
        
        # Extract EquivalentClasses axioms
        for s, p, o in graph.triples((None, OWL.equivalentClass, None)):
            if isinstance(s, URIRef) and isinstance(o, URIRef):
                axiom = {
                    'type': 'EquivalentClasses',
                    'subject': str(s),
                    'object': str(o)
                }
                ontology_def.axioms.append(axiom)
    
    def _get_literal_value(self, elem: etree._Element, tag: str, namespaces: Dict[str, str]) -> str:
        """Get literal value from element"""
        tag_elem = elem.find(f'.//{tag}', namespaces)
        return tag_elem.text if tag_elem is not None else ""
    
    def _get_resource_value(self, elem: etree._Element, tag: str, namespaces: Dict[str, str]) -> Optional[str]:
        """Get resource value from element"""
        tag_elem = elem.find(f'.//{tag}', namespaces)
        return tag_elem.get('{http://www.w3.org/1999/02/22-rdf-syntax-ns#}resource') if tag_elem is not None else None
    
    def _determine_semantic_type(self, class_name: str, label: str, comment: str) -> str:
        """Determine semantic type based on class name, label, and comment"""
        text = f"{class_name} {label} {comment}".lower()
        
        if any(word in text for word in ["recommendation", "suggestion", "advice"]):
            return "recommendation"
        elif any(word in text for word in ["reasoning", "explanation", "why", "rationale"]):
            return "reasoning"
        elif any(word in text for word in ["input", "user", "request"]):
            return "user_input"
        elif any(word in text for word in ["decision", "choice", "selection"]):
            return "decision"
        elif any(word in text for word in ["analysis", "evaluation", "assessment"]):
            return "analysis"
        else:
            return "general"
    
    def parse_file(self, file_path: str) -> Graph:
        """
        Parse OWL ontologies from file into rdflib.Graph.
        
        Args:
            file_path: Path to XML file containing OWL ontologies
            
        Returns:
            Graph: rdflib.Graph containing OWL ontologies. Returns empty graph if no ontologies found.
            
        Raises:
            FileNotFoundError: If file does not exist
        """
        with open(file_path, 'r', encoding='utf-8') as f:
            xml_content = f.read()
        
        return self.parse(xml_content)
    
    def parse_ontology_definition_from_file(self, file_path: str, ontology_name: str = "default") -> OWLOntologyDefinition:
        """
        Parse OWL ontologies from file into structured OWLOntologyDefinition.
        
        Args:
            file_path: Path to XML file containing OWL ontologies
            ontology_name: Name for the ontology
            
        Returns:
            OWLOntologyDefinition: Structured ontology definition for DSPy consumption
            
        Raises:
            FileNotFoundError: If file does not exist
        """
        with open(file_path, 'r', encoding='utf-8') as f:
            xml_content = f.read()
        
        return self.parse_ontology_definition(xml_content, ontology_name)
    
    def find_ontologies_in_bpmn(self, bpmn_xml: str) -> Graph:
        """
        Find and parse OWL ontologies embedded in BPMN XML.
        
        Args:
            bpmn_xml: BPMN XML string that may contain embedded OWL ontologies
            
        Returns:
            Graph: rdflib.Graph containing OWL ontologies. Returns empty graph if no ontologies found.
        """
        root = etree.fromstring(bpmn_xml.encode('utf-8'))
        
        # Find all owl:Ontology elements within BPMN
        ontology_elements = root.xpath('.//owl:Ontology', namespaces=self.owl_ns)
        
        if not ontology_elements:
            return Graph()
        
        # Combine all ontology elements
        combined_graph = Graph()
        
        for ontology_element in ontology_elements:
            # Find RDF content within each ontology block
            rdf_elements = ontology_element.xpath('.//rdf:RDF', namespaces=self.rdf_ns)
            
            for rdf_element in rdf_elements:
                rdf_xml = etree.tostring(rdf_element, encoding='unicode')
                graph = Graph()
                graph.parse(data=rdf_xml, format='xml')
                combined_graph += graph
        
        return combined_graph
    
    def find_ontology_definitions_in_bpmn(self, bpmn_xml: str, ontology_name: str = "default") -> OWLOntologyDefinition:
        """
        Find and parse OWL ontologies embedded in BPMN XML into structured definition.
        
        Args:
            bpmn_xml: BPMN XML string that may contain embedded OWL ontologies
            ontology_name: Name for the ontology
            
        Returns:
            OWLOntologyDefinition: Structured ontology definition for DSPy consumption
        """
        # Use the find_ontologies_in_bpmn method to get the graph
        graph = self.find_ontologies_in_bpmn(bpmn_xml)
        
        if len(graph) == 0:
            return OWLOntologyDefinition(
                ontology_uri=f"http://autotel.ai/ontology/{ontology_name}",
                prefix=ontology_name,
                namespace=f"http://autotel.ai/ontology/{ontology_name}#"
            )
        
        # Extract ontology URI from graph
        ontology_uri = self._extract_ontology_uri(graph, ontology_name)
        
        # Create ontology definition
        ontology_def = OWLOntologyDefinition(
            ontology_uri=ontology_uri,
            prefix=ontology_name,
            namespace=ontology_uri + "#"
        )
        
        # Extract structured data from the graph
        self._extract_classes_from_graph(graph, ontology_def)
        self._extract_properties_from_graph(graph, ontology_def)
        self._extract_individuals_from_graph(graph, ontology_def)
        self._extract_axioms_from_graph(graph, ontology_def)
        
        return ontology_def 