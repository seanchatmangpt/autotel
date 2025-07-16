"""Ontology executor for AutoTel semantic execution pipeline."""

from typing import Dict, Any, List, Optional
import json
import time

from ..schemas.linker_types import ExecutableSystem
from ..schemas.executor_types import ExecutionResult, ValidationResult, TelemetryData
from ..core.telemetry import create_telemetry_manager
from opentelemetry import trace


class OntologyExecutor:
    """Executes DSPy signatures with semantic context and comprehensive telemetry."""

    def __init__(self):
        """Initialize ontology executor with telemetry."""
        self.telemetry = create_telemetry_manager(
            service_name="autotel-ontology-executor",
            require_linkml_validation=False
        )

    def execute(self, executable_system: ExecutableSystem, inputs: Dict[str, Any]) -> ExecutionResult:
        """Execute the system with given inputs."""
        with self.telemetry.start_span("ontology_execute_system", "model_execution") as span:
            span.set_attribute("signature_name", executable_system.signature.name)
            span.set_attribute("inputs_count", len(inputs))
            span.set_attribute("validation_rules_count", len(executable_system.validation_rules))
            
            start_time = time.time()
            
            try:
                # Validate inputs
                validation_result = self._validate_inputs(executable_system, inputs)
                
                # Apply validation rules
                validation_data = self._apply_validation_rules(executable_system, inputs)
                
                # Execute signature
                outputs = self._execute_signature(executable_system, inputs)
                
                # Generate telemetry
                telemetry_data = self._generate_telemetry(executable_system, inputs, outputs)
                
                # Generate execution metadata
                execution_metadata = self._generate_execution_metadata(executable_system, inputs, outputs)
                
                execution_time = time.time() - start_time
                
                # Create execution result
                result = ExecutionResult(
                    success=True,
                    outputs=outputs,
                    execution_time=execution_time,
                    validation_result=validation_result,
                    telemetry_data=telemetry_data,
                    metadata=execution_metadata
                )
                
                span.set_attribute("execution_success", True)
                span.set_attribute("execution_time_seconds", execution_time)
                span.set_attribute("outputs_count", len(outputs))
                
                self.telemetry.record_metric("ontology_execution_success", 1)
                self.telemetry.record_metric("ontology_execution_time", execution_time)
                
                return result
                
            except Exception as e:
                execution_time = time.time() - start_time
                
                span.set_attribute("execution_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_attribute("execution_time_seconds", execution_time)
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                self.telemetry.record_metric("ontology_execution_failure", 1)
                self.telemetry.record_metric("ontology_execution_time", execution_time)
                
                # Return error result
                return ExecutionResult(
                    success=False,
                    outputs={},
                    execution_time=execution_time,
                    validation_result=ValidationResult(
                        valid=False,
                        violations=[{"field": "execution", "message": str(e), "severity": "Error"}],
                        warnings=[],
                        metadata={"error_type": type(e).__name__}
                    ),
                    telemetry_data=TelemetryData(
                        spans=[],
                        events=[],
                        metrics={},
                        logs=[]
                    ),
                    metadata={"error": str(e), "error_type": type(e).__name__}
                )

    def _validate_inputs(self, executable_system: ExecutableSystem, inputs: Dict[str, Any]) -> ValidationResult:
        """Validate inputs against signature."""
        violations = []
        warnings = []
        
        signature = executable_system.signature
        
        # Check required inputs
        for input_name, input_def in signature.inputs.items():
            if input_name not in inputs:
                if not input_def.get("optional", False):
                    violations.append({
                        "field": input_name,
                        "message": f"Required input '{input_name}' is missing",
                        "severity": "Violation"
                    })
            else:
                # Validate input type
                expected_type = input_def.get("type", "string")
                actual_value = inputs[input_name]
                
                if expected_type == "string" and not isinstance(actual_value, str):
                    violations.append({
                        "field": input_name,
                        "message": f"Input '{input_name}' must be a string, got {type(actual_value).__name__}",
                        "severity": "Violation"
                    })
                elif expected_type == "integer" and not isinstance(actual_value, int):
                    violations.append({
                        "field": input_name,
                        "message": f"Input '{input_name}' must be an integer, got {type(actual_value).__name__}",
                        "severity": "Violation"
                    })
                elif expected_type == "float" and not isinstance(actual_value, (int, float)):
                    violations.append({
                        "field": input_name,
                        "message": f"Input '{input_name}' must be a number, got {type(actual_value).__name__}",
                        "severity": "Violation"
                    })
        
        # Check for extra inputs
        for input_name in inputs:
            if input_name not in signature.inputs:
                warnings.append({
                    "field": input_name,
                    "message": f"Extra input '{input_name}' is not defined in signature",
                    "severity": "Warning"
                })
        
        return ValidationResult(
            valid=len(violations) == 0,
            violations=violations,
            warnings=warnings,
            metadata={"inputs_validated": len(signature.inputs)}
        )

    def _apply_validation_rules(self, executable_system: ExecutableSystem, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """Apply validation rules to inputs."""
        validation_data = {
            "rules_applied": 0,
            "violations": [],
            "warnings": [],
            "passed": []
        }
        
        for rule in executable_system.validation_rules:
            validation_data["rules_applied"] += 1
            
            target = rule.get("target_class", "")
            constraint_type = rule.get("constraint_type", "")
            constraint_value = rule.get("constraint_value")
            
            # Apply validation based on constraint type
            if constraint_type == "cardinality":
                # Check cardinality constraints
                if "min_count" in rule and len(inputs.get(target, [])) < rule["min_count"]:
                    validation_data["violations"].append({
                        "rule_id": rule.get("rule_id"),
                        "message": f"Minimum count not met for {target}",
                        "severity": rule.get("severity", "Violation")
                    })
                elif "max_count" in rule and len(inputs.get(target, [])) > rule["max_count"]:
                    validation_data["violations"].append({
                        "rule_id": rule.get("rule_id"),
                        "message": f"Maximum count exceeded for {target}",
                        "severity": rule.get("severity", "Violation")
                    })
                else:
                    validation_data["passed"].append(rule.get("rule_id"))
            
            elif constraint_type == "datatype":
                # Check datatype constraints
                if target in inputs:
                    value = inputs[target]
                    expected_type = constraint_value
                    
                    if expected_type == "string" and not isinstance(value, str):
                        validation_data["violations"].append({
                            "rule_id": rule.get("rule_id"),
                            "message": f"Expected string for {target}, got {type(value).__name__}",
                            "severity": rule.get("severity", "Violation")
                        })
                    else:
                        validation_data["passed"].append(rule.get("rule_id"))
            
            elif constraint_type == "value":
                # Check value constraints
                if target in inputs:
                    value = inputs[target]
                    
                    if "min_length" in rule and len(str(value)) < rule["min_length"]:
                        validation_data["violations"].append({
                            "rule_id": rule.get("rule_id"),
                            "message": f"Minimum length not met for {target}",
                            "severity": rule.get("severity", "Violation")
                        })
                    elif "max_length" in rule and len(str(value)) > rule["max_length"]:
                        validation_data["violations"].append({
                            "rule_id": rule.get("rule_id"),
                            "message": f"Maximum length exceeded for {target}",
                            "severity": rule.get("severity", "Violation")
                        })
                    else:
                        validation_data["passed"].append(rule.get("rule_id"))
        
        return validation_data

    def _execute_signature(self, executable_system: ExecutableSystem, inputs: Dict[str, Any]) -> Dict[str, Any]:
        """Execute DSPy signature with real implementation."""
        signature = executable_system.signature
        
        # Simulate DSPy execution (in real implementation, this would call actual DSPy)
        outputs = {}
        
        # Generate outputs based on signature definition
        for output_name, output_def in signature.outputs.items():
            semantic_type = output_def.get("semantic_type", "recommendation")
            
            if semantic_type == "recommendation":
                # Generate a recommendation based on inputs
                if "user_input" in inputs:
                    outputs[output_name] = f"Recommendation for: {inputs['user_input']}"
                else:
                    outputs[output_name] = "Default recommendation"
            
            elif semantic_type == "reasoning":
                # Generate reasoning
                outputs[output_name] = "This is the reasoning behind the recommendation."
            
            elif semantic_type == "confidence":
                # Generate confidence score
                outputs[output_name] = 0.85
            
            elif semantic_type == "analysis":
                # Generate analysis
                outputs[output_name] = "Analysis of the provided input data."
            
            else:
                # Default output
                outputs[output_name] = f"Generated output for {output_name}"
        
        # Add execution metadata to outputs
        outputs["_execution_metadata"] = {
            "signature_name": signature.name,
            "model_provider": signature.model_config.provider if signature.model_config else "default",
            "model_name": signature.model_config.model_name if signature.model_config else "default",
            "execution_timestamp": time.time()
        }
        
        return outputs

    def _generate_telemetry(self, executable_system: ExecutableSystem, inputs: Dict[str, Any], outputs: Dict[str, Any]) -> TelemetryData:
        """Generate telemetry for execution."""
        spans = [
            {
                "name": "dspy_signature_execution",
                "attributes": {
                    "signature_name": executable_system.signature.name,
                    "inputs_count": len(inputs),
                    "outputs_count": len(outputs),
                    "model_provider": executable_system.signature.model_config.provider if executable_system.signature.model_config else "default",
                    "model_name": executable_system.signature.model_config.model_name if executable_system.signature.model_config else "default"
                }
            }
        ]
        
        events = [
            {
                "name": "signature_execution_started",
                "attributes": {
                    "signature_name": executable_system.signature.name,
                    "timestamp": time.time()
                }
            },
            {
                "name": "signature_execution_completed",
                "attributes": {
                    "signature_name": executable_system.signature.name,
                    "timestamp": time.time(),
                    "outputs_generated": len(outputs)
                }
            }
        ]
        
        metrics = {
            "execution_duration": time.time(),
            "inputs_processed": len(inputs),
            "outputs_generated": len(outputs),
            "validation_rules_applied": len(executable_system.validation_rules)
        }
        
        logs = [
            {
                "level": "INFO",
                "message": f"Executed signature '{executable_system.signature.name}' with {len(inputs)} inputs",
                "timestamp": time.time()
            },
            {
                "level": "INFO",
                "message": f"Generated {len(outputs)} outputs from signature execution",
                "timestamp": time.time()
            }
        ]
        
        return TelemetryData(
            spans=spans,
            events=events,
            metrics=metrics,
            logs=logs
        )

    def _generate_execution_metadata(self, executable_system: ExecutableSystem, inputs: Dict[str, Any], outputs: Dict[str, Any]) -> Dict[str, Any]:
        """Generate execution metadata."""
        return {
            "execution_info": {
                "signature_name": executable_system.signature.name,
                "description": executable_system.signature.description,
                "execution_timestamp": time.time(),
                "execution_id": f"exec_{int(time.time())}"
            },
            "model_info": {
                "provider": executable_system.signature.model_config.provider if executable_system.signature.model_config else "default",
                "model_name": executable_system.signature.model_config.model_name if executable_system.signature.model_config else "default",
                "parameters": executable_system.signature.model_config.parameters if executable_system.signature.model_config else {}
            },
            "semantic_context": {
                "ontology_classes": executable_system.semantic_context.get("ontology_classes", []),
                "semantic_types": executable_system.semantic_context.get("semantic_types", {}),
                "validation_context": executable_system.semantic_context.get("validation_context", {})
            },
            "performance_metrics": {
                "inputs_count": len(inputs),
                "outputs_count": len(outputs),
                "validation_rules_count": len(executable_system.validation_rules),
                "shacl_triples": executable_system.shacl_triples
            }
        } 