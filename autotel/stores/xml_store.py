"""
AutoTel XML Store

Store implementation for XML file operations using ElementTree.
"""

import xml.etree.ElementTree as ET
from typing import Any, Dict
from pathlib import Path

from autotel.stores.base import BaseStore, StoreResult
from autotel.stores.meta import store_metadata


@store_metadata(
    name="xml_store",
    version="1.0.0",
    capabilities=["xml_parsing", "xml_serialization", "tree_structure"],
    supported_formats=["xml"],
    author="AutoTel",
    enterprise_ready=True,
    performance_characteristics={
        "load_time": "O(n) - linear with XML size",
        "memory_usage": "Moderate - full tree in memory",
        "validation": "Basic XML well-formedness"
    }
)
class XMLStore(BaseStore):
    """
    Store implementation for XML file operations.
    
    Uses ElementTree for XML parsing and serialization. Supports both
    loading XML files into ElementTree objects and saving data as XML.
    
    Example:
        config = StoreConfig(name="config", path="config.xml")
        store = XMLStore(config)
        result = store.load()  # Returns ElementTree.Element
    """

    def _load_impl(self) -> StoreResult:
        """
        Load XML data from file.
        
        Returns:
            StoreResult with ElementTree.Element as data
        """
        try:
            tree = ET.parse(self.config.path)
            root = tree.getroot()
            
            # Add metadata about the XML structure
            metadata = {
                "root_tag": root.tag,
                "child_count": len(root),
                "file_size": Path(self.config.path).stat().st_size if Path(self.config.path).exists() else 0
            }
            
            return StoreResult.success_result(root, metadata)
            
        except ET.ParseError as e:
            return StoreResult.error_result(
                f"XML parsing error: {str(e)}",
                {"error_type": "ParseError", "line": getattr(e, 'line', 'unknown')}
            )
        except FileNotFoundError:
            return StoreResult.error_result(
                f"XML file not found: {self.config.path}",
                {"error_type": "FileNotFoundError"}
            )
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to load XML: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def _save_impl(self, data: Any) -> StoreResult:
        """
        Save data as XML file.
        
        Args:
            data: ElementTree.Element or dict/list to save as XML
            
        Returns:
            StoreResult with save operation result
        """
        try:
            # Handle different input types
            if isinstance(data, ET.Element):
                root = data
            elif isinstance(data, dict):
                root = self._dict_to_element(data, "root")
            elif isinstance(data, list):
                root = ET.Element("root")
                for i, item in enumerate(data):
                    child = self._dict_to_element(item, f"item_{i}")
                    root.append(child)
            else:
                return StoreResult.error_result(
                    f"Unsupported data type for XML: {type(data).__name__}",
                    {"error_type": "TypeError"}
                )
            
            # Create tree and save
            tree = ET.ElementTree(root)
            
            # Get formatting settings
            indent = self.get_setting("indent", 2)
            encoding = self.get_setting("encoding", "utf-8")
            
            # Ensure directory exists
            Path(self.config.path).parent.mkdir(parents=True, exist_ok=True)
            
            # Write with pretty formatting if indent is specified
            if indent > 0:
                self._indent(root, indent)
            
            tree.write(
                self.config.path,
                encoding=encoding,
                xml_declaration=True
            )
            
            metadata = {
                "root_tag": root.tag,
                "child_count": len(root),
                "file_size": Path(self.config.path).stat().st_size
            }
            
            return StoreResult.success_result(data, metadata)
            
        except Exception as e:
            return StoreResult.error_result(
                f"Failed to save XML: {str(e)}",
                {"error_type": type(e).__name__}
            )

    def _dict_to_element(self, data: Dict, tag: str) -> ET.Element:
        """Convert dictionary to ElementTree Element."""
        element = ET.Element(tag)
        
        for key, value in data.items():
            if isinstance(value, dict):
                child = self._dict_to_element(value, key)
                element.append(child)
            elif isinstance(value, list):
                for i, item in enumerate(value):
                    if isinstance(item, dict):
                        child = self._dict_to_element(item, f"{key}_{i}")
                    else:
                        child = ET.SubElement(element, f"{key}_{i}")
                        child.text = str(item)
                    element.append(child)
            else:
                child = ET.SubElement(element, key)
                child.text = str(value)
        
        return element

    def _indent(self, elem: ET.Element, level: int = 0) -> None:
        """Add indentation to XML element for pretty printing."""
        i = "\n" + level * "  "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "  "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for child in elem:
                self._indent(child, level + 1)
            if not child.tail or not child.tail.strip():
                child.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i 