#!/usr/bin/env python3
"""
Generic SHACL Processing Tests

Tests the SHACL processor with any SHACL file in a generic, non-hardcoded way.
"""

import pytest
import sys
import json
from pathlib import Path
from typing import Dict, Any, List

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.factory.processors.shacl_processor import SHACLProcessor

def find_shacl_files() -> List[Path]:
    """Find all SHACL files in the project"""
    shacl_files = []
    
    # Common locations for SHACL files
    search_paths = [
        "ontologies",
        "schemas", 
        "bpmn",
        "examples"
    ]
    
    for search_path in search_paths:
        if Path(search_path).exists():
            # Look for .shacl files
            shacl_files.extend(Path(search_path).rglob("*.shacl"))
            # Look for .ttl files (Turtle format)
            shacl_files.extend(Path(search_path).rglob("*.ttl"))
            # Look for .rdf files
            shacl_files.extend(Path(search_path).rglob("*.rdf"))
    
    return shacl_files

def extract_namespace_from_shape_id(shape_id: str) -> str:
    """Extract namespace from shape ID"""
    if ":" in shape_id:
        return shape_id.split(":")[0]
    return "default"

def generate_sample_data_from_shapes(node_shapes: List[Dict[str, Any]]) -> Dict[str, Any]:
    """Generate sample data based on the actual shapes found"""
    sample_data = {}
    
    for shape in node_shapes:
        shape_id = shape.get("shape_id", "")
        target_class = shape.get("target_class", "")
        
        if not target_class:
            continue
            
        # Extract class name from target class URI
        class_name = target_class.split(":")[-1] if ":" in target_class else target_class
        
        # Generate minimal sample data for this class
        sample_data[class_name] = {}
        
        # Add properties based on shape properties
        properties = shape.get("properties", [])
        for prop in properties:
            prop_path = prop.get("path", "")
            if prop_path:
                # Extract property name
                prop_name = prop_path.split(":")[-1] if ":" in prop_path else prop_path
                
                # Generate appropriate sample value based on constraints
                constraints = prop.get("constraints", {})
                sample_value = generate_sample_value_from_constraints(constraints)
                sample_data[class_name][prop_name] = sample_value
    
    return sample_data

def generate_sample_value_from_constraints(constraints: Dict[str, Any]) -> Any:
    """Generate appropriate sample value based on constraints"""
    datatype = constraints.get("datatype", "")
    
    if "decimal" in datatype or "float" in datatype or "double" in datatype:
        return 1.0
    elif "integer" in datatype or "int" in datatype:
        return 1
    elif "string" in datatype or "text" in datatype:
        return "sample_value"
    elif "boolean" in datatype or "bool" in datatype:
        return True
    elif "date" in datatype or "time" in datatype:
        return "2024-01-01"
    else:
        # Default to string for unknown types
        return "sample_value"

@pytest.mark.integration
def test_shacl_file_parsing():
    """Test parsing any SHACL file"""
    processor = SHACLProcessor()
    
    # Find all SHACL files
    shacl_files = find_shacl_files()
    
    if not shacl_files:
        pytest.skip("No SHACL files found in the project")
    
    # Test the first available SHACL file
    shacl_file = shacl_files[0]
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(shacl_file))
        
        # Verify the result structure
        assert result is not None
        assert isinstance(result, dict)
        
        # Check for expected keys
        assert "node_shapes" in result
        assert "property_shapes" in result
        assert "constraints" in result
        assert "metadata" in result
        
        # Verify we have node shapes
        node_shapes = result["node_shapes"]
        assert len(node_shapes) > 0
        
        # Extract namespaces dynamically
        shape_ids = [shape["shape_id"] for shape in node_shapes]
        namespaces = set()
        for shape_id in shape_ids:
            namespace = extract_namespace_from_shape_id(shape_id)
            namespaces.add(namespace)
        
        print(f"✅ Successfully parsed SHACL file: {shacl_file}")
        print(f"   📊 Node shapes: {len(node_shapes)}")
        print(f"   🏷️  Namespaces: {list(namespaces)}")
        print(f"   🎯 Shape IDs: {shape_ids[:5]}...")
        
    except Exception as e:
        pytest.fail(f"Failed to parse SHACL file {shacl_file}: {e}")

