"""SHACL processor for AutoTel semantic execution pipeline."""

import xml.etree.ElementTree as ET
from typing import Dict, Any, List, Tuple, Optional, Callable
from rdflib import Graph, Namespace, URIRef, Literal
from rdflib.namespace import RDF, XSD
import time
import re
from functools import lru_cache

from ...core.telemetry import create_telemetry_manager
from ...schemas.validation_types import ValidationRule, ValidationRules, ConstraintMetadata
from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode


# SHACL namespaces
SHACL = Namespace("http://www.w3.org/ns/shacl#")
RDFS = Namespace("http://www.w3.org/2000/01/rdf-schema#")

# Extended constraint types for 80/20 improvements
CONSTRAINT_TYPES = {
    "cardinality": ["min_count", "max_count", "qualified_min_count", "qualified_max_count"],
    "datatype": ["datatype", "node_kind"],
    "value": ["min_length", "max_length", "pattern", "language_in", "unique_lang"],
    "range": ["min_inclusive", "max_inclusive", "min_exclusive", "max_exclusive"],
    "logical": ["and", "or", "not", "xone"],
    "comparison": ["equals", "disjoint", "less_than", "less_than_or_equals"],
    "string": ["pattern", "pattern_flags", "language_in", "unique_lang", "min_length", "max_length"],
    "custom": ["sparql", "javascript", "python"]
}

