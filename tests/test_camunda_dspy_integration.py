#!/usr/bin/env python3
"""
Test DSPy BPMN Parser with Camunda Integration
Verifies that the DspyBpmnParser works properly with camunda-style workflows.
"""

import os
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.workflows.autotel_camunda import AutoTelCamundaEngine, create_autotel_camunda_engine
from autotel.core.telemetry import TelemetryManager
from autotel.schemas.validation import SchemaValidator
from autotel.utils.dspy_services import dspy_registry


def create_test_bpmn_with_dspy():
    """Create a test BPMN file with DSPy service task"""
    bpmn_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  xmlns:dspy="http://autotel.ai/dspy"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <dspy:signatures>
    <dspy:signature name="TestSignature" description="A test DSPy signature">
      <dspy:input name="input_text" description="Input text to process" optional="false"/>
      <dspy:output name="result" description="Processed result"/>
    </dspy:signature>
  </dspy:signatures>

  <bpmn:process id="Process_1" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="DSPy Service Task">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <bpmn:extensionElements>
        <dspy:service name="test_service" result="service_result">
          <dspy:param name="text" value="input_text"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="EndEvent_1" />
  </bpmn:process>
</bpmn:definitions>'''
    
    return bpmn_xml


def create_test_dspy_service():
    """Create a test DSPy service using the registry"""
    import dspy
    
    # Create a test signature class
    class TestService(dspy.Signature):
        """A test DSPy service"""
        text = dspy.InputField(desc="Input text to process")
        result = dspy.OutputField(desc="Processed result")
    
    # Register the signature using the correct method
    dspy_registry.register_dynamic_signature(
        name="test_service",
        input_fields={"text": "Input text to process"},
        output_fields={"result": "Processed result"},
        description="A test DSPy service"
    )
    
    return TestService


def test_camunda_dspy_integration():
    """Test that DSPy BPMN parser works with camunda integration"""
    print("🧪 Testing DSPy BPMN Parser with Camunda Integration")
    
    # Create test DSPy service
    test_service = create_test_dspy_service()
    print("✅ Created test DSPy service")
    
    # Create test BPMN file
    bpmn_xml = create_test_bpmn_with_dspy()
    print("✅ Created test BPMN file with DSPy service task")
    
    # Create temporary file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(bpmn_xml)
        bpmn_path = f.name
    
    try:
        # Create AutoTel Camunda engine
        telemetry_manager = TelemetryManager()
        schema_validator = SchemaValidator()
        engine = create_autotel_camunda_engine(telemetry_manager, schema_validator)
        print("✅ Created AutoTel Camunda engine")
        
        # Add BPMN file
        engine.add_bpmn_file(bpmn_path)
        print("✅ Added BPMN file to engine")
        
        # Check that signatures were parsed
        if hasattr(engine.parser, 'signature_definitions'):
            signatures = engine.parser.signature_definitions
            print(f"✅ Parsed {len(signatures)} DSPy signatures")
            for name, sig_def in signatures.items():
                print(f"   - {name}: {sig_def['description']}")
        
        # Check registered signatures in registry
        registered_signatures = dspy_registry.list_signatures()
        print(f"✅ Registry has {len(registered_signatures)} signatures")
        for name, sig_info in registered_signatures.items():
            print(f"   - {name}: {sig_info.description}")
        
        # Create workflow
        initial_data = {"input_text": "Hello, DSPy!"}
        workflow = engine.create_workflow("Process_1", initial_data)
        print("✅ Created workflow with initial data")
        
        # Execute workflow
        result = engine.execute_workflow(workflow, run_until_user_input=False)
        print("✅ Executed workflow")
        
        # Check results
        if "service_result" in result:
            print(f"✅ DSPy service executed successfully: {result['service_result']}")
        else:
            print("❌ DSPy service result not found in workflow data")
            print(f"Available data: {list(result.keys())}")
        
        # List processes
        processes = engine.list_processes()
        print(f"✅ Available processes: {list(processes.keys())}")
        
        print("🎉 All tests passed! DSPy BPMN parser works with camunda integration.")
        return True
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    finally:
        # Clean up temporary file
        if os.path.exists(bpmn_path):
            os.unlink(bpmn_path)


def test_cdata_validation():
    """Test that CDATA validation works in camunda context"""
    print("\n🧪 Testing CDATA validation in Camunda context")
    
    # Create BPMN with CDATA (should be rejected)
    bpmn_with_cdata = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="Process_1" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_1</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="EndEvent_1" />
    <bpmn:documentation><![CDATA[This should be rejected]]></bpmn:documentation>
  </bpmn:process>
</bpmn:definitions>'''
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
        f.write(bpmn_with_cdata)
        bpmn_path = f.name
    
    try:
        telemetry_manager = TelemetryManager()
        schema_validator = SchemaValidator()
        engine = create_autotel_camunda_engine(telemetry_manager, schema_validator)
        
        try:
            engine.add_bpmn_file(bpmn_path)
            print("❌ CDATA validation failed - should have rejected the file")
            return False
        except Exception as e:
            if "CDATA" in str(e):
                print("✅ CDATA validation working correctly")
                return True
            else:
                print(f"❌ Unexpected error: {e}")
                return False
                
    finally:
        if os.path.exists(bpmn_path):
            os.unlink(bpmn_path)


def test_dspy_registry_integration():
    """Test DSPy registry integration directly"""
    print("\n🧪 Testing DSPy Registry Integration")
    
    # Test direct registry usage
    dspy_registry.register_dynamic_signature(
        name="direct_test",
        input_fields={"message": "Input message"},
        output_fields={"response": "Response message"},
        description="Direct registry test"
    )
    
    # Test calling the signature
    try:
        result = dspy_registry.call_signature("direct_test", message="Hello from test!")
        print(f"✅ Direct registry call successful: {result}")
        return True
    except Exception as e:
        print(f"❌ Direct registry call failed: {e}")
        return False


if __name__ == "__main__":
    print("🚀 Testing DSPy BPMN Parser with Camunda Integration")
    print("=" * 60)
    
    # Test 1: Basic integration
    success1 = test_camunda_dspy_integration()
    
    # Test 2: CDATA validation
    success2 = test_cdata_validation()
    
    # Test 3: Direct registry integration
    success3 = test_dspy_registry_integration()
    
    print("\n" + "=" * 60)
    if success1 and success2 and success3:
        print("🎉 All tests passed! DSPy BPMN parser works correctly with camunda integration.")
        sys.exit(0)
    else:
        print("❌ Some tests failed. Check the output above for details.")
        sys.exit(1) 