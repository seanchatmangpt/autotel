"""Semantic linker for AutoTel semantic execution pipeline."""

from typing import Dict, Any, List

from ..schemas.dspy_types import DSPySignature
from ..schemas.linker_types import ExecutableSystem
from ..core.telemetry import create_telemetry_manager
from opentelemetry import trace


class SemanticLinker:
    """Creates executable systems from compiled DSPy signatures."""

    def __init__(self):
        """Initialize semantic linker with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-semantic-linker",
            require_linkml_validation=False
        )

    def link(
        self, 
        ontology_schema: Any, 
        validation_rules: Any, 
        dspy_signature: Any
    ) -> ExecutableSystem:
        """Link ontology, validation rules, and DSPy signature into executable system configuration."""
        with self.telemetry.start_span("semantic_link_system", "dspy_processing") as span:
            span.set_attribute("has_ontology", ontology_schema is not None)
            span.set_attribute("has_validation_rules", validation_rules is not None)
            span.set_attribute("has_dspy_signature", dspy_signature is not None)
            
            try:
                # Create a basic executable system from available components
                if dspy_signature:
                    # Use DSPy signature as base
                    signature = dspy_signature
                    span.set_attribute("signature_name", signature.name)
                    span.set_attribute("inputs_count", len(signature.inputs))
                    span.set_attribute("outputs_count", len(signature.outputs))
                else:
                    # Create a minimal signature if none provided
                    signature = DSPySignature(
                        name="default_signature",
                        description="Default signature from pipeline",
                        inputs={},
                        outputs={},
                        examples=[],
                        validation_rules=[],
                        model_config=None,
                        module_config=None,
                        ontology_metadata={}
                    )
                
                # Validate signature before linking
                self._validate_signature(signature)
                
                # Generate semantic context
                semantic_context = self._generate_semantic_context(signature)
                
                # Prepare validation rules
                validation_rules_list = self._prepare_validation_rules(signature)
                
                # Generate metadata
                metadata = self._generate_metadata(signature)
                
                # Create executable system
                executable_system = ExecutableSystem(
                    signature=signature,
                    semantic_context=semantic_context,
                    validation_rules=validation_rules_list,
                    examples=signature.examples,
                    model_config=signature.model_config,
                    module_config=signature.module_config,
                    shacl_validation=True,
                    shacl_triples=len(signature.validation_rules),
                    metadata=metadata
                )
                
                span.set_attribute("link_success", True)
                span.set_attribute("semantic_context_size", len(str(semantic_context)))
                span.set_attribute("validation_rules_prepared", len(validation_rules_list))
                
                self.telemetry.record_metric("semantic_link_success", 1)
                
                return executable_system
                
            except Exception as e:
                span.set_attribute("link_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                self.telemetry.record_metric("semantic_link_failure", 1)
                raise

    def _validate_signature(self, signature: DSPySignature) -> None:
        """Validate DSPy signature before linking."""
        # Check required fields
        if not signature.name:
            raise ValueError("Signature name is required")
        
        if not signature.inputs and not signature.outputs:
            raise ValueError("Signature must have at least one input or output")
        
        # Validate input/output structure
        for input_name, input_def in signature.inputs.items():
            if not isinstance(input_def, dict):
                raise ValueError(f"Input definition for '{input_name}' must be a dictionary")
            if "name" not in input_def:
                raise ValueError(f"Input '{input_name}' must have a 'name' field")
        
        for output_name, output_def in signature.outputs.items():
            if not isinstance(output_def, dict):
                raise ValueError(f"Output definition for '{output_name}' must be a dictionary")
            if "name" not in output_def:
                raise ValueError(f"Output '{output_name}' must have a 'name' field")
        
        # Validate model configuration if present
        if signature.model_config:
            if not signature.model_config.provider:
                raise ValueError("Model configuration must have a provider")
            if not signature.model_config.model_name:
                raise ValueError("Model configuration must have a model name")

    def _generate_semantic_context(self, signature: DSPySignature) -> Dict[str, Any]:
        """Generate semantic context from signature."""
        semantic_context = {
            "execution_context": {
                "signature_name": signature.name,
                "description": signature.description,
                "model_provider": signature.model_config.provider if signature.model_config else "default",
                "model_name": signature.model_config.model_name if signature.model_config else "default",
                "temperature": signature.model_config.parameters.get("temperature", 0.7) if signature.model_config else 0.7
            },
            "semantic_types": {},
            "ontology_classes": [],
            "property_mappings": {},
            "class_hierarchies": {},
            "validation_context": {
                "rules_count": len(signature.validation_rules),
                "severity_levels": {}
            }
        }
        
        # Extract semantic types from inputs and outputs
        for input_name, input_def in signature.inputs.items():
            semantic_type = input_def.get("semantic_type", "user_input")
            semantic_context["semantic_types"][input_name] = semantic_type
            
            ontology_class = input_def.get("ontology_class")
            if ontology_class:
                semantic_context["ontology_classes"].append(ontology_class)
        
        for output_name, output_def in signature.outputs.items():
            semantic_type = output_def.get("semantic_type", "recommendation")
            semantic_context["semantic_types"][output_name] = semantic_type
            
            ontology_class = output_def.get("ontology_class")
            if ontology_class:
                semantic_context["ontology_classes"].append(ontology_class)
        
        # Extract validation context
        for rule in signature.validation_rules:
            severity = rule.get("severity", "Violation")
            if severity not in semantic_context["validation_context"]["severity_levels"]:
                semantic_context["validation_context"]["severity_levels"][severity] = 0
            semantic_context["validation_context"]["severity_levels"][severity] += 1
        
        # Add ontology metadata if available
        if signature.ontology_metadata:
            semantic_context["ontology_metadata"] = signature.ontology_metadata
        
        return semantic_context

    def _prepare_validation_rules(self, signature: DSPySignature) -> List[Dict[str, Any]]:
        """Prepare validation rules for execution."""
        prepared_rules = []
        
        for rule in signature.validation_rules:
            prepared_rule = {
                "rule_id": rule.get("rule_id", f"rule_{len(prepared_rules)}"),
                "target_class": rule.get("target_class", ""),
                "property_path": rule.get("property_path", ""),
                "constraint_type": rule.get("constraint_type", "unknown"),
                "constraint_value": rule.get("constraint_value"),
                "severity": rule.get("severity", "Violation"),
                "message": rule.get("message", ""),
                "executable": True,
                "validation_function": self._create_validation_function(rule)
            }
            prepared_rules.append(prepared_rule)
        
        return prepared_rules

    def _create_validation_function(self, rule: Dict[str, Any]) -> str:
        """Create validation function name based on rule type."""
        constraint_type = rule.get("constraint_type", "unknown")
        
        if constraint_type == "cardinality":
            return "validate_cardinality"
        elif constraint_type == "datatype":
            return "validate_datatype"
        elif constraint_type == "value":
            return "validate_value"
        elif constraint_type == "logical":
            return "validate_logical"
        else:
            return "validate_generic"

    def _generate_metadata(self, signature: DSPySignature) -> Dict[str, Any]:
        """Generate metadata for executable system."""
        metadata = {
            "system_info": {
                "name": signature.name,
                "description": signature.description,
                "created_at": "2024-01-01T00:00:00Z",  # Would be dynamic in real implementation
                "version": "1.0.0"
            },
            "execution_context": {
                "signature_name": signature.name,
                "model_provider": signature.model_config.provider if signature.model_config else "default",
                "model_name": signature.model_config.model_name if signature.model_config else "default",
                "temperature": signature.model_config.parameters.get("temperature", 0.7) if signature.model_config else 0.7
            },
            "telemetry_config": {
                "enabled": True,
                "sampling_rate": 1.0,
                "span_attributes": {
                    "signature_name": signature.name,
                    "inputs_count": len(signature.inputs),
                    "outputs_count": len(signature.outputs),
                    "validation_rules_count": len(signature.validation_rules)
                }
            },
            "validation_metadata": {
                "rules_count": len(signature.validation_rules),
                "severity_distribution": {},
                "constraint_types": {}
            },
            "ontology_metadata": {
                "classes_count": len(signature.semantic_context.get("ontology_classes", [])),
                "semantic_types_count": len(signature.semantic_context.get("semantic_types", {})),
                "ontology_uri": signature.ontology_metadata.get("ontology_uri", "") if signature.ontology_metadata else ""
            }
        }
        
        # Calculate severity distribution
        for rule in signature.validation_rules:
            severity = rule.get("severity", "Violation")
            if severity not in metadata["validation_metadata"]["severity_distribution"]:
                metadata["validation_metadata"]["severity_distribution"][severity] = 0
            metadata["validation_metadata"]["severity_distribution"][severity] += 1
            
            constraint_type = rule.get("constraint_type", "unknown")
            if constraint_type not in metadata["validation_metadata"]["constraint_types"]:
                metadata["validation_metadata"]["constraint_types"][constraint_type] = 0
            metadata["validation_metadata"]["constraint_types"][constraint_type] += 1
        
        return metadata 