"""
AutoTel Schema Validation - Comprehensive input validation system
"""

import json
import yaml
from typing import Dict, Any, List, Optional, Union
from pathlib import Path
from dataclasses import dataclass
from enum import Enum

# LinkML imports
from linkml_runtime.utils.schemaview import SchemaView
from linkml_runtime.loaders import yaml_loader, json_loader
from linkml_runtime.dumpers import yaml_dumper, json_dumper

"""
SHACL Validation Utilities for DMN Data

Provides robust, reusable functions to validate DMN decision row data against SHACL shapes.
"""
from typing import Dict, Any
from rdflib import Graph, Namespace, URIRef, Literal
from rdflib.namespace import RDF, XSD
import pyshacl

AUTOTEL_NS = Namespace("http://autotel.ai/data/")


def dmn_row_to_rdf_graph(
    data: Dict[str, Any],
    shape_uri: str,
    instance_uri: str = None
) -> Graph:
    """
    Convert a DMN row (dict) to an RDF graph for SHACL validation.
    Args:
        data: Dict of field -> value (e.g., {"customerScore": 800})
        shape_uri: URI of the SHACL NodeShape to use as rdf:type
        instance_uri: Optional URI for the instance (default: AUTOTEL_NS['instance'])
    Returns:
        rdflib.Graph representing the data instance
    """
    g = Graph()
    subj = URIRef(instance_uri) if instance_uri else AUTOTEL_NS['instance']
    g.add((subj, RDF.type, URIRef(shape_uri)))
    for key, value in data.items():
        pred = AUTOTEL_NS[key]
        if isinstance(value, int):
            g.add((subj, pred, Literal(value, datatype=XSD.integer)))
        elif isinstance(value, float):
            g.add((subj, pred, Literal(value, datatype=XSD.decimal)))
        elif isinstance(value, bool):
            g.add((subj, pred, Literal(value, datatype=XSD.boolean)))
        else:
            g.add((subj, pred, Literal(str(value), datatype=XSD.string)))
    return g


def validate_dmn_row_with_shacl(
    data: Dict[str, Any],
    shape_uri: str,
    shacl_graph: Graph,
    instance_uri: str = None,
    raise_on_error: bool = True
) -> bool:
    """
    Validate a DMN row (dict) against a SHACL NodeShape.
    Args:
        data: Dict of field -> value
        shape_uri: URI of the SHACL NodeShape
        shacl_graph: rdflib.Graph containing SHACL shapes
        instance_uri: Optional URI for the instance
        raise_on_error: If True, raise AssertionError on failure; else return False
    Returns:
        True if valid, False if not (unless raise_on_error=True)
    """
    data_graph = dmn_row_to_rdf_graph(data, shape_uri, instance_uri)
    conforms, results_graph, results_text = pyshacl.validate(
        data_graph,
        shacl_graph=shacl_graph,
        inference='rdfs',
        debug=False
    )
    if not conforms:
        msg = f"SHACL validation failed for shape <{shape_uri}>:\n{results_text}"
        if raise_on_error:
            raise AssertionError(msg)
        return False
    return True

class ValidationLevel(Enum):
    """Validation levels for different contexts"""
    STRICT = "strict"
    NORMAL = "normal"
    LENIENT = "lenient"

@dataclass
class ValidationResult:
    """Result of a validation operation"""
    valid: bool
    errors: List[str] = None
    warnings: List[str] = None
    validated_data: Any = None
    
    def __post_init__(self):
        if self.errors is None:
            self.errors = []
        if self.warnings is None:
            self.warnings = []

