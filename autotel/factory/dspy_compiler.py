"""DSPy compiler for AutoTel semantic execution pipeline."""

from typing import Dict, Any, List, Optional

from ..schemas.dspy_types import DSPySignature, DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration
from ..schemas.ontology_types import OntologySchema
from ..schemas.validation_types import ValidationRules
from ..core.telemetry import create_telemetry_manager
from opentelemetry import trace


class DSPyCompiler:
    """Compiles DSPy signatures with ontology and validation context."""

    def __init__(self):
        """Initialize DSPy compiler with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-dspy-compiler",
            require_linkml_validation=False
        )

    def compile(
        self,
        ontology_schema: OntologySchema,
        validation_rules: ValidationRules,
        dspy_signatures: List[DSPySignatureDefinition],
        dspy_modules: List[DSPyModuleDefinition],
        model_config: DSPyModelConfiguration
    ) -> DSPySignature:
        """Compile DSPy components into executable signature."""
        with self.telemetry.start_span("dspy_compile_signature", "dspy_processing") as span:
            span.set_attribute("signatures_count", len(dspy_signatures))
            span.set_attribute("modules_count", len(dspy_modules))
            span.set_attribute("ontology_classes_count", len(ontology_schema.classes))
            span.set_attribute("validation_rules_count", validation_rules.constraint_count)
            
            try:
                # Merge multiple signatures into a single signature
                merged_signature = self._merge_signatures(dspy_signatures)
                
                # Integrate ontology context
                signature_with_ontology = self._integrate_ontology_context(merged_signature, ontology_schema)
                
                # Integrate validation rules
                signature_with_validation = self._integrate_validation_rules(signature_with_ontology, validation_rules)
                
                # Create final DSPy signature
                final_signature = self._create_dspy_signature(
                    signature_with_validation,
                    dspy_modules,
                    model_config,
                    ontology_schema,
                    validation_rules
                )
                
                span.set_attribute("compile_success", True)
                self.telemetry.record_metric("dspy_compile_success", 1)
                
                return final_signature
                
            except Exception as e:
                span.set_attribute("compile_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                self.telemetry.record_metric("dspy_compile_failure", 1)
                raise

    def _merge_signatures(self, signatures: List[DSPySignatureDefinition]) -> DSPySignatureDefinition:
        """Merge multiple signatures into a single signature."""
        if not signatures:
            return DSPySignatureDefinition(
                name="default_signature",
                description="Default merged signature",
                inputs={},
                outputs={},
                examples=[]
            )
        
        if len(signatures) == 1:
            return signatures[0]
        
        # Merge multiple signatures
        merged_inputs = {}
        merged_outputs = {}
        merged_examples = []
        
        for signature in signatures:
            # Merge inputs
            for input_name, input_def in signature.inputs.items():
                if input_name not in merged_inputs:
                    merged_inputs[input_name] = input_def
                else:
                    # Merge input definitions (take the more specific one)
                    existing = merged_inputs[input_name]
                    if input_def.get("description") and not existing.get("description"):
                        existing["description"] = input_def["description"]
                    if input_def.get("owl_class") and not existing.get("owl_class"):
                        existing["owl_class"] = input_def["owl_class"]
            
            # Merge outputs
            for output_name, output_def in signature.outputs.items():
                if output_name not in merged_outputs:
                    merged_outputs[output_name] = output_def
                else:
                    # Merge output definitions
                    existing = merged_outputs[output_name]
                    if output_def.get("description") and not existing.get("description"):
                        existing["description"] = output_def["description"]
                    if output_def.get("owl_class") and not existing.get("owl_class"):
                        existing["owl_class"] = output_def["owl_class"]
            
            # Merge examples
            merged_examples.extend(signature.examples)
        
        return DSPySignatureDefinition(
            name="merged_signature",
            description=f"Merged signature from {len(signatures)} signatures",
            inputs=merged_inputs,
            outputs=merged_outputs,
            examples=merged_examples
        )

    def _integrate_ontology_context(self, signature: DSPySignatureDefinition, ontology: OntologySchema) -> DSPySignatureDefinition:
        """Integrate ontology context into signature."""
        enhanced_inputs = {}
        enhanced_outputs = {}
        
        # Enhance inputs with ontology information
        for input_name, input_def in signature.inputs.items():
            enhanced_input = input_def.copy()
            
            # Map to ontology class if available
            owl_class = input_def.get("owl_class")
            if owl_class and owl_class in ontology.classes:
                class_schema = ontology.classes[owl_class]
                enhanced_input["semantic_type"] = class_schema.semantic_type
                enhanced_input["ontology_class"] = class_schema.name
                enhanced_input["properties"] = class_schema.properties
                enhanced_input["description"] = class_schema.description or enhanced_input.get("description", "")
            
            enhanced_inputs[input_name] = enhanced_input
        
        # Enhance outputs with ontology information
        for output_name, output_def in signature.outputs.items():
            enhanced_output = output_def.copy()
            
            # Map to ontology class if available
            owl_class = output_def.get("owl_class")
            if owl_class and owl_class in ontology.classes:
                class_schema = ontology.classes[owl_class]
                enhanced_output["semantic_type"] = class_schema.semantic_type
                enhanced_output["ontology_class"] = class_schema.name
                enhanced_output["properties"] = class_schema.properties
                enhanced_output["description"] = class_schema.description or enhanced_output.get("description", "")
            
            enhanced_outputs[output_name] = enhanced_output
        
        return DSPySignatureDefinition(
            name=signature.name,
            description=signature.description,
            inputs=enhanced_inputs,
            outputs=enhanced_outputs,
            examples=signature.examples
        )

    def _integrate_validation_rules(self, signature: DSPySignatureDefinition, validation: ValidationRules) -> DSPySignatureDefinition:
        """Integrate validation rules into signature."""
        enhanced_inputs = {}
        enhanced_outputs = {}
        
        # Apply validation rules to inputs
        for input_name, input_def in signature.inputs.items():
            enhanced_input = input_def.copy()
            input_rules = []
            
            # Find validation rules for this input
            for class_name, rules in validation.target_classes.items():
                for rule in rules:
                    if rule.property_path == input_name or class_name in input_name:
                        input_rules.append({
                            "rule_id": rule.rule_id,
                            "constraint_type": rule.constraint_type,
                            "constraint_value": rule.constraint_value,
                            "severity": rule.severity,
                            "message": rule.message
                        })
            
            if input_rules:
                enhanced_input["validation_rules"] = input_rules
            
            enhanced_inputs[input_name] = enhanced_input
        
        # Apply validation rules to outputs
        for output_name, output_def in signature.outputs.items():
            enhanced_output = output_def.copy()
            output_rules = []
            
            # Find validation rules for this output
            for class_name, rules in validation.target_classes.items():
                for rule in rules:
                    if rule.property_path == output_name or class_name in output_name:
                        output_rules.append({
                            "rule_id": rule.rule_id,
                            "constraint_type": rule.constraint_type,
                            "constraint_value": rule.constraint_value,
                            "severity": rule.severity,
                            "message": rule.message
                        })
            
            if output_rules:
                enhanced_output["validation_rules"] = output_rules
            
            enhanced_outputs[output_name] = enhanced_output
        
        return DSPySignatureDefinition(
            name=signature.name,
            description=signature.description,
            inputs=enhanced_inputs,
            outputs=enhanced_outputs,
            examples=signature.examples
        )

    def _create_dspy_signature(
        self,
        signature: DSPySignatureDefinition,
        modules: List[DSPyModuleDefinition],
        model_config: DSPyModelConfiguration,
        ontology: OntologySchema,
        validation: ValidationRules
    ) -> DSPySignature:
        """Create DSPy signature from components."""
        # Convert inputs to OWL class mapping format
        inputs_mapping = {}
        for input_name, input_def in signature.inputs.items():
            inputs_mapping[input_name] = {
                "name": input_name,
                "type": input_def.get("type", "string"),
                "description": input_def.get("description", ""),
                "semantic_type": input_def.get("semantic_type", "user_input"),
                "ontology_class": input_def.get("ontology_class"),
                "validation_rules": input_def.get("validation_rules", []),
                "properties": input_def.get("properties", {})
            }
        
        # Convert outputs to OWL class mapping format
        outputs_mapping = {}
        for output_name, output_def in signature.outputs.items():
            outputs_mapping[output_name] = {
                "name": output_name,
                "type": output_def.get("type", "string"),
                "description": output_def.get("description", ""),
                "semantic_type": output_def.get("semantic_type", "recommendation"),
                "ontology_class": output_def.get("ontology_class"),
                "validation_rules": output_def.get("validation_rules", []),
                "properties": output_def.get("properties", {})
            }
        
        # Extract validation rules
        validation_rules = []
        for class_name, rules in validation.target_classes.items():
            for rule in rules:
                validation_rules.append({
                    "rule_id": rule.rule_id,
                    "target_class": rule.target_class,
                    "property_path": rule.property_path,
                    "constraint_type": rule.constraint_type,
                    "constraint_value": rule.constraint_value,
                    "severity": rule.severity,
                    "message": rule.message,
                    "metadata": rule.metadata
                })
        
        # Create semantic context
        semantic_context = {
            "ontology_uri": ontology.ontology_uri,
            "namespace": ontology.namespace,
            "classes": list(ontology.classes.keys()),
            "semantic_types": {name: cls.semantic_type for name, cls in ontology.classes.items()},
            "property_mappings": {},
            "class_hierarchies": {}
        }
        
        # Create ontology metadata
        ontology_metadata = {
            "ontology_uri": ontology.ontology_uri,
            "namespace": ontology.namespace,
            "prefix": ontology.prefix,
            "class_count": len(ontology.classes),
            "semantic_context": ontology.semantic_context,
            "examples": ontology.examples
        }
        
        # Get module configuration (use first module if available)
        module_config = modules[0] if modules else None
        
        return DSPySignature(
            name=signature.name,
            description=signature.description,
            inputs=inputs_mapping,
            outputs=outputs_mapping,
            validation_rules=validation_rules,
            examples=signature.examples,
            model_config=model_config,
            module_config=module_config,
            semantic_context=semantic_context,
            ontology_metadata=ontology_metadata
        ) 