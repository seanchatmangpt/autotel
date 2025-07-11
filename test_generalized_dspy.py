#!/usr/bin/env python3
"""
Test script for generalized DSPy services
Demonstrates how to call any DSPy signature from BPMN XML
"""

import json
from autotel.utils.dspy_services import (
    dspy_registry, 
    call_dspy_service,
    interpret_otel_spans,
    analyze_process,
    diagnose_error,
    optimize_workflow
)

def test_generalized_dspy():
    """Test the generalized DSPy system"""
    
    print("🚀 Testing Generalized DSPy Services for BPMN Integration")
    print("=" * 70)
    
    # Show registered signatures
    print("\n📋 Registered DSPy Signatures:")
    print("-" * 40)
    signatures = dspy_registry.list_signatures()
    for name, sig_info in signatures.items():
        print(f"✅ {name}: {sig_info.description}")
        print(f"   Inputs: {list(sig_info.input_fields.keys())}")
        print(f"   Outputs: {list(sig_info.output_fields.keys())}")
        print()
    
    # Test 1: OTEL Span Interpretation
    print("\n🔍 Test 1: OTEL Span Interpretation")
    print("-" * 40)
    
    sample_otel_spans = {
        "resource": {"attributes": {"service.name": "autotel-service"}},
        "spans": [
            {
                "name": "execute_process",
                "status": {"code": "ERROR", "message": "Process failed"},
                "events": [{"name": "exception", "attributes": {"exception.type": "AttributeError"}}]
            }
        ]
    }
    
    result1 = interpret_otel_spans(json.dumps(sample_otel_spans))
    print("Result:")
    print(json.loads(result1)["interpretation"][:200] + "...")
    
    # Test 2: Process Analysis
    print("\n📊 Test 2: Process Analysis")
    print("-" * 40)
    
    sample_process_data = {
        "process_id": "order_process",
        "execution_time": 45.2,
        "steps_completed": 8,
        "errors": 1,
        "variables": {"customer_id": "123", "amount": 100.50}
    }
    
    result2 = analyze_process(json.dumps(sample_process_data))
    parsed_result2 = json.loads(result2)
    print("Analysis:", parsed_result2["analysis"][:150] + "...")
    print("Performance Score:", parsed_result2["performance_score"])
    
    # Test 3: Error Diagnosis
    print("\n🚨 Test 3: Error Diagnosis")
    print("-" * 40)
    
    sample_error_data = {
        "error_type": "AttributeError",
        "message": "'int' object has no attribute 'workflow'",
        "stack_trace": "File 'orchestrator.py', line 45",
        "context": "Process execution"
    }
    
    result3 = diagnose_error(json.dumps(sample_error_data))
    parsed_result3 = json.loads(result3)
    print("Diagnosis:", parsed_result3["diagnosis"][:150] + "...")
    print("Severity:", parsed_result3["severity"])
    
    # Test 4: Workflow Optimization
    print("\n⚡ Test 4: Workflow Optimization")
    print("-" * 40)
    
    sample_workflow_data = {
        "workflow_id": "complex_order_process",
        "average_execution_time": 120.5,
        "bottlenecks": ["payment_validation", "inventory_check"],
        "resource_usage": {"cpu": 85, "memory": 70}
    }
    
    result4 = optimize_workflow(json.dumps(sample_workflow_data))
    parsed_result4 = json.loads(result4)
    print("Optimizations:", parsed_result4["optimizations"][:150] + "...")
    print("Expected Improvement:", parsed_result4["expected_improvement"])
    
    # Test 5: Generic call_dspy_service function
    print("\n🔧 Test 5: Generic call_dspy_service Function")
    print("-" * 40)
    
    # This is how it would be called from BPMN Service Task
    generic_result = call_dspy_service(
        operation_name="interpret_otel_spans",
        otel_spans=json.dumps(sample_otel_spans)
    )
    
    print("Generic call result:")
    print(json.loads(generic_result)["interpretation"][:100] + "...")
    
    print("\n✅ All tests completed successfully!")
    print("\n📝 BPMN XML Usage Examples:")
    print("-" * 40)
    print("""
<!-- OTEL Interpretation Service Task -->
<bpmn:serviceTask id="InterpretOtel" name="Interpret OTEL Spans">
  <bpmn:extensionElements>
    <spiffworkflow:serviceTaskOperator id="interpret_otel_spans" resultVariable="otel_interpretation">
      <spiffworkflow:parameters>
        <spiffworkflow:parameter id="otel_spans" type="str" value="otel_spans_json"/>
      </spiffworkflow:parameters>
    </spiffworkflow:serviceTaskOperator>
  </bpmn:extensionElements>
</bpmn:serviceTask>

<!-- Process Analysis Service Task -->
<bpmn:serviceTask id="AnalyzeProcess" name="Analyze Process">
  <bpmn:extensionElements>
    <spiffworkflow:serviceTaskOperator id="analyze_process" resultVariable="process_analysis">
      <spiffworkflow:parameters>
        <spiffworkflow:parameter id="process_data" type="str" value="process_data_json"/>
      </spiffworkflow:parameters>
    </spiffworkflow:serviceTaskOperator>
  </bpmn:extensionElements>
</bpmn:serviceTask>

<!-- Error Diagnosis Service Task -->
<bpmn:serviceTask id="DiagnoseError" name="Diagnose Error">
  <bpmn:extensionElements>
    <spiffworkflow:serviceTaskOperator id="diagnose_error" resultVariable="error_diagnosis">
      <spiffworkflow:parameters>
        <spiffworkflow:parameter id="error_data" type="str" value="error_data_json"/>
      </spiffworkflow:parameters>
    </spiffworkflow:serviceTaskOperator>
  </bpmn:extensionElements>
</bpmn:serviceTask>

<!-- Workflow Optimization Service Task -->
<bpmn:serviceTask id="OptimizeWorkflow" name="Optimize Workflow">
  <bpmn:extensionElements>
    <spiffworkflow:serviceTaskOperator id="optimize_workflow" resultVariable="workflow_optimizations">
      <spiffworkflow:parameters>
        <spiffworkflow:parameter id="workflow_data" type="str" value="workflow_data_json"/>
      </spiffworkflow:parameters>
    </spiffworkflow:serviceTaskOperator>
  </bpmn:extensionElements>
</bpmn:serviceTask>
    """)

if __name__ == "__main__":
    test_generalized_dspy() 