class SchemaValidator:
    """Comprehensive schema validation system"""
    
    def __init__(self, validation_level: ValidationLevel = ValidationLevel.NORMAL):
        self.validation_level = validation_level
        self.schema_views: Dict[str, SchemaView] = {}
        self.validation_cache: Dict[str, ValidationResult] = {}
    
    def load_schema(self, schema_path: str, schema_name: str = None) -> None:
        """Load a LinkML schema for validation"""
        try:
            schema_view = SchemaView(schema_path)
            schema_name = schema_name or Path(schema_path).stem
            self.schema_views[schema_name] = schema_view
            print(f"✅ Loaded schema: {schema_name}")
        except Exception as e:
            raise ValueError(f"Failed to load schema {schema_path}: {e}")
    
    def validate_telemetry_schema(self, schema_path: str) -> ValidationResult:
        """
        Validate a LinkML telemetry schema
        
        Args:
            schema_path: Path to the LinkML schema file
            
        Returns:
            Validation result
        """
        try:
            with open(schema_path, 'r') as f:
                schema = yaml.safe_load(f)
            
            validation_result = ValidationResult(valid=True)
            
            # Basic structure validation
            required_keys = ['classes', 'enums']
            for key in required_keys:
                if key not in schema:
                    validation_result.valid = False
                    validation_result.errors.append(f"Missing required key: {key}")
            
            if not validation_result.valid:
                return validation_result
            
            # Validate classes
            classes = schema.get('classes', {})
            for class_name, class_def in classes.items():
                if not isinstance(class_def, dict):
                    validation_result.errors.append(f"Invalid class definition for {class_name}")
                    continue
                
                # Check for required class attributes
                if 'attributes' not in class_def:
                    validation_result.warnings.append(f"Class {class_name} has no attributes")
            
            # Validate enums
            enums = schema.get('enums', {})
            for enum_name, enum_def in enums.items():
                if not isinstance(enum_def, dict):
                    validation_result.errors.append(f"Invalid enum definition for {enum_name}")
                    continue
                
                # Check for permissible values
                if 'permissible_values' not in enum_def:
                    validation_result.warnings.append(f"Enum {enum_name} has no permissible values")
            
            # Update cache
            cache_key = f"telemetry_schema_{schema_path}"
            self.validation_cache[cache_key] = validation_result
            
            return validation_result
            
        except Exception as e:
            return ValidationResult(valid=False, errors=[f"Schema validation failed: {e}"])
    
    def validate_dspy_input(self, signature_name: str, input_data: Dict[str, Any]) -> Dict[str, Any]:
        """Validate DSPy signature input data"""
        try:
            # Get signature definition from registry
            from autotel.utils.dspy_services import dspy_registry
            signature_info = dspy_registry.get_signature_info(signature_name)
            
            if not signature_info:
                raise ValueError(f"Unknown DSPy signature: {signature_name}")
            
            validated_data = {}
            errors = []
            
            # Validate each input field
            for field_name, field_desc in signature_info.input_fields.items():
                if field_name in input_data:
                    # Basic type validation (could be enhanced with LinkML schemas)
                    validated_data[field_name] = input_data[field_name]
                else:
                    # Check if field is required
                    if not field_desc.get('optional', False):
                        errors.append(f"Required field '{field_name}' missing")
            
            if errors:
                raise ValueError(f"DSPy input validation failed: {'; '.join(errors)}")
            
            return validated_data
            
        except Exception as e:
            raise ValueError(f"DSPy input validation error: {e}")
    
    def validate_dspy_output(self, signature_name: str, output_data: Dict[str, Any]) -> Dict[str, Any]:
        """Validate DSPy signature output data"""
        try:
            # Get signature definition from registry
            from autotel.utils.dspy_services import dspy_registry
            signature_info = dspy_registry.get_signature_info(signature_name)
            
            if not signature_info:
                raise ValueError(f"Unknown DSPy signature: {signature_name}")
            
            validated_data = {}
            
            # Validate each output field
            for field_name, field_desc in signature_info.output_fields.items():
                if field_name in output_data:
                    validated_data[field_name] = output_data[field_name]
            
            return validated_data
            
        except Exception as e:
            raise ValueError(f"DSPy output validation error: {e}")
    
    def validate_dmn_input(self, decision_id: str, input_data: Dict[str, Any]) -> Dict[str, Any]:
        """Validate DMN decision input data"""
        try:
            # Basic validation - could be enhanced with DMN schema validation
            if not isinstance(input_data, dict):
                raise ValueError("DMN input data must be a dictionary")
            
            validated_data = {}
            
            # Validate each input field
            for key, value in input_data.items():
                if value is not None:
                    validated_data[key] = value
            
            return validated_data
            
        except Exception as e:
            raise ValueError(f"DMN input validation error: {e}")
    
    def validate_workflow_variables(self, variables: Dict[str, Any]) -> Dict[str, Any]:
        """Validate workflow variables"""
        try:
            if not isinstance(variables, dict):
                raise ValueError("Workflow variables must be a dictionary")
            
            validated_vars = {}
            
            # Validate each variable
            for key, value in variables.items():
                if not isinstance(key, str):
                    raise ValueError(f"Variable key must be string, got {type(key)}")
                
                # Basic type validation
                if isinstance(value, (str, int, float, bool, list, dict, type(None))):
                    validated_vars[key] = value
                else:
                    # Convert to string for complex objects
                    validated_vars[key] = str(value)
            
            return validated_vars
            
        except Exception as e:
            raise ValueError(f"Workflow variable validation error: {e}")
    
    def validate_bpmn_file(self, bpmn_path: str) -> ValidationResult:
        """Validate BPMN file structure and content"""
        try:
            import xml.etree.ElementTree as ET
            
            tree = ET.parse(bpmn_path)
            root = tree.getroot()
            
            validation_result = ValidationResult(valid=True)
            
            # Check for CDATA sections (forbidden)
            for elem in root.iter():
                if elem.text and '<![CDATA[' in elem.text:
                    validation_result.valid = False
                    validation_result.errors.append("CDATA sections are not allowed")
                    break
            
            # Check for required BPMN elements
            bpmn_ns = {'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'}
            processes = root.findall('.//bpmn:process', bpmn_ns)
            
            if not processes:
                validation_result.valid = False
                validation_result.errors.append("No BPMN processes found")
            
            # Check for DSPy signatures
            dspy_ns = {'dspy': 'http://autotel.ai/dspy'}
            signatures = root.findall('.//dspy:signature', dspy_ns)
            
            if signatures:
                validation_result.warnings.append(f"Found {len(signatures)} DSPy signatures")
            
            return validation_result
            
        except Exception as e:
            return ValidationResult(valid=False, errors=[f"BPMN validation failed: {e}"])
    
    def validate_dmn_file(self, dmn_path: str) -> ValidationResult:
        """Validate DMN file structure and content"""
        try:
            import xml.etree.ElementTree as ET
            
            tree = ET.parse(dmn_path)
            root = tree.getroot()
            
            validation_result = ValidationResult(valid=True)
            
            # Check for CDATA sections (forbidden)
            for elem in root.iter():
                if elem.text and '<![CDATA[' in elem.text:
                    validation_result.valid = False
                    validation_result.errors.append("CDATA sections are not allowed")
                    break
            
            # Check for required DMN elements
            dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
            decisions = root.findall('.//dmn:decision', dmn_ns)
            
            if not decisions:
                validation_result.valid = False
                validation_result.errors.append("No DMN decisions found")
            
            return validation_result
            
        except Exception as e:
            return ValidationResult(valid=False, errors=[f"DMN validation failed: {e}"])
    
    def validate_cli_input(self, command: str, args: Dict[str, Any]) -> ValidationResult:
        """Validate CLI command input"""
        try:
            validation_result = ValidationResult(valid=True)
            
            # Validate command name
            if not isinstance(command, str) or not command.strip():
                validation_result.valid = False
                validation_result.errors.append("Invalid command name")
            
            # Validate arguments
            if not isinstance(args, dict):
                validation_result.valid = False
                validation_result.errors.append("Arguments must be a dictionary")
            else:
                for key, value in args.items():
                    if not isinstance(key, str):
                        validation_result.errors.append(f"Argument key must be string: {key}")
            
            return validation_result
            
        except Exception as e:
            return ValidationResult(valid=False, errors=[f"CLI input validation failed: {e}"])
    
    def get_validation_summary(self) -> Dict[str, Any]:
        """Get a summary of validation results"""
        return {
            "loaded_schemas": list(self.schema_views.keys()),
            "cached_results": len(self.validation_cache),
            "validation_level": self.validation_level.value
        }

# Global validator instance
schema_validator = SchemaValidator()

def validate_telemetry_schema(schema_path: str) -> ValidationResult:
    """Validate a telemetry schema"""
    return schema_validator.validate_telemetry_schema(schema_path) 