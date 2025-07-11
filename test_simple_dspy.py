#!/usr/bin/env python3
"""
Simple Test for BPMN + DSPy Integration
Demonstrates the two technologies working together
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import dspy_registry

def test_simple_dspy_integration():
    """Test the simple BPMN + DSPy integration"""
    print("🚀 Testing Simple BPMN + DSPy Integration")
    print("=" * 45)
    
    # Set up initial context
    initial_context = {
        "input_data": "Sample data for analysis"
    }
    
    print(f"📋 Initial context: {json.dumps(initial_context, indent=2)}")
    
    # Run the workflow
    bpmn_path = "bpmn/simple_dspy_workflow.bpmn"
    process_id = "SimpleDspyWorkflow"
    
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

def test_dspy_parsing():
    """Test DSPy parsing separately"""
    print("\n🧪 Testing DSPy Parsing")
    print("=" * 25)
    
    # Create parser and load BPMN file
    parser = DspyBpmnParser()
    bpmn_path = "bpmn/simple_dspy_workflow.bpmn"
    parser.add_bpmn_file(bpmn_path)
    
    # Check process specs
    specs = parser.find_all_specs()
    print(f"📋 Found {len(specs)} process specs")
    
    for process_id, spec in specs.items():
        print(f"\n🔍 Process: {process_id}")
        for task_id, task_spec in spec.task_specs.items():
            print(f"   Task: {task_spec.name} ({task_spec.__class__.__name__})")

def test_dspy_signatures():
    """Test DSPy signatures separately"""
    print("\n🧠 Testing DSPy Signatures")
    print("=" * 30)
    
    # Get parser with signatures
    parser = DspyBpmnParser()
    bpmn_path = "bpmn/simple_dspy_workflow.bpmn"
    parser.add_bpmn_file(bpmn_path)
    
    # Register dynamic signatures
    dspy_registry.register_parser_signatures(parser.dynamic_signatures)
    
    # Test the analyze_data signature
    print("\n🧪 Testing analyze_data signature:")
    try:
        result = dspy_registry.call_signature("analyze_data", data="Test data for analysis")
        print(f"   ✅ Result: {result}")
    except Exception as e:
        print(f"   ❌ Error: {e}")

if __name__ == "__main__":
    try:
        # Test DSPy parsing
        test_dspy_parsing()
        
        # Test DSPy signatures
        test_dspy_signatures()
        
        # Test complete integration
        test_simple_dspy_integration()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc() 