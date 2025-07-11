#!/usr/bin/env python3
"""
Test XML-Defined DSPy Signatures
Demonstrates DSPy signatures defined directly in BPMN XML
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import dspy_registry

def test_xml_signature_parsing():
    """Test parsing DSPy signatures from XML"""
    print("🧪 Testing XML Signature Parsing")
    print("=" * 40)
    
    # Create parser and load BPMN file
    parser = DspyBpmnParser()
    bpmn_path = "bpmn/self_interpreting_workflow.bpmn"
    parser.add_bpmn_file(bpmn_path)
    
    # Check signature definitions
    signatures = parser.list_signatures()
    print(f"📋 Found {len(signatures)} signature definitions in XML:")
    
    for name, sig_def in signatures.items():
        print(f"\n🔍 Signature: {name}")
        print(f"   Description: {sig_def.description}")
        print(f"   Inputs: {list(sig_def.inputs.keys())}")
        print(f"   Outputs: {list(sig_def.outputs.keys())}")
        
        # Show input details
        for input_name, input_info in sig_def.inputs.items():
            optional = " (optional)" if input_info['optional'] else ""
            print(f"     Input '{input_name}': {input_info['description']}{optional}")
        
        # Show output details
        for output_name, output_desc in sig_def.outputs.items():
            print(f"     Output '{output_name}': {output_desc}")
    
    # Check dynamic signatures
    dynamic_sigs = parser.dynamic_signatures
    print(f"\n⚡ Created {len(dynamic_sigs)} dynamic DSPy signature classes:")
    for name in dynamic_sigs.keys():
        print(f"   ✅ {name}")
    
    return parser

def test_dynamic_signature_execution():
    """Test executing dynamic signatures created from XML"""
    print("\n🚀 Testing Dynamic Signature Execution")
    print("=" * 45)
    
    # Get parser with signatures
    parser = test_xml_signature_parsing()
    
    # Register dynamic signatures
    dspy_registry.register_parser_signatures(parser.dynamic_signatures)
    
    # Test each signature
    test_cases = [
        {
            "name": "perform_work",
            "params": {
                "work_type": "initial_work",
                "iteration": "1"
            }
        },
        {
            "name": "interpret_workflow_results",
            "params": {
                "work_result": json.dumps({"work_result": "Test work completed"}),
                "execution_history": json.dumps([]),
                "current_iteration": "1"
            }
        },
        {
            "name": "generate_workflow_summary",
            "params": {
                "work_result": json.dumps({"work_result": "Final work result"}),
                "interpretation": json.dumps({"should_continue": "false"}),
                "execution_history": json.dumps([{"iteration": 1}]),
                "current_iteration": "1"
            }
        }
    ]
    
    for test_case in test_cases:
        print(f"\n🧪 Testing signature: {test_case['name']}")
        try:
            result = dspy_registry.call_signature(test_case['name'], **test_case['params'])
            print(f"   ✅ Success: {result}")
        except Exception as e:
            print(f"   ❌ Error: {e}")

def test_xml_workflow_execution():
    """Test the complete workflow with XML-defined signatures"""
    print("\n🎯 Testing Complete XML Workflow")
    print("=" * 40)
    
    # Set up initial context
    initial_context = {
        "current_iteration": "1",
        "execution_history": json.dumps([]),
        "max_iterations": "3"
    }
    
    print(f"📋 Initial context: {json.dumps(initial_context, indent=2)}")
    
    # Run the workflow
    bpmn_path = "bpmn/self_interpreting_workflow.bpmn"
    process_id = "SelfInterpretingWorkflow"
    
    print(f"🎯 Running process: {process_id}")
    print(f"📄 BPMN file: {bpmn_path}")
    
    try:
        # Execute the workflow
        result = run_dspy_bpmn_process(bpmn_path, process_id, initial_context)
        
        print(f"\n✅ Workflow completed successfully!")
        print("=" * 50)
        print(f"📊 Final Results:")
        
        # Pretty print the results
        for key, value in result.items():
            if isinstance(value, str) and value.startswith('{'):
                try:
                    parsed_value = json.loads(value)
                    print(f"\n🔍 {key}:")
                    print(json.dumps(parsed_value, indent=2))
                except:
                    print(f"\n🔍 {key}: {value}")
            else:
                print(f"\n🔍 {key}: {value}")
        
    except Exception as e:
        print(f"❌ Error running workflow: {e}")
        import traceback
        traceback.print_exc()

def test_signature_registry():
    """Test the signature registry with XML signatures"""
    print("\n📋 Testing Signature Registry")
    print("=" * 30)
    
    # Get parser
    parser = test_xml_signature_parsing()
    
    # Register signatures
    dspy_registry.register_parser_signatures(parser.dynamic_signatures)
    
    # List all signatures
    print(f"\n📋 All registered signatures:")
    regular_sigs = dspy_registry.list_signatures()
    parser_sigs = dspy_registry.list_parser_signatures()
    
    print(f"   Regular signatures: {list(regular_sigs.keys())}")
    print(f"   Parser signatures: {list(parser_sigs.keys())}")
    
    # Test signature info
    for sig_name in parser_sigs.keys():
        sig_def = parser.get_signature_definition(sig_name)
        if sig_def:
            print(f"\n🔍 Signature '{sig_name}' definition:")
            print(f"   Description: {sig_def.description}")
            print(f"   Inputs: {list(sig_def.inputs.keys())}")
            print(f"   Outputs: {list(sig_def.outputs.keys())}")

if __name__ == "__main__":
    try:
        # Test XML signature parsing
        test_xml_signature_parsing()
        
        # Test dynamic signature execution
        test_dynamic_signature_execution()
        
        # Test signature registry
        test_signature_registry()
        
        # Test complete workflow
        test_xml_workflow_execution()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc() 