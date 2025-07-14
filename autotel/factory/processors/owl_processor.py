"""
OWL Processor - Isolated OWL Ontology Parser

Implements AUTOTEL-104: Parse custom <owl:Ontology> blocks from XML into 
standard rdflib.Graph objects for knowledge representation.
"""

from typing import Optional
from rdflib import Graph
import lxml.etree as etree


class OWLProcessor:
    """
    Isolated OWL processor that parses custom <owl:Ontology> blocks from XML
    into standard rdflib.Graph objects for knowledge representation.
    """
    
    def __init__(self):
        """Initialize the OWL processor."""
        pass
    
    def parse(self, xml_fragment: str) -> Graph:
        """
        Parse OWL ontologies from XML fragment into rdflib.Graph.
        
        Args:
            xml_fragment: XML string containing OWL ontologies
            
        Returns:
            Graph: rdflib.Graph containing OWL ontologies. Returns empty graph if no ontologies found.
        """
        try:
            # Parse XML fragment
            root = etree.fromstring(xml_fragment.encode('utf-8'))
            
            # Find owl:Ontology blocks
            owl_ns = {'owl': 'http://www.w3.org/2002/07/owl#'}
            ontology_elements = root.xpath('.//owl:Ontology', namespaces=owl_ns)
            
            if not ontology_elements:
                # No OWL ontologies found, return empty graph
                return Graph()
            
            # Combine all ontology elements
            combined_graph = Graph()
            
            for ontology_element in ontology_elements:
                # Find RDF content within each ontology block
                rdf_elements = ontology_element.xpath('.//rdf:RDF', namespaces={'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'})
                
                for rdf_element in rdf_elements:
                    rdf_xml = etree.tostring(rdf_element, encoding='unicode')
                    graph = Graph()
                    graph.parse(data=rdf_xml, format='xml')
                    combined_graph += graph
            
            return combined_graph
            
        except etree.XMLSyntaxError:
            # If XML is malformed, return empty graph
            return Graph()
        except Exception:
            # Handle any other errors gracefully
            return Graph()
    
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
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                xml_content = f.read()
            
            return self.parse(xml_content)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"OWL file not found: {file_path}")
        except Exception:
            # Handle any other errors gracefully
            return Graph()
    
    def find_ontologies_in_bpmn(self, bpmn_xml: str) -> Graph:
        """
        Find and parse OWL ontologies embedded in BPMN XML.
        
        Args:
            bpmn_xml: BPMN XML string that may contain embedded OWL ontologies
            
        Returns:
            Graph: rdflib.Graph containing OWL ontologies. Returns empty graph if no ontologies found.
        """
        try:
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            # Find all owl:Ontology elements within BPMN
            owl_ns = {'owl': 'http://www.w3.org/2002/07/owl#'}
            ontology_elements = root.xpath('.//owl:Ontology', namespaces=owl_ns)
            
            if not ontology_elements:
                return Graph()
            
            # Combine all ontology elements
            combined_graph = Graph()
            
            for ontology_element in ontology_elements:
                # Find RDF content within each ontology block
                rdf_elements = ontology_element.xpath('.//rdf:RDF', namespaces={'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'})
                
                for rdf_element in rdf_elements:
                    rdf_xml = etree.tostring(rdf_element, encoding='unicode')
                    graph = Graph()
                    graph.parse(data=rdf_xml, format='xml')
                    combined_graph += graph
            
            return combined_graph
            
        except etree.XMLSyntaxError:
            # If BPMN XML is malformed, return empty graph
            return Graph()
        except Exception:
            # Handle any other errors gracefully
            return Graph() 