"""
SHACL Processor - Isolated SHACL Shapes Parser

Implements AUTOTEL-104: Parse custom <shacl:shapes> blocks from XML into 
standard rdflib.Graph objects for data schema validation.
"""

from typing import Optional
from rdflib import Graph
import lxml.etree as etree


class SHACLProcessor:
    """
    Isolated SHACL processor that parses custom <shacl:shapes> blocks from XML
    into standard rdflib.Graph objects for data schema validation.
    """
    
    def __init__(self):
        """Initialize the SHACL processor."""
        pass
    
    def parse(self, xml_fragment: str) -> Graph:
        """
        Parse SHACL shapes from XML fragment into rdflib.Graph.
        
        Args:
            xml_fragment: XML string containing SHACL shapes
            
        Returns:
            Graph: rdflib.Graph containing SHACL shapes. Returns empty graph if no shapes found.
        """
        try:
            # Parse XML fragment
            root = etree.fromstring(xml_fragment.encode('utf-8'))
            
            # Find shacl:shapes block
            shacl_ns = {'shacl': 'http://www.w3.org/ns/shacl#'}
            shapes_elements = root.xpath('.//shacl:shapes', namespaces=shacl_ns)
            
            if not shapes_elements:
                # No SHACL shapes found, return empty graph
                return Graph()
            
            # Get the first shapes element
            shapes_element = shapes_elements[0]
            
            # Find RDF content within the shapes block
            rdf_elements = shapes_element.xpath('.//rdf:RDF', namespaces={'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'})
            
            if not rdf_elements:
                # No RDF content found, return empty graph
                return Graph()
            
            # Get the RDF content
            rdf_element = rdf_elements[0]
            
            # Convert to string and parse into rdflib.Graph
            rdf_xml = etree.tostring(rdf_element, encoding='unicode')
            
            graph = Graph()
            graph.parse(data=rdf_xml, format='xml')
            
            return graph
            
        except etree.XMLSyntaxError:
            # If XML is malformed, return empty graph
            return Graph()
        except Exception:
            # Handle any other errors gracefully
            return Graph()
    
    def parse_file(self, file_path: str) -> Graph:
        """
        Parse SHACL shapes from file into rdflib.Graph.
        
        Args:
            file_path: Path to XML file containing SHACL shapes
            
        Returns:
            Graph: rdflib.Graph containing SHACL shapes. Returns empty graph if no shapes found.
            
        Raises:
            FileNotFoundError: If file does not exist
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                xml_content = f.read()
            
            return self.parse(xml_content)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"SHACL file not found: {file_path}")
        except Exception:
            # Handle any other errors gracefully
            return Graph()
    
    def find_shapes_in_bpmn(self, bpmn_xml: str) -> Graph:
        """
        Find and parse SHACL shapes embedded in BPMN XML.
        
        Args:
            bpmn_xml: BPMN XML string that may contain embedded SHACL shapes
            
        Returns:
            Graph: rdflib.Graph containing SHACL shapes. Returns empty graph if no shapes found.
        """
        try:
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            # Find all shacl:shapes elements within BPMN
            shacl_ns = {'shacl': 'http://www.w3.org/ns/shacl#'}
            shapes_elements = root.xpath('.//shacl:shapes', namespaces=shacl_ns)
            
            if not shapes_elements:
                return Graph()
            
            # Combine all shapes elements
            combined_graph = Graph()
            
            for shapes_element in shapes_elements:
                # Find RDF content within each shapes block
                rdf_elements = shapes_element.xpath('.//rdf:RDF', namespaces={'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'})
                
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