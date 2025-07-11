"""
AutoTel Schema Validation
"""

import json
import yaml
from typing import Dict, Any, List, Optional
from pathlib import Path

def validate_telemetry_schema(schema_path: str) -> Dict[str, Any]:
    """
    Validate a LinkML telemetry schema
    
    Args:
        schema_path: Path to the LinkML schema file
        
    Returns:
        Validation result dictionary
    """
    try:
        with open(schema_path, 'r') as f:
            schema = yaml.safe_load(f)
        
        validation_result = {
            "valid": True,
            "errors": [],
            "warnings": [],
            "schema_info": {}
        }
        
        # Basic structure validation
        required_keys = ['classes', 'enums']
        for key in required_keys:
            if key not in schema:
                validation_result["valid"] = False
                validation_result["errors"].append(f"Missing required key: {key}")
        
        if not validation_result["valid"]:
            return validation_result
        
        # Validate classes
        classes = schema.get('classes', {})
        for class_name, class_def in classes.items():
            if not isinstance(class_def, dict):
                validation_result["errors"].append(f"Invalid class definition for {class_name}")
                continue
            
            # Check for required class attributes
            if 'attributes' not in class_def:
                validation_result["warnings"].append(f"Class {class_name} has no attributes")
            
            # Validate attributes
            attributes = class_def.get('attributes', {})
            for attr_name, attr_def in attributes.items():
                if not isinstance(attr_def, dict):
                    validation_result["errors"].append(f"Invalid attribute definition for {class_name}.{attr_name}")
                    continue
                
                # Check for required attribute fields
                if 'range' not in attr_def:
                    validation_result["warnings"].append(f"Attribute {class_name}.{attr_name} has no range")
        
        # Validate enums
        enums = schema.get('enums', {})
        for enum_name, enum_def in enums.items():
            if not isinstance(enum_def, dict):
                validation_result["errors"].append(f"Invalid enum definition for {enum_name}")
                continue
            
            # Check for permissible values
            if 'permissible_values' not in enum_def:
                validation_result["warnings"].append(f"Enum {enum_name} has no permissible values")
        
        # Check for telemetry-specific classes
        telemetry_classes = ['Span', 'Metric', 'LinkMLOperation', 'ValidationResult']
        found_telemetry_classes = []
        for class_name in telemetry_classes:
            if class_name in classes:
                found_telemetry_classes.append(class_name)
        
        if not found_telemetry_classes:
            validation_result["warnings"].append("No telemetry-specific classes found")
        
        # Update validation status
        if validation_result["errors"]:
            validation_result["valid"] = False
        
        # Add schema information
        validation_result["schema_info"] = {
            "total_classes": len(classes),
            "total_enums": len(enums),
            "telemetry_classes": found_telemetry_classes,
            "schema_version": schema.get('version', 'unknown')
        }
        
        return validation_result
        
    except Exception as e:
        return {
            "valid": False,
            "errors": [f"Schema validation failed: {str(e)}"],
            "warnings": [],
            "schema_info": {}
        } 