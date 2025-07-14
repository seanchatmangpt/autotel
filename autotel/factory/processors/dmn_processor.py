"""
DMN Processor - Isolated DMN 1.3 Parser

Implements AUTOTEL-102: Parse DMN 1.3 elements from XML into executable 
SpiffWorkflow decision objects, ignoring all non-DMN elements.
"""

from typing import Dict, Any
from SpiffWorkflow.camunda.parser import CamundaParser
import lxml.etree as etree


class DMNProcessor:
    """
    Isolated DMN processor that exclusively uses the trusted SpiffWorkflow.camunda.parser.CamundaParser
    for parsing DMN 1.3 elements, completely ignoring all non-DMN elements.
    """
    
    def __init__(self):
        """Initialize the DMN processor with a CamundaParser instance."""
        self.parser = CamundaParser()
    
    def parse(self, xml_string: str) -> Dict[str, Any]:
        """
        Parse DMN XML string and return executable DMN decision objects.
        
        Args:
            xml_string: DMN XML content as string
            
        Returns:
            Dict[str, Any]: Dictionary mapping decision IDs to executable DMN decision objects.
                           Returns empty dict if no DMN definitions found.
        """
        try:
            # Parse XML string into etree
            dmn_tree = etree.fromstring(xml_string.encode('utf-8'))
            
            # Add DMN XML to parser
            self.parser.add_dmn_xml(dmn_tree)
            
            # Return the parser's decisions dictionary
            decisions = self.parser.decisions
            self._last_decisions = decisions  # Store for get_decision
            return decisions
            
        except etree.XMLSyntaxError as e:
            # Handle XML parsing errors gracefully
            return {}
        except Exception as e:
            # Handle any other errors gracefully
            return {}
    
    def parse_file(self, file_path: str) -> Dict[str, Any]:
        """
        Parse DMN file and return executable SpiffWorkflow.dmn.specs.DmnDecision objects.
        
        Args:
            file_path: Path to DMN XML file
            
        Returns:
            Dict[str, Any]: Dictionary mapping decision IDs to executable DMN decision objects.
                           Returns empty dict if no DMN definitions found.
                                   
        Raises:
            FileNotFoundError: If file does not exist
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                xml_string = f.read()
            
            return self.parse(xml_string)
            
        except FileNotFoundError:
            raise FileNotFoundError(f"DMN file not found: {file_path}")
        except Exception as e:
            # Handle any other errors gracefully
            return {}
    
    def get_decision(self, decision_id: str) -> Any:
        """
        Get a specific decision by ID.
        
        Args:
            decision_id: ID of the decision to retrieve
            
        Returns:
            Any: The requested decision object
            
        Raises:
            KeyError: If decision_id is not found
        """
        if not hasattr(self, '_last_decisions'):
            raise KeyError(f"Decision ID '{decision_id}' not found")
        if decision_id not in self._last_decisions:
            raise KeyError(f"Decision ID '{decision_id}' not found")
        return self._last_decisions[decision_id] 