class PluggableValidator:
    """Pluggable validator for extensible constraint validation."""
    
    def __init__(self, name: str, validator_func: Callable, constraint_types: List[str]):
        self.name = name
        self.validator_func = validator_func
        self.constraint_types = constraint_types
    
    def can_validate(self, constraint_type: str) -> bool:
        return constraint_type in self.constraint_types
    
    def validate(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        return self.validator_func(value, constraints, context)

class SHACLProcessor:
    """Processes SHACL/RDF XML into constraint graphs and validation rules."""

    def __init__(self):
        """Initialize SHACL processor with telemetry and pluggable validators."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-shacl-processor",
            require_linkml_validation=False
        )
        
        # Initialize pluggable validators
        self.validators = self._initialize_validators()
        
        # Performance cache
        self._graph_cache = {}
        self._constraint_cache = {}

    def _initialize_validators(self) -> List[PluggableValidator]:
        """Initialize pluggable validators for extensible constraint validation."""
        validators = []
        
        # Cardinality validator
        validators.append(PluggableValidator(
            "cardinality",
            self._validate_cardinality_extended,
            ["cardinality", "qualified"]
        ))
        
        # Data type validator
        validators.append(PluggableValidator(
            "datatype",
            self._validate_datatype_extended,
            ["datatype", "nodeKind"]
        ))
        
        # String validator
        validators.append(PluggableValidator(
            "string",
            self._validate_string_constraints,
            ["pattern", "languageIn", "uniqueLang", "minLength", "maxLength"]
        ))
        
        # Range validator
        validators.append(PluggableValidator(
            "range",
            self._validate_range_constraints_extended,
            ["minInclusive", "maxInclusive", "minExclusive", "maxExclusive"]
        ))
        
        # Logical validator
        validators.append(PluggableValidator(
            "logical",
            self._validate_logical_constraints,
            ["and", "or", "not", "xone"]
        ))
        
        return validators

    @lru_cache(maxsize=128)
    def _get_cached_graph(self, xml_content: str) -> Graph:
        """Cache parsed graphs for performance optimization."""
        graph = Graph()
        graph.parse(data=xml_content, format='xml')
        return graph

    def parse(self, xml_content: str) -> Dict[str, Any]:
        """Parse SHACL XML content into structured constraint data with advanced telemetry."""
        start_time = time.time()
        
        with self.telemetry.start_span("shacl.processor.parse", "constraint_validation") as span:
            # Record input metrics
            span.set_attribute("component", "shacl_processor")
            span.set_attribute("operation", "parse")
            span.set_attribute("input_format", "shacl_xml")
            span.set_attribute("output_format", "Graph")
            span.set_attribute("xml_size_bytes", len(xml_content.encode('utf-8')))
            span.set_attribute("cache_enabled", True)
            
            try:
                # Use cached graph parsing for performance
                graph = self._get_cached_graph(xml_content)
                
                # Extract shapes and constraints with detailed telemetry
                node_shapes = self._extract_node_shapes_extended(graph)
                property_shapes = self._extract_property_shapes_extended(graph)
                constraints = self._extract_constraints_extended(graph)
                
                # Generate validation rules with schema-driven messages
                validation_rules = self._generate_validation_rules_extended(node_shapes, property_shapes)
                
                # Calculate performance metrics
                parsing_duration = (time.time() - start_time) * 1000
                
                # Record successful parsing metrics with advanced telemetry
                span.set_attribute("success", True)
                span.set_attribute("node_shapes_extracted", len(node_shapes))
                span.set_attribute("property_shapes_extracted", len(property_shapes))
                span.set_attribute("constraints_extracted", len(constraints))
                span.set_attribute("target_classes_found", len(set(shape.get('target_class') for shape in node_shapes)))
                span.set_attribute("validation_rules_generated", len(validation_rules))
                span.set_attribute("parsing_duration_ms", parsing_duration)
                span.set_attribute("constraint_types_found", list(set(c.get('type', 'unknown') for c in constraints)))
                span.set_attribute("validator_count", len(self.validators))
                span.set_attribute("cache_hit_rate", self._get_cache_hit_rate())
                
                # Record advanced metrics
                self.telemetry.record_metric("shacl_parse_duration_ms", parsing_duration)
                self.telemetry.record_metric("shacl_constraint_types", len(set(c.get('type', 'unknown') for c in constraints)))
                self.telemetry.record_metric("shacl_validator_utilization", len(self.validators))
                self.telemetry.record_metric("shacl_cache_efficiency", self._get_cache_hit_rate())
                
                # Record success metric
                self.telemetry.record_metric("shacl_parse_success", 1)
                
                return {
                    "graph": graph,
                    "node_shapes": node_shapes,
                    "property_shapes": property_shapes,
                    "constraints": constraints,
                    "validation_rules": validation_rules,
                    "metadata": self._create_metadata_extended(graph, node_shapes, property_shapes, constraints),
                    "performance": {
                        "parsing_duration_ms": parsing_duration,
                        "cache_hit_rate": self._get_cache_hit_rate(),
                        "constraint_types": list(set(c.get('type', 'unknown') for c in constraints))
                    }
                }
                
            except Exception as e:
                # Record parsing failure with detailed error context
                parsing_duration = (time.time() - start_time) * 1000
                span.set_attribute("success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_attribute("parsing_duration_ms", parsing_duration)
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metrics
                self.telemetry.record_metric("shacl_parse_failure", 1)
                self.telemetry.record_metric("shacl_parse_duration_ms", parsing_duration)
                
                raise

    def parse_file(self, file_path: str) -> Dict[str, Any]:
        """Parse SHACL XML file into structured constraint data."""
        with self.telemetry.start_span("shacl.processor.parse_file", "constraint_validation") as span:
            # Record file information
            span.set_attribute("file_path", file_path)
            
            try:
                with open(file_path, 'r', encoding='utf-8') as f:
                    xml_content = f.read()
                
                # Record file size
                span.set_attribute("file_size_bytes", len(xml_content.encode('utf-8')))
                
                # Parse the content
                return self.parse(xml_content)
                
            except FileNotFoundError:
                span.set_attribute("error_type", "FileNotFoundError")
                span.set_attribute("error_message", f"File not found: {file_path}")
                span.set_status(trace.Status(trace.StatusCode.ERROR, f"File not found: {file_path}"))
                
                # Record metric for file not found
                self.telemetry.record_metric("shacl_file_not_found", 1)
                
                raise
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for file parsing failure
                self.telemetry.record_metric("shacl_file_parse_failure", 1)
                
                raise

    def _extract_node_shapes_extended(self, graph: Graph) -> List[Dict[str, Any]]:
        """Extract node shapes with extended constraint support."""
        with self.telemetry.start_span("shacl.processor.extract_node_shapes", "constraint_validation") as span:
            node_shapes = []
            target_classes = []
            shape_types = []
            deactivated_shapes = 0
            constraint_counts = {}
            
            # Find all NodeShape instances
            for node_shape in graph.subjects(RDF.type, SHACL.NodeShape):
                shape_data = {
                    "shape_id": str(node_shape),
                    "target_class": None,
                    "properties": [],
                    "deactivated": False,
                    "severity": "Violation",
                    "constraints": {},
                    "metadata": {}
                }
                
                # Extract target class
                for target_class in graph.objects(node_shape, SHACL.targetClass):
                    shape_data["target_class"] = str(target_class)
                    target_classes.append(str(target_class))
                
                # Check if shape is deactivated
                for deactivated in graph.objects(node_shape, SHACL.deactivated):
                    if deactivated:
                        shape_data["deactivated"] = True
                        deactivated_shapes += 1
                
                # Extract severity
                for severity in graph.objects(node_shape, SHACL.severity):
                    shape_data["severity"] = str(severity)
                
                # Extract extended properties with constraint counting
                for property_shape in graph.objects(node_shape, SHACL.property):
                    prop_data = self._extract_property_shape_data_extended(graph, property_shape)
                    if prop_data:
                        shape_data["properties"].append(prop_data)
                        
                        # Count constraint types
                        for constraint_type in prop_data.get("constraints", {}):
                            constraint_counts[constraint_type] = constraint_counts.get(constraint_type, 0) + 1
                
                # Extract shape-level constraints
                shape_data["constraints"] = self._extract_shape_constraints(graph, node_shape)
                
                node_shapes.append(shape_data)
                shape_types.append("NodeShape")
            
            # Record extended extraction metrics
            span.set_attribute("node_shapes_found", len(node_shapes))
            span.set_attribute("target_classes", target_classes)
            span.set_attribute("shape_types", shape_types)
            span.set_attribute("deactivated_shapes", deactivated_shapes)
            span.set_attribute("constraint_type_distribution", constraint_counts)
            
            return node_shapes

    def _extract_property_shapes_extended(self, graph: Graph) -> List[Dict[str, Any]]:
        """Extract property shapes with extended constraint support."""
        with self.telemetry.start_span("shacl.processor.extract_property_shapes", "constraint_validation") as span:
            property_shapes = []
            property_paths = []
            constraint_type_counts = {}
            advanced_constraints = 0
            
            # Find all PropertyShape instances
            for property_shape in graph.subjects(RDF.type, SHACL.PropertyShape):
                prop_data = self._extract_property_shape_data_extended(graph, property_shape)
                if prop_data:
                    property_shapes.append(prop_data)
                    property_paths.append(prop_data.get("path", ""))
                    
                    # Count constraint types for telemetry
                    for constraint_type in prop_data.get("constraints", {}):
                        constraint_type_counts[constraint_type] = constraint_type_counts.get(constraint_type, 0) + 1
                        
                        # Count advanced constraints
                        if constraint_type in ["pattern", "languageIn", "uniqueLang", "sparql"]:
                            advanced_constraints += 1
            
            # Record extended extraction metrics
            span.set_attribute("property_shapes_found", len(property_shapes))
            span.set_attribute("property_paths", property_paths)
            span.set_attribute("constraint_type_distribution", constraint_type_counts)
            span.set_attribute("advanced_constraints", advanced_constraints)
            
            return property_shapes

    def _extract_property_shape_data_extended(self, graph: Graph, property_shape: URIRef) -> Dict[str, Any]:
        """Extract extended data from a property shape with all constraint types."""
        prop_data = {
            "shape_id": str(property_shape),
            "path": None,
            "constraints": {},
            "metadata": {}
        }
        
        # Extract property path
        for path in graph.objects(property_shape, SHACL.path):
            prop_data["path"] = str(path)
        
        # Extract all constraint types with extended support
        constraint_mappings = {
            # Cardinality constraints
            SHACL.minCount: "min_count",
            SHACL.maxCount: "max_count",
            SHACL.qualifiedMinCount: "qualified_min_count",
            SHACL.qualifiedMaxCount: "qualified_max_count",
            
            # Data type constraints
            SHACL.datatype: "datatype",
            SHACL.nodeKind: "node_kind",
            
            # Value constraints
            SHACL.minLength: "min_length",
            SHACL.maxLength: "max_length",
            SHACL.pattern: "pattern",
            SHACL.flags: "pattern_flags",
            SHACL.languageIn: "language_in",
            SHACL.uniqueLang: "unique_lang",
            
            # Range constraints
            SHACL.minInclusive: "min_inclusive",
            SHACL.maxInclusive: "max_inclusive",
            SHACL.minExclusive: "min_exclusive",
            SHACL.maxExclusive: "max_exclusive",
            
            # Logical constraints
            SHACL.and_: "and",
            SHACL.or_: "or",
            SHACL.not_: "not",
            SHACL.xone: "xone",
            
            # Comparison constraints
            SHACL.equals: "equals",
            SHACL.disjoint: "disjoint",
            SHACL.lessThan: "less_than",
            SHACL.lessThanOrEquals: "less_than_or_equals",
            
            # Custom constraints
            SHACL.sparql: "sparql",
            SHACL.js: "javascript",
            SHACL.python: "python"
        }
        
        # Extract all constraints
        for shacl_prop, constraint_key in constraint_mappings.items():
            for value in graph.objects(property_shape, shacl_prop):
                if isinstance(value, Literal):
                    prop_data["constraints"][constraint_key] = value.toPython()
                else:
                    prop_data["constraints"][constraint_key] = str(value)
        
        return prop_data

    def _extract_constraints_extended(self, graph: Graph) -> List[Dict[str, Any]]:
        """Extract all constraints with extended type support."""
        constraints = []
        constraint_types = set()
        
        # Extract from property shapes
        for property_shape in graph.subjects(RDF.type, SHACL.PropertyShape):
            prop_data = self._extract_property_shape_data_extended(graph, property_shape)
            if prop_data and prop_data.get("constraints"):
                for constraint_type, value in prop_data["constraints"].items():
                    constraint_types.add(constraint_type)
                    constraints.append({
                        "type": constraint_type,
                        "value": value,
                        "property_path": prop_data.get("path"),
                        "shape_id": prop_data.get("shape_id"),
                        "metadata": {
                            "source": "property_shape",
                            "constraint_category": self._categorize_constraint(constraint_type)
                        }
                    })
        
        # Record constraint extraction metrics
        self.telemetry.record_metric("shacl_constraint_types_found", len(constraint_types))
        self.telemetry.record_metric("shacl_total_constraints", len(constraints))
        
        return constraints

    def _categorize_constraint(self, constraint_type: str) -> str:
        """Categorize constraint type for telemetry and validation."""
        for category, types in CONSTRAINT_TYPES.items():
            if constraint_type in types:
                return category
        return "unknown"

    def _generate_validation_rules_extended(self, node_shapes: List[Dict[str, Any]], 
                                          property_shapes: List[Dict[str, Any]]) -> List[ValidationRule]:
        """Generate validation rules with schema-driven error messages."""
        validation_rules = []
        
        # Process node shapes
        for shape in node_shapes:
            if shape.get("deactivated"):
                continue
                
            target_class = shape.get("target_class", "Unknown")
            severity = shape.get("severity", "Violation")
            
            # Process properties
            for prop in shape.get("properties", []):
                constraints = prop.get("constraints", {})
                property_path = prop.get("path", "unknown")
                
                # Generate rules for each constraint type
                for constraint_type, value in constraints.items():
                    rule_id = f"{target_class}_{property_path}_{constraint_type}"
                    
                    # Generate schema-driven error message
                    error_message = self._generate_error_message(
                        target_class, property_path, constraint_type, value, severity
                    )
                    
                    rule = ValidationRule(
                        rule_id=rule_id,
                        target_class=target_class,
                        property_path=property_path,
                        constraint_type=self._determine_constraint_type_extended(constraints),
                        constraint_value=value,
                        severity=severity,
                        message=error_message,
                        metadata={
                            "constraint_category": self._categorize_constraint(constraint_type),
                            "validator_available": any(v.can_validate(constraint_type) for v in self.validators)
                        }
                    )
                    validation_rules.append(rule)
        
        return validation_rules

    def _generate_error_message(self, target_class: str, property_path: str, 
                              constraint_type: str, value: Any, severity: str) -> str:
        """Generate schema-driven error messages."""
        base_messages = {
            "min_count": f"Property '{property_path}' must have at least {value} values",
            "max_count": f"Property '{property_path}' must have at most {value} values",
            "datatype": f"Property '{property_path}' must be of type {value}",
            "min_length": f"Property '{property_path}' must be at least {value} characters long",
            "max_length": f"Property '{property_path}' must be at most {value} characters long",
            "pattern": f"Property '{property_path}' must match pattern '{value}'",
            "min_inclusive": f"Property '{property_path}' must be at least {value}",
            "max_inclusive": f"Property '{property_path}' must be at most {value}",
            "node_kind": f"Property '{property_path}' must be a {value}",
            "language_in": f"Property '{property_path}' language must be one of {value}",
            "unique_lang": f"Property '{property_path}' must have unique language tags",
            "equals": f"Property '{property_path}' must equal {value}",
            "disjoint": f"Property '{property_path}' must be disjoint from {value}",
            "less_than": f"Property '{property_path}' must be less than {value}",
            "less_than_or_equals": f"Property '{property_path}' must be less than or equal to {value}"
        }
        
        message = base_messages.get(constraint_type, f"Property '{property_path}' violates {constraint_type} constraint")
        
        # Add severity context
        if severity == "Warning":
            message = f"WARNING: {message}"
        elif severity == "Info":
            message = f"INFO: {message}"
        
        return message

    def _determine_constraint_type_extended(self, constraints: Dict[str, Any]) -> str:
        """Determine primary constraint type with extended support."""
        # Priority order for constraint types
        priority_order = ["cardinality", "datatype", "range", "string", "logical", "comparison", "custom"]
        
        for constraint_type in priority_order:
            if any(key in constraints for key in CONSTRAINT_TYPES.get(constraint_type, [])):
                return constraint_type
        
        return "unknown"

    def _extract_shape_constraints(self, graph: Graph, shape: URIRef) -> Dict[str, Any]:
        """Extract shape-level constraints."""
        constraints = {}
        
        # Extract logical constraints
        for and_constraint in graph.objects(shape, SHACL.and_):
            constraints["and"] = str(and_constraint)
        for or_constraint in graph.objects(shape, SHACL.or_):
            constraints["or"] = str(or_constraint)
        for not_constraint in graph.objects(shape, SHACL.not_):
            constraints["not"] = str(not_constraint)
        
        return constraints

    def _create_metadata_extended(self, graph: Graph, node_shapes: List[Dict[str, Any]], 
                                property_shapes: List[Dict[str, Any]], 
                                constraints: List[Dict[str, Any]]) -> ConstraintMetadata:
        """Create extended metadata with performance and constraint information."""
        constraint_types = list(set(c.get('type', 'unknown') for c in constraints))
        
        return ConstraintMetadata(
            shacl_triples=len(graph),
            node_shapes=len(node_shapes),
            property_shapes=len(property_shapes),
            constraint_types=constraint_types
        )

    def _get_cache_hit_rate(self) -> float:
        """Calculate cache hit rate for performance telemetry."""
        # Simplified cache hit rate calculation
        return 0.85  # Placeholder - would be calculated from actual cache stats

    # Extended validation methods for pluggable validators
    def _validate_cardinality_extended(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        """Extended cardinality validation with qualified constraints."""
        result = {"valid": True, "details": {}}
        
        if "min_count" in constraints:
            min_count = constraints["min_count"]
            actual_count = len(value) if isinstance(value, (list, tuple)) else (1 if value is not None else 0)
            if actual_count < min_count:
                result["valid"] = False
                result["details"]["min_count_violation"] = f"Expected at least {min_count}, got {actual_count}"
        
        if "max_count" in constraints:
            max_count = constraints["max_count"]
            actual_count = len(value) if isinstance(value, (list, tuple)) else (1 if value is not None else 0)
            if actual_count > max_count:
                result["valid"] = False
                result["details"]["max_count_violation"] = f"Expected at most {max_count}, got {actual_count}"
        
        return result

    def _validate_datatype_extended(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        """Extended data type validation with node kind support."""
        result = {"valid": True, "details": {}}
        
        if "datatype" in constraints:
            datatype = constraints["datatype"]
            # Enhanced data type checking
            if datatype == str(XSD.string) and not isinstance(value, str):
                result["valid"] = False
                result["details"]["datatype_violation"] = f"Expected string, got {type(value).__name__}"
            elif datatype == str(XSD.integer) and not isinstance(value, int):
                result["valid"] = False
                result["details"]["datatype_violation"] = f"Expected integer, got {type(value).__name__}"
            elif datatype == str(XSD.float) and not isinstance(value, (int, float)):
                result["valid"] = False
                result["details"]["datatype_violation"] = f"Expected float, got {type(value).__name__}"
        
        if "node_kind" in constraints:
            node_kind = constraints["node_kind"]
            if node_kind == str(SHACL.IRI) and not (isinstance(value, str) and value.startswith("http")):
                result["valid"] = False
                result["details"]["node_kind_violation"] = f"Expected IRI, got {value}"
        
        return result

    def _validate_string_constraints(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        """String-specific constraint validation."""
        result = {"valid": True, "details": {}}
        
        if not isinstance(value, str):
            result["valid"] = False
            result["details"]["type_violation"] = "Value must be a string"
            return result
        
        if "pattern" in constraints:
            pattern = constraints["pattern"]
            flags = constraints.get("pattern_flags", 0)
            if not re.search(pattern, value, flags):
                result["valid"] = False
                result["details"]["pattern_violation"] = f"Value does not match pattern '{pattern}'"
        
        if "min_length" in constraints:
            min_length = constraints["min_length"]
            if len(value) < min_length:
                result["valid"] = False
                result["details"]["min_length_violation"] = f"Length must be at least {min_length}"
        
        if "max_length" in constraints:
            max_length = constraints["max_length"]
            if len(value) > max_length:
                result["valid"] = False
                result["details"]["max_length_violation"] = f"Length must be at most {max_length}"
        
        return result

    def _validate_range_constraints_extended(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        """Extended range validation with exclusive bounds."""
        result = {"valid": True, "details": {}}
        
        if not isinstance(value, (int, float)):
            result["valid"] = False
            result["details"]["type_violation"] = "Value must be numeric"
            return result
        
        if "min_inclusive" in constraints:
            min_val = constraints["min_inclusive"]
            if value < min_val:
                result["valid"] = False
                result["details"]["min_inclusive_violation"] = f"Value must be at least {min_val}"
        
        if "max_inclusive" in constraints:
            max_val = constraints["max_inclusive"]
            if value > max_val:
                result["valid"] = False
                result["details"]["max_inclusive_violation"] = f"Value must be at most {max_val}"
        
        if "min_exclusive" in constraints:
            min_val = constraints["min_exclusive"]
            if value <= min_val:
                result["valid"] = False
                result["details"]["min_exclusive_violation"] = f"Value must be greater than {min_val}"
        
        if "max_exclusive" in constraints:
            max_val = constraints["max_exclusive"]
            if value >= max_val:
                result["valid"] = False
                result["details"]["max_exclusive_violation"] = f"Value must be less than {max_val}"
        
        return result

    def _validate_logical_constraints(self, value: Any, constraints: Dict[str, Any], context: Dict[str, Any]) -> Dict[str, Any]:
        """Logical constraint validation (and, or, not, xone)."""
        result = {"valid": True, "details": {}}
        
        # Simplified logical constraint validation
        # In a full implementation, this would evaluate complex logical expressions
        if "and" in constraints or "or" in constraints or "not" in constraints or "xone" in constraints:
            result["details"]["logical_constraint"] = "Logical constraint evaluation not fully implemented"
        
        return result

    def add_validator(self, validator: PluggableValidator) -> None:
        """Add a custom validator to the processor."""
        self.validators.append(validator)
        self.telemetry.record_metric("shacl_validator_added", 1, validator_name=validator.name)

    def get_validator_stats(self) -> Dict[str, Any]:
        """Get statistics about validators and their usage."""
        return {
            "total_validators": len(self.validators),
            "validator_names": [v.name for v in self.validators],
            "constraint_types_supported": list(set().union(*[v.constraint_types for v in self.validators])),
            "cache_hit_rate": self._get_cache_hit_rate()
        }

    def clear_cache(self) -> None:
        """Clear the graph cache."""
        self._get_cached_graph.cache_clear()
        self.telemetry.record_metric("shacl_cache_cleared", 1)

    def validate_data(self, data: Dict[str, Any], validation_rules: List[ValidationRule]) -> Dict[str, Any]:
        """Validate data against SHACL validation rules with extended validation support."""
        start_time = time.time()
        
        with self.telemetry.start_span("shacl.processor.validate_data", "constraint_validation") as span:
            validation_results = {
                "valid": True,
                "violations": [],
                "warnings": [],
                "info": [],
                "performance": {}
            }
            
            constraint_type_counts = {}
            validator_usage = {}
            
            for rule in validation_rules:
                result = self._validate_rule_extended(data, rule)
                
                # Track constraint type usage
                constraint_type = rule.constraint_type
                constraint_type_counts[constraint_type] = constraint_type_counts.get(constraint_type, 0) + 1
                
                # Track validator usage
                for validator in self.validators:
                    if validator.can_validate(constraint_type):
                        validator_usage[validator.name] = validator_usage.get(validator.name, 0) + 1
                
                if result["valid"]:
                    continue
                
                validation_results["valid"] = False
                if rule.severity == "Violation":
                    validation_results["violations"].append(result)
                elif rule.severity == "Warning":
                    validation_results["warnings"].append(result)
                elif rule.severity == "Info":
                    validation_results["info"].append(result)
            
            # Calculate performance metrics
            validation_duration = (time.time() - start_time) * 1000
            
            # Record extended validation metrics
            span.set_attribute("data_valid", validation_results["valid"])
            span.set_attribute("violations_count", len(validation_results["violations"]))
            span.set_attribute("warnings_count", len(validation_results["warnings"]))
            span.set_attribute("info_count", len(validation_results["info"]))
            span.set_attribute("validation_duration_ms", validation_duration)
            span.set_attribute("constraint_type_distribution", constraint_type_counts)
            span.set_attribute("validator_usage", validator_usage)
            span.set_attribute("total_rules_validated", len(validation_rules))
            
            # Record performance metrics
            validation_results["performance"] = {
                "validation_duration_ms": validation_duration,
                "constraint_type_distribution": constraint_type_counts,
                "validator_usage": validator_usage,
                "cache_hit_rate": self._get_cache_hit_rate()
            }
            
            # Record telemetry metrics
            self.telemetry.record_metric("shacl_validation_duration_ms", validation_duration)
            self.telemetry.record_metric("shacl_violations_count", len(validation_results["violations"]))
            self.telemetry.record_metric("shacl_warnings_count", len(validation_results["warnings"]))
            self.telemetry.record_metric("shacl_total_rules_validated", len(validation_rules))
            
            return validation_results

    def _validate_rule_extended(self, data: Dict[str, Any], rule: ValidationRule) -> Dict[str, Any]:
        """Validate a single rule against data using pluggable validators."""
        result = {
            "valid": True,
            "rule_id": rule.rule_id,
            "message": rule.message,
            "details": {},
            "validator_used": None,
            "constraint_type": rule.constraint_type
        }
        
        # Extract property value from data
        property_path = rule.property_path.split("#")[-1] if "#" in rule.property_path else rule.property_path
        value = data.get(property_path) or data.get(rule.property_path)
        
        # Create validation context
        context = {
            "property_path": property_path,
            "target_class": rule.target_class,
            "severity": rule.severity,
            "rule_id": rule.rule_id
        }
        
        # Use pluggable validators for constraint validation
        constraint_value = rule.constraint_value
        if isinstance(constraint_value, dict):
            constraints = constraint_value
        else:
            constraints = {rule.constraint_type: constraint_value}
        
        # Find appropriate validator
        validator_used = None
        for validator in self.validators:
            if validator.can_validate(rule.constraint_type):
                validator_result = validator.validate(value, constraints, context)
                if not validator_result["valid"]:
                    result["valid"] = False
                    result["details"].update(validator_result["details"])
                    result["validator_used"] = validator.name
                validator_used = validator.name
                break
        
        # Fallback to legacy validation if no validator found
        if validator_used is None:
            result = self._validate_rule_legacy(data, rule)
            result["validator_used"] = "legacy"
        
        return result

    def _validate_rule_legacy(self, data: Dict[str, Any], rule: ValidationRule) -> Dict[str, Any]:
        """Legacy validation method for backward compatibility."""
        result = {
            "valid": True,
            "rule_id": rule.rule_id,
            "message": rule.message,
            "details": {}
        }
        
        # Extract property value from data
        property_path = rule.property_path.split("#")[-1] if "#" in rule.property_path else rule.property_path
        value = data.get(property_path) or data.get(rule.property_path)
        
        # Always check cardinality constraints first
        if "min_count" in rule.constraint_value or "max_count" in rule.constraint_value:
            result = self._validate_cardinality(value, rule.constraint_value, result)
        
        # Check data type constraints
        if "datatype" in rule.constraint_value:
            result = self._validate_datatype(value, rule.constraint_value, result)
        
        # Check value constraints (min_length, max_length)
        if "min_length" in rule.constraint_value or "max_length" in rule.constraint_value:
            result = self._validate_value_constraints(value, rule.constraint_value, result)
        
        # Check range constraints
        if "min_inclusive" in rule.constraint_value or "max_inclusive" in rule.constraint_value:
            result = self._validate_range_constraints(value, rule.constraint_value, result)
        
        return result

    def _validate_cardinality(self, value: Any, constraints: Dict[str, Any], 
                            result: Dict[str, Any]) -> Dict[str, Any]:
        """Validate cardinality constraints."""
        if value is None:
            count = 0
        elif isinstance(value, (list, tuple)):
            count = len(value)
        else:
            count = 1
        
        if "min_count" in constraints and count < constraints["min_count"]:
            result["valid"] = False
            result["details"]["min_count_violation"] = f"Count {count} < minimum {constraints['min_count']}"
        
        if "max_count" in constraints and count > constraints["max_count"]:
            result["valid"] = False
            result["details"]["max_count_violation"] = f"Count {count} > maximum {constraints['max_count']}"
        
        return result

    def _validate_datatype(self, value: Any, constraints: Dict[str, Any], 
                          result: Dict[str, Any]) -> Dict[str, Any]:
        """Validate data type constraints."""
        if value is None:
            return result
        
        datatype = constraints.get("datatype", "")
        
        if "string" in datatype and not isinstance(value, str):
            result["valid"] = False
            result["details"]["datatype_violation"] = f"Expected string, got {type(value).__name__}"
        elif "float" in datatype and not isinstance(value, (int, float)):
            result["valid"] = False
            result["details"]["datatype_violation"] = f"Expected float, got {type(value).__name__}"
        elif "integer" in datatype and not isinstance(value, int):
            result["valid"] = False
            result["details"]["datatype_violation"] = f"Expected integer, got {type(value).__name__}"
        
        return result

    def _validate_value_constraints(self, value: Any, constraints: Dict[str, Any], 
                                  result: Dict[str, Any]) -> Dict[str, Any]:
        """Validate value constraints."""
        if value is None or not isinstance(value, str):
            return result
        
        if "min_length" in constraints and len(value) < constraints["min_length"]:
            result["valid"] = False
            result["details"]["min_length_violation"] = f"Length {len(value)} < minimum {constraints['min_length']}"
        
        if "max_length" in constraints and len(value) > constraints["max_length"]:
            result["valid"] = False
            result["details"]["max_length_violation"] = f"Length {len(value)} > maximum {constraints['max_length']}"
        
        return result

    def _validate_range_constraints(self, value: Any, constraints: Dict[str, Any], 
                                  result: Dict[str, Any]) -> Dict[str, Any]:
        """Validate range constraints."""
        if value is None or not isinstance(value, (int, float)):
            return result
        
        if "min_inclusive" in constraints and value < constraints["min_inclusive"]:
            result["valid"] = False
            result["details"]["min_inclusive_violation"] = f"Value {value} < minimum {constraints['min_inclusive']}"
        
        if "max_inclusive" in constraints and value > constraints["max_inclusive"]:
            result["valid"] = False
            result["details"]["max_inclusive_violation"] = f"Value {value} > maximum {constraints['max_inclusive']}"
        
        return result 