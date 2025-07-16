"""Validation compiler for AutoTel semantic execution pipeline."""

from typing import Dict, Any, List
from rdflib import Graph

from ..schemas.validation_types import ValidationRule, ValidationRules, ConstraintMetadata
from ..core.telemetry import create_telemetry_manager
from opentelemetry import trace


class ValidationCompiler:
    """Compiles SHACL validation rules into executable validation schemas."""

    def __init__(self):
        """Initialize validation compiler with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-validation-compiler",
            require_linkml_validation=False
        )

    def compile(self, shacl_output: Dict[str, Any]) -> ValidationRules:
        """Compile SHACL processor output into validation schema."""
        with self.telemetry.start_span("validation.compiler.compile", "constraint_validation") as span:
            try:
                # Extract components from SHACL processor output
                graph = shacl_output.get("graph")
                node_shapes = shacl_output.get("node_shapes", [])
                property_shapes = shacl_output.get("property_shapes", [])
                constraints = shacl_output.get("constraints", [])
                validation_rules = shacl_output.get("validation_rules", [])
                metadata = shacl_output.get("metadata")
                
                # Record compilation metrics
                span.set_attribute("node_shapes_compiled", len(node_shapes))
                span.set_attribute("property_shapes_compiled", len(property_shapes))
                span.set_attribute("constraints_compiled", len(constraints))
                span.set_attribute("validation_rules_compiled", len(validation_rules))
                
                # Organize validation rules by target class
                target_classes = {}
                severity_levels = {"Violation": 0, "Warning": 0, "Info": 0}
                
                for rule in validation_rules:
                    target_class = rule.target_class
                    if target_class not in target_classes:
                        target_classes[target_class] = []
                    target_classes[target_class].append(rule)
                    
                    # Count severity levels
                    severity_levels[rule.severity] += 1
                
                # Create compiled validation rules
                compiled_rules = ValidationRules(
                    target_classes=target_classes,
                    constraint_count=len(constraints),
                    severity_levels=severity_levels,
                    metadata={
                        "shacl_triples": metadata.shacl_triples if metadata else 0,
                        "node_shapes": metadata.node_shapes if metadata else 0,
                        "property_shapes": metadata.property_shapes if metadata else 0,
                        "constraint_types": metadata.constraint_types if metadata else []
                    }
                )
                
                # Record successful compilation
                span.set_attribute("compilation_success", True)
                span.set_attribute("target_classes_count", len(target_classes))
                span.set_attribute("total_rules_count", len(validation_rules))
                
                # Record metric for successful compilation
                self.telemetry.record_metric("validation_compilation_success", 1)
                
                return compiled_rules
                
            except Exception as e:
                # Record compilation failure
                span.set_attribute("compilation_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric for compilation failure
                self.telemetry.record_metric("validation_compilation_failure", 1)
                
                raise

    def compile_from_graph(self, shacl_graph: Graph) -> ValidationRules:
        """Compile SHACL graph directly into validation schema."""
        with self.telemetry.start_span("validation.compiler.compile_from_graph", "constraint_validation") as span:
            try:
                # Extract node shapes from graph
                node_shapes = self._extract_node_shapes(shacl_graph)
                
                # Extract property shapes from graph
                property_shapes = self._extract_property_shapes(shacl_graph)
                
                # Extract constraints from graph
                constraints = self._extract_constraints(shacl_graph)
                
                # Create validation rules
                validation_rules = self._create_validation_rules(node_shapes, property_shapes)
                
                # Organize by target class
                target_classes = {}
                severity_levels = {"Violation": 0, "Warning": 0, "Info": 0}
                
                for rule in validation_rules:
                    target_class = rule.target_class
                    if target_class not in target_classes:
                        target_classes[target_class] = []
                    target_classes[target_class].append(rule)
                    severity_levels[rule.severity] += 1
                
                # Create compiled rules
                compiled_rules = ValidationRules(
                    target_classes=target_classes,
                    constraint_count=len(constraints),
                    severity_levels=severity_levels,
                    metadata={
                        "shacl_triples": len(shacl_graph),
                        "node_shapes": len(node_shapes),
                        "property_shapes": len(property_shapes),
                        "constraint_types": list(set(rule.constraint_type for rule in validation_rules))
                    }
                )
                
                # Record successful compilation
                span.set_attribute("compilation_success", True)
                span.set_attribute("node_shapes_extracted", len(node_shapes))
                span.set_attribute("property_shapes_extracted", len(property_shapes))
                span.set_attribute("constraints_extracted", len(constraints))
                span.set_attribute("validation_rules_created", len(validation_rules))
                
                return compiled_rules
                
            except Exception as e:
                span.set_attribute("compilation_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                raise

    def _extract_node_shapes(self, shacl_graph: Graph) -> List[Dict[str, Any]]:
        """Extract node shapes from SHACL graph."""
        with self.telemetry.start_span("validation.compiler.extract_node_shapes", "constraint_validation") as span:
            from rdflib.namespace import RDF
            from rdflib import Namespace
            
            SHACL = Namespace("http://www.w3.org/ns/shacl#")
            node_shapes = []
            
            # Find all NodeShape instances
            for node_shape in shacl_graph.subjects(RDF.type, SHACL.NodeShape):
                shape_data = {
                    "shape_id": str(node_shape),
                    "target_class": None,
                    "properties": [],
                    "deactivated": False,
                    "severity": "Violation"
                }
                
                # Extract target class
                for target_class in shacl_graph.objects(node_shape, SHACL.targetClass):
                    shape_data["target_class"] = str(target_class)
                
                # Check if shape is deactivated
                for deactivated in shacl_graph.objects(node_shape, SHACL.deactivated):
                    if deactivated:
                        shape_data["deactivated"] = True
                
                # Extract severity
                for severity in shacl_graph.objects(node_shape, SHACL.severity):
                    shape_data["severity"] = str(severity)
                
                # Extract properties
                for property_shape in shacl_graph.objects(node_shape, SHACL.property):
                    prop_data = self._extract_property_shape_data(shacl_graph, property_shape)
                    if prop_data:
                        shape_data["properties"].append(prop_data)
                
                node_shapes.append(shape_data)
            
            span.set_attribute("node_shapes_found", len(node_shapes))
            return node_shapes

    def _extract_property_shapes(self, shacl_graph: Graph) -> List[Dict[str, Any]]:
        """Extract property shapes from SHACL graph."""
        with self.telemetry.start_span("validation.compiler.extract_property_shapes", "constraint_validation") as span:
            from rdflib.namespace import RDF
            from rdflib import Namespace
            
            SHACL = Namespace("http://www.w3.org/ns/shacl#")
            property_shapes = []
            
            # Find all PropertyShape instances
            for property_shape in shacl_graph.subjects(RDF.type, SHACL.PropertyShape):
                prop_data = self._extract_property_shape_data(shacl_graph, property_shape)
                if prop_data:
                    property_shapes.append(prop_data)
            
            span.set_attribute("property_shapes_found", len(property_shapes))
            return property_shapes

    def _extract_property_shape_data(self, graph: Graph, property_shape) -> Dict[str, Any]:
        """Extract data from a property shape."""
        from rdflib import Namespace
        
        SHACL = Namespace("http://www.w3.org/ns/shacl#")
        
        prop_data = {
            "shape_id": str(property_shape),
            "path": None,
            "constraints": {}
        }
        
        # Extract property path
        for path in graph.objects(property_shape, SHACL.path):
            prop_data["path"] = str(path)
        
        # Extract cardinality constraints
        for min_count in graph.objects(property_shape, SHACL.minCount):
            prop_data["constraints"]["min_count"] = int(min_count)
        for max_count in graph.objects(property_shape, SHACL.maxCount):
            prop_data["constraints"]["max_count"] = int(max_count)
        
        # Extract data type constraints
        for datatype in graph.objects(property_shape, SHACL.datatype):
            prop_data["constraints"]["datatype"] = str(datatype)
        
        # Extract value constraints
        for min_length in graph.objects(property_shape, SHACL.minLength):
            prop_data["constraints"]["min_length"] = int(min_length)
        for max_length in graph.objects(property_shape, SHACL.maxLength):
            prop_data["constraints"]["max_length"] = int(max_length)
        
        # Extract range constraints
        for min_inclusive in graph.objects(property_shape, SHACL.minInclusive):
            prop_data["constraints"]["min_inclusive"] = float(min_inclusive)
        for max_inclusive in graph.objects(property_shape, SHACL.maxInclusive):
            prop_data["constraints"]["max_inclusive"] = float(max_inclusive)
        
        # Extract pattern constraints
        for pattern in graph.objects(property_shape, SHACL.pattern):
            prop_data["constraints"]["pattern"] = str(pattern)
        
        return prop_data

    def _extract_constraints(self, shacl_graph: Graph) -> List[Dict[str, Any]]:
        """Extract constraints from SHACL graph."""
        constraints = []
        
        # Extract constraints from property shapes
        from rdflib.namespace import RDF
        from rdflib import Namespace
        
        SHACL = Namespace("http://www.w3.org/ns/shacl#")
        
        for property_shape in shacl_graph.subjects(RDF.type, SHACL.PropertyShape):
            constraint_data = self._extract_property_shape_data(shacl_graph, property_shape)
            if constraint_data["constraints"]:
                constraints.append(constraint_data)
        
        return constraints

    def _create_validation_rules(self, node_shapes: List[Dict[str, Any]], 
                                property_shapes: List[Dict[str, Any]]) -> List[ValidationRule]:
        """Create validation rules from shape data."""
        validation_rules = []
        
        for node_shape in node_shapes:
            if node_shape["deactivated"]:
                continue
                
            for property_data in node_shape["properties"]:
                rule = ValidationRule(
                    rule_id=f"{node_shape['shape_id']}_{property_data['shape_id']}",
                    target_class=node_shape["target_class"] or "Unknown",
                    property_path=property_data["path"] or "Unknown",
                    constraint_type=self._determine_constraint_type(property_data["constraints"]),
                    constraint_value=property_data["constraints"],
                    severity=node_shape["severity"],
                    message=f"Validation rule for {property_data['path']} in {node_shape['target_class']}",
                    metadata={
                        "shape_id": node_shape["shape_id"],
                        "property_shape_id": property_data["shape_id"]
                    }
                )
                validation_rules.append(rule)
        
        return validation_rules

    def _determine_constraint_type(self, constraints: Dict[str, Any]) -> str:
        """Determine the primary constraint type from constraints."""
        if "min_count" in constraints or "max_count" in constraints:
            return "cardinality"
        elif "datatype" in constraints:
            return "datatype"
        elif "min_length" in constraints or "max_length" in constraints:
            return "value"
        elif "pattern" in constraints:
            return "pattern"
        elif "min_inclusive" in constraints or "max_inclusive" in constraints:
            return "range"
        else:
            return "logical" 