@pytest.mark.integration
def test_shacl_validation_with_generated_data():
    """Test SHACL validation with dynamically generated data"""
    processor = SHACLProcessor()
    
    # Find all SHACL files
    shacl_files = find_shacl_files()
    
    if not shacl_files:
        pytest.skip("No SHACL files found in the project")
    
    # Test the first available SHACL file
    shacl_file = shacl_files[0]
    
    try:
        # Parse the SHACL file
        shacl_result = processor.parse_file(str(shacl_file))
        
        # Generate sample data based on actual shapes
        sample_data = generate_sample_data_from_shapes(shacl_result["node_shapes"])
        
        if not sample_data:
            pytest.skip(f"No valid shapes found in {shacl_file} for data generation")
        
        # Validate the data
        validation_result = processor.validate_data(sample_data, shacl_result["validation_rules"])
        
        # Check validation result
        assert validation_result is not None
        assert "valid" in validation_result
        
        print(f"✅ Successfully validated generated data for: {shacl_file}")
        print(f"   📊 Valid: {validation_result['valid']}")
        print(f"   📋 Validation result keys: {list(validation_result.keys())}")
        print(f"   🎯 Sample data classes: {list(sample_data.keys())}")
        
        # Check for errors if they exist
        if "errors" in validation_result:
            print(f"   ❌ Errors: {len(validation_result['errors'])}")
        else:
            print(f"   ✅ No errors field (clean validation)")
        
    except Exception as e:
        pytest.fail(f"Failed to validate data for {shacl_file}: {e}")

@pytest.mark.integration
def test_shacl_constraint_extraction():
    """Test constraint extraction from any SHACL file"""
    processor = SHACLProcessor()
    
    # Find all SHACL files
    shacl_files = find_shacl_files()
    
    if not shacl_files:
        pytest.skip("No SHACL files found in the project")
    
    # Test the first available SHACL file
    shacl_file = shacl_files[0]
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(shacl_file))
        
        # Find shapes with properties
        shapes_with_properties = []
        for shape in result["node_shapes"]:
            properties = shape.get("properties", [])
            if properties:
                shapes_with_properties.append(shape)
        
        if not shapes_with_properties:
            pytest.skip(f"No shapes with properties found in {shacl_file}")
        
        # Test the first shape with properties
        test_shape = shapes_with_properties[0]
        shape_id = test_shape.get("shape_id", "unknown")
        properties = test_shape.get("properties", [])
        
        # Verify we have properties
        assert len(properties) > 0, f"No properties found in shape {shape_id}"
        
        # Check for constraint types
        constraint_types = set()
        for prop in properties:
            constraints = prop.get("constraints", {})
            constraint_types.update(constraints.keys())
        
        print(f"✅ Successfully extracted constraints from: {shacl_file}")
        print(f"   📊 Test shape: {shape_id}")
        print(f"   🔧 Properties: {len(properties)}")
        print(f"   🎯 Constraint types: {list(constraint_types)}")
        
        # Show some example constraints
        for i, prop in enumerate(properties[:3]):  # Show first 3 properties
            prop_path = prop.get("path", "unknown")
            constraints = prop.get("constraints", {})
            print(f"   📋 Property {i+1}: {prop_path} -> {constraints}")
        
    except Exception as e:
        pytest.fail(f"Failed to extract constraints from {shacl_file}: {e}")

@pytest.mark.integration
def test_shacl_metadata_extraction():
    """Test metadata extraction from SHACL files"""
    processor = SHACLProcessor()
    
    # Find all SHACL files
    shacl_files = find_shacl_files()
    
    if not shacl_files:
        pytest.skip("No SHACL files found in the project")
    
    # Test the first available SHACL file
    shacl_file = shacl_files[0]
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(shacl_file))
        
        # Check metadata
        metadata = result.get("metadata")
        
        print(f"✅ Successfully extracted metadata from: {shacl_file}")
        
        if metadata is not None:
            print(f"   📊 Metadata type: {type(metadata).__name__}")
            print(f"   📋 SHACL triples: {getattr(metadata, 'shacl_triples', 'N/A')}")
            print(f"   📋 Node shapes: {getattr(metadata, 'node_shapes', 'N/A')}")
            print(f"   📋 Property shapes: {getattr(metadata, 'property_shapes', 'N/A')}")
            print(f"   📋 Constraint types: {getattr(metadata, 'constraint_types', 'N/A')}")
        else:
            print(f"   ⚠️  No metadata found")
        
    except Exception as e:
        pytest.fail(f"Failed to extract metadata from {shacl_file}: {e}")

if __name__ == "__main__":
    pytest.main([__file__, "-v"]) 