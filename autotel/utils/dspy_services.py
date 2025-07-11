"""
Generalized DSPy Services for BPMN Integration
Allows calling any DSPy signature from BPMN XML using Service Tasks
Supports dynamic signature creation from XML definitions
"""

import json
import dspy
from typing import Dict, Any, Optional, Type
from dataclasses import dataclass

# Configure DSPy globally
ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.7)
dspy.configure(lm=ollama_lm)

@dataclass
class DSPySignature:
    """Registry entry for a DSPy signature"""
    signature_class: Type[dspy.Signature]
    description: str
    input_fields: Dict[str, str]  # field_name -> description
    output_fields: Dict[str, str]  # field_name -> description

class DSPyServiceRegistry:
    """Registry for DSPy signatures that can be called from BPMN"""
    
    def __init__(self):
        self._signatures: Dict[str, DSPySignature] = {}
        self._predict_modules: Dict[str, dspy.Predict] = {}
        self._parser_signatures: Dict[str, Type[dspy.Signature]] = {}
    
    def register_signature(self, name: str, signature_class: Type[dspy.Signature], 
                          description: str = ""):
        """Register a DSPy signature for use in BPMN"""
        # Extract field information
        input_fields = {}
        output_fields = {}
        
        # Get all attributes of the class
        for attr_name in dir(signature_class):
            if not attr_name.startswith('_'):
                attr_value = getattr(signature_class, attr_name)
                if hasattr(attr_value, '__class__'):
                    tname = attr_value.__class__.__name__
                    if tname == 'InputField':
                        input_fields[attr_name] = getattr(attr_value, 'desc', attr_name)
                    elif tname == 'OutputField':
                        output_fields[attr_name] = getattr(attr_value, 'desc', attr_name)
        
        # Create registry entry
        signature_info = DSPySignature(
            signature_class=signature_class,
            description=description,
            input_fields=input_fields,
            output_fields=output_fields
        )
        self._signatures[name] = signature_info
        self._predict_modules[name] = dspy.Predict(signature_class)
        print(f"✅ Registered DSPy signature: {name}")
        print(f"   Inputs: {list(input_fields.keys())}")
        print(f"   Outputs: {list(output_fields.keys())}")
    
    def register_parser_signatures(self, parser_signatures: Dict[str, Type[dspy.Signature]]):
        """Register dynamic signatures from the BPMN parser"""
        for name, signature_class in parser_signatures.items():
            self._parser_signatures[name] = signature_class
            self._predict_modules[name] = dspy.Predict(signature_class)
            print(f"✅ Registered parser signature: {name}")
    
    def call_signature(self, signature_name: str, **kwargs) -> Dict[str, Any]:
        """Call a registered DSPy signature with the given parameters"""
        # Check if it's a parser signature first
        if signature_name in self._parser_signatures:
            try:
                # Call the dynamic DSPy signature
                result = self._predict_modules[signature_name](**kwargs)
                # Extract from _store if present
                if hasattr(result, '_store') and isinstance(result._store, dict) and result._store:
                    output_dict = dict(result._store)
                else:
                    # Fallback: return all public fields
                    output_dict = {k: v for k, v in vars(result).items() if not k.startswith('_')}
                return output_dict
            except Exception as e:
                raise Exception(f"Error calling dynamic DSPy signature '{signature_name}': {str(e)}")
        
        # Fall back to regular signatures
        if signature_name not in self._signatures:
            raise ValueError(f"Unknown DSPy signature: {signature_name}")
        try:
            # Call the DSPy signature
            result = self._predict_modules[signature_name](**kwargs)
            # Extract from _store if present
            if hasattr(result, '_store') and isinstance(result._store, dict) and result._store:
                output_dict = dict(result._store)
            else:
                # Fallback: return all public fields
                output_dict = {k: v for k, v in vars(result).items() if not k.startswith('_')}
            return output_dict
        except Exception as e:
            raise Exception(f"Error calling DSPy signature '{signature_name}': {str(e)}")
    
    def get_signature_info(self, signature_name: str) -> Optional[DSPySignature]:
        """Get information about a registered signature"""
        return self._signatures.get(signature_name)
    
    def list_signatures(self) -> Dict[str, DSPySignature]:
        """List all registered signatures"""
        return self._signatures.copy()
    
    def list_parser_signatures(self) -> Dict[str, Type[dspy.Signature]]:
        """List all parser signatures"""
        return self._parser_signatures.copy()

# Global registry instance
dspy_registry = DSPyServiceRegistry()

# Pre-defined DSPy signatures (fallback for backward compatibility)
class OTELInterpreter(dspy.Signature):
    """Interpret OpenTelemetry spans and provide insights"""
    otel_spans = dspy.InputField(desc="OpenTelemetry span data in JSON format")
    interpretation = dspy.OutputField(desc="Human-friendly interpretation of the OTEL spans")

class ProcessAnalyzer(dspy.Signature):
    """Analyze BPMN process execution and provide recommendations"""
    process_data = dspy.InputField(desc="BPMN process execution data in JSON format")
    analysis = dspy.OutputField(desc="Process analysis and recommendations")
    performance_score = dspy.OutputField(desc="Performance score from 0-100")

class ErrorDiagnoser(dspy.Signature):
    """Diagnose errors in workflow execution"""
    error_data = dspy.InputField(desc="Error information in JSON format")
    diagnosis = dspy.OutputField(desc="Error diagnosis and suggested fixes")
    severity = dspy.OutputField(desc="Error severity level (LOW, MEDIUM, HIGH, CRITICAL)")

class WorkflowOptimizer(dspy.Signature):
    """Suggest optimizations for workflow performance"""
    workflow_data = dspy.InputField(desc="Workflow execution data in JSON format")
    optimizations = dspy.OutputField(desc="List of suggested optimizations")
    expected_improvement = dspy.OutputField(desc="Expected performance improvement percentage")

# Register the fallback signatures
dspy_registry.register_signature(
    "interpret_otel_spans", 
    OTELInterpreter,
    "Interpret OpenTelemetry spans and provide human-friendly insights"
)

dspy_registry.register_signature(
    "analyze_process", 
    ProcessAnalyzer,
    "Analyze BPMN process execution and provide recommendations"
)

dspy_registry.register_signature(
    "diagnose_error", 
    ErrorDiagnoser,
    "Diagnose errors in workflow execution and suggest fixes"
)

dspy_registry.register_signature(
    "optimize_workflow", 
    WorkflowOptimizer,
    "Suggest optimizations for workflow performance"
)

# Generalized function for BPMN integration
def call_dspy_service(operation_name: str, **operation_params) -> str:
    """
    Generalized function to call any DSPy signature from BPMN.
    This is the main entry point for Service Tasks.
    
    Args:
        operation_name: Name of the registered DSPy signature
        **operation_params: Parameters to pass to the signature
    
    Returns:
        JSON string containing the results
    """
    try:
        # Call the DSPy signature
        result = dspy_registry.call_signature(operation_name, **operation_params)
        
        # Return as JSON string for BPMN compatibility
        return json.dumps(result, indent=2)
        
    except Exception as e:
        # Return error information as JSON
        error_result = {
            "error": True,
            "message": str(e),
            "operation": operation_name
        }
        return json.dumps(error_result, indent=2)

# Convenience functions for specific use cases
def interpret_otel_spans(otel_spans_json: str) -> str:
    """Convenience function for OTEL span interpretation"""
    return call_dspy_service("interpret_otel_spans", otel_spans=otel_spans_json)

def analyze_process(process_data_json: str) -> str:
    """Convenience function for process analysis"""
    return call_dspy_service("analyze_process", process_data=process_data_json)

def diagnose_error(error_data_json: str) -> str:
    """Convenience function for error diagnosis"""
    return call_dspy_service("diagnose_error", error_data=error_data_json)

def optimize_workflow(workflow_data_json: str) -> str:
    """Convenience function for workflow optimization"""
    return call_dspy_service("optimize_workflow", workflow_data=workflow_data_json)

def perform_work(work_type: str, iteration: str, interpretation: str = None) -> str:
    """Convenience function for performing work"""
    params = {"work_type": work_type, "iteration": iteration}
    if interpretation:
        params["interpretation"] = interpretation
    return call_dspy_service("perform_work", **params)

def interpret_workflow_results(work_result: str, execution_history: str, current_iteration: str) -> str:
    """Convenience function for interpreting workflow results"""
    return call_dspy_service("interpret_workflow_results", 
                           work_result=work_result, 
                           execution_history=execution_history, 
                           current_iteration=current_iteration)

def generate_workflow_summary(work_result: str, interpretation: str, execution_history: str, current_iteration: str) -> str:
    """Convenience function for generating workflow summary"""
    return call_dspy_service("generate_workflow_summary", 
                           work_result=work_result, 
                           interpretation=interpretation, 
                           execution_history=execution_history, 
                           current_iteration=current_iteration)

# Export dspy_service for workflow integration
dspy_service = call_dspy_service 