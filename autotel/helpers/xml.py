"""
AutoTel XML Helper Utilities

Provides XML parsing and extraction utilities for BPMN, DMN, SHACL, OWL, and DSPy processors.
Happy path only, with type hints and docstrings.
"""

from typing import Any, Optional, List
from pathlib import Path
import xml.etree.ElementTree as ET


def parse_xml_file(file_path: str) -> ET.Element:
    """
    Parse an XML file and return the root element.
    Args:
        file_path: Path to the XML file
    Returns:
        Root XML element
    """
    tree = ET.parse(file_path)
    return tree.getroot()


def parse_xml_string(xml_string: str) -> ET.Element:
    """
    Parse an XML string and return the root element.
    Args:
        xml_string: XML content as string
    Returns:
        Root XML element
    """
    return ET.fromstring(xml_string)


def get_elements_by_tag(root: ET.Element, tag: str) -> List[ET.Element]:
    """
    Get all elements with a given tag under the root.
    Args:
        root: Root XML element
        tag: Tag name to search for
    Returns:
        List of matching elements
    """
    return root.findall(f'.//{tag}')


def get_element_text(element: ET.Element) -> Optional[str]:
    """
    Get the text content of an XML element.
    Args:
        element: XML element
    Returns:
        Text content or None
    """
    return element.text


def get_attribute(element: ET.Element, attr: str) -> Optional[str]:
    """
    Get the value of an attribute from an XML element.
    Args:
        element: XML element
        attr: Attribute name
    Returns:
        Attribute value or None
    """
    return element.attrib.get(attr)


def pretty_print_xml(element: ET.Element) -> str:
    """
    Return a pretty-printed XML string for the given element.
    Args:
        element: XML element
    Returns:
        Pretty-printed XML string
    """
    return ET.tostring(element, encoding='unicode', method='xml') 