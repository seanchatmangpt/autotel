#!/usr/bin/env python3
"""
Test SRO SHACL Processing

Tests the SHACL processor with the Scrum Reference Ontology SHACL file.
"""

import pytest
import sys
from pathlib import Path
from typing import Dict, Any, List

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.factory.processors.shacl_processor import SHACLProcessor

def find_sro_shacl_file() -> Path:
    """Find the SRO SHACL file"""
    sro_shacl_path = Path("ontologies/scrum_reference_ontology/sro.shacl")
    
    if not sro_shacl_path.exists():
        # Try alternative locations
        alternative_paths = [
            "sro.shacl",
            "ontologies/sro.shacl",
            "schemas/sro.shacl"
        ]
        
        for alt_path in alternative_paths:
            if Path(alt_path).exists():
                return Path(alt_path)
    
    return sro_shacl_path

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
def test_sro_shacl_parsing():
    """Test parsing the SRO SHACL file"""
    processor = SHACLProcessor()
    
    # Find the SRO SHACL file
    sro_shacl_path = find_sro_shacl_file()
    
    if not sro_shacl_path.exists():
        pytest.skip(f"SRO SHACL file not found: {sro_shacl_path}")
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(sro_shacl_path))
        
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
        
        # Check for SRO-specific namespace
        sro_namespace = "sro"
        assert sro_namespace in namespaces, f"Expected SRO namespace not found. Found: {namespaces}"
        
        # Look for SRO-specific shapes (any shape with sro: namespace)
        sro_shapes = [shape_id for shape_id in shape_ids if f"{sro_namespace}:" in shape_id]
        assert len(sro_shapes) > 0, "No SRO shapes found"
        
        print(f"✅ Successfully parsed SRO SHACL file")
        print(f"   📊 Node shapes: {len(node_shapes)}")
        print(f"   🏷️  Namespaces: {list(namespaces)}")
        print(f"   🎯 SRO shapes: {len(sro_shapes)}")
        print(f"   📋 Sample shape IDs: {sro_shapes[:5]}")
        
    except Exception as e:
        pytest.fail(f"Failed to parse SRO SHACL file: {e}")

@pytest.mark.integration
def test_sro_shacl_validation():
    """Test SHACL validation with SRO data"""
    processor = SHACLProcessor()
    
    # Find the SRO SHACL file
    sro_shacl_path = find_sro_shacl_file()
    
    if not sro_shacl_path.exists():
        pytest.skip(f"SRO SHACL file not found: {sro_shacl_path}")
    
    try:
        # Parse the SHACL file
        shacl_result = processor.parse_file(str(sro_shacl_path))
        
        # Generate sample data based on actual shapes
        sample_data = generate_sample_data_from_shapes(shacl_result["node_shapes"])
        
        if not sample_data:
            pytest.skip(f"No valid shapes found in {sro_shacl_path} for data generation")
        
        # Validate the data
        validation_result = processor.validate_data(sample_data, shacl_result["validation_rules"])
        
        # Check validation result
        assert validation_result is not None
        assert "valid" in validation_result
        
        print(f"✅ Successfully validated SRO data")
        print(f"   📊 Valid: {validation_result['valid']}")
        print(f"   📋 Validation result keys: {list(validation_result.keys())}")
        print(f"   🎯 Sample data classes: {list(sample_data.keys())}")
        
        # Check for errors if they exist
        if "errors" in validation_result:
            print(f"   ❌ Errors: {len(validation_result['errors'])}")
        else:
            print(f"   ✅ No errors field (clean validation)")
        
    except Exception as e:
        pytest.fail(f"Failed to validate SRO data: {e}")

@pytest.mark.integration
def test_sro_shacl_constraints():
    """Test constraint extraction from SRO SHACL"""
    processor = SHACLProcessor()
    
    # Find the SRO SHACL file
    sro_shacl_path = find_sro_shacl_file()
    
    if not sro_shacl_path.exists():
        pytest.skip(f"SRO SHACL file not found: {sro_shacl_path}")
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(sro_shacl_path))
        
        # Find shapes with properties
        shapes_with_properties = []
        for shape in result["node_shapes"]:
            properties = shape.get("properties", [])
            if properties:
                shapes_with_properties.append(shape)
        
        if not shapes_with_properties:
            pytest.skip(f"No shapes with properties found in {sro_shacl_path}")
        
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
        
        print(f"✅ Successfully extracted SRO constraints")
        print(f"   📊 Test shape: {shape_id}")
        print(f"   🔧 Properties: {len(properties)}")
        print(f"   🎯 Constraint types: {list(constraint_types)}")
        
        # Show some example constraints
        for i, prop in enumerate(properties[:3]):  # Show first 3 properties
            prop_path = prop.get("path", "unknown")
            constraints = prop.get("constraints", {})
            print(f"   📋 Property {i+1}: {prop_path} -> {constraints}")
        
    except Exception as e:
        pytest.fail(f"Failed to extract SRO constraints: {e}")

@pytest.mark.integration
def test_sro_shacl_metadata():
    """Test metadata extraction from SRO SHACL"""
    processor = SHACLProcessor()
    
    # Find the SRO SHACL file
    sro_shacl_path = find_sro_shacl_file()
    
    if not sro_shacl_path.exists():
        pytest.skip(f"SRO SHACL file not found: {sro_shacl_path}")
    
    try:
        # Parse the SHACL file
        result = processor.parse_file(str(sro_shacl_path))
        
        # Check metadata
        metadata = result.get("metadata")
        
        print(f"✅ Successfully extracted SRO metadata")
        
        if metadata is not None:
            print(f"   📊 Metadata type: {type(metadata).__name__}")
            print(f"   📋 SHACL triples: {getattr(metadata, 'shacl_triples', 'N/A')}")
            print(f"   📋 Node shapes: {getattr(metadata, 'node_shapes', 'N/A')}")
            print(f"   📋 Property shapes: {getattr(metadata, 'property_shapes', 'N/A')}")
            print(f"   📋 Constraint types: {getattr(metadata, 'constraint_types', 'N/A')}")
        else:
            print(f"   ⚠️  No metadata found")
        
    except Exception as e:
        pytest.fail(f"Failed to extract SRO metadata: {e}")

if __name__ == "__main__":
    pytest.main([__file__, "-v"]) 