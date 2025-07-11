#!/usr/bin/env python3
"""
Test CDATA validation: verify that DspyBpmnParser rejects BPMN files with CDATA sections.
"""
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from SpiffWorkflow.bpmn.parser.ValidationException import ValidationException

def test_cdata_rejection():
    """Test that CDATA sections are rejected"""
    
    # BPMN with CDATA (should be rejected)
    bpmn_with_cdata = '''<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="TestProcess" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start"/>
    <bpmn:scriptTask id="ScriptTask_1" name="Script">
      <bpmn:script><![CDATA[
        // This should be rejected
        var x = 1;
      ]]></bpmn:script>
    </bpmn:scriptTask>
    <bpmn:endEvent id="EndEvent_1" name="End"/>
  </bpmn:process>
</bpmn:definitions>'''
    
    parser = DspyBpmnParser()
    
    try:
        parser.add_bpmn_str(bpmn_with_cdata, "test_with_cdata.bpmn")
        print("❌ FAILED: CDATA was not rejected")
        return False
    except ValidationException as e:
        if "CDATA sections are not allowed" in str(e):
            print("✅ PASSED: CDATA sections are properly rejected")
            return True
        else:
            print(f"❌ FAILED: Unexpected validation error: {e}")
            return False
    except Exception as e:
        print(f"❌ FAILED: Unexpected error: {e}")
        return False

def test_valid_bpmn():
    """Test that valid BPMN without CDATA is accepted"""
    
    # Valid BPMN without CDATA (should be accepted)
    valid_bpmn = '''<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="TestProcess" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start"/>
    <bpmn:serviceTask id="ServiceTask_1" name="Service">
      <bpmn:extensionElements>
        <dspy:service name="interpret_otel_spans" result="result">
          <dspy:param name="otel_spans" value="otel_spans_json"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    <bpmn:endEvent id="EndEvent_1" name="End"/>
  </bpmn:process>
</bpmn:definitions>'''
    
    parser = DspyBpmnParser()
    
    try:
        parser.add_bpmn_str(valid_bpmn, "test_valid.bpmn")
        print("✅ PASSED: Valid BPMN without CDATA is accepted")
        return True
    except Exception as e:
        print(f"❌ FAILED: Valid BPMN was rejected: {e}")
        return False

if __name__ == "__main__":
    print("Testing CDATA validation...")
    test1 = test_cdata_rejection()
    test2 = test_valid_bpmn()
    
    if test1 and test2:
        print("\n🎉 All tests passed! CDATA validation is working correctly.")
    else:
        print("\n💥 Some tests failed!") 