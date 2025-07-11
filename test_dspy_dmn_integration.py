#!/usr/bin/env python3
"""
Test for DSPy + DMN Integration using Camunda-style structure
Demonstrates BPMN + DSPy + External DMN working together
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import dspy_registry

def test_dspy_dmn_integration():
    """Test the DSPy + DMN integration using Camunda-style structure"""
    print("🚀 Testing DSPy + DMN Integration (Camunda-style)")
    print("=" * 55)
    
    # Set up initial context
    initial_context = {
        "input_data": "Sample data for analysis"
    }
    
    print(f"📋 Initial context: {json.dumps(initial_context, indent=2)}")
    
    # Run the workflow
    bpmn_path = "bpmn/dspy_dmn_workflow.bpmn"
    process_id = "DspyDmnWorkflow"
    
    print(f"🎯 Running process: {process_id}")
    print(f"📄 BPMN file: {bpmn_path}")
    print(f"📄 DMN file: bpmn/quality_decision.dmn")
    
    try:
        # Execute the workflow with DMN file
        result = run_dspy_bpmn_process(bpmn_path, process_id, initial_context, dmn_files=["bpmn/quality_decision.dmn"])
        
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

def test_dmn_parsing():
    """Test DMN parsing separately"""
    print("\n🧪 Testing DMN Parsing")
    print("=" * 25)
    
    # Create parser and load BPMN file
    parser = DspyBpmnParser()
    bpmn_path = "bpmn/dspy_dmn_workflow.bpmn"
    dmn_path = "bpmn/quality_decision.dmn"
    
    # Load DMN file first
    parser.add_dmn_file(dmn_path)
    print(f"✅ Loaded DMN file: {dmn_path}")
    
    # Load BPMN file
    parser.add_bpmn_file(bpmn_path)
    print(f"✅ Loaded BPMN file: {bpmn_path}")
    
    # Check process specs for DMN decisions
    specs = parser.find_all_specs()
    print(f"📋 Found {len(specs)} process specs")
    
    for process_id, spec in specs.items():
        print(f"\n🔍 Process: {process_id}")
        for task_id, task_spec in spec.task_specs.items():
            if hasattr(task_spec, 'decision_table'):
                print(f"   DMN Task: {task_spec.name}")
                print(f"   Decision Table: {task_spec.decision_table}")

def test_dmn_execution():
    """Test DMN execution separately"""
    print("\n⚡ Testing DMN Execution")
    print("=" * 30)
    
    # Test different scenarios using the workflow
    test_cases = [
        {
            "name": "High Quality - Should Proceed",
            "context": {"input_data": "High quality data", "quality_score": 0.9}
        },
        {
            "name": "Low Quality - Should Stop", 
            "context": {"input_data": "Low quality data", "quality_score": 0.5}
        }
    ]
    
    for test_case in test_cases:
        print(f"\n🧪 {test_case['name']}")
        print(f"   Context: {test_case['context']}")
        
        try:
            # Run a mini workflow to test DMN execution
            from SpiffWorkflow import Workflow
            from SpiffWorkflow.util.task import TaskState
            
            parser = DspyBpmnParser()
            bpmn_path = "bpmn/dspy_dmn_workflow.bpmn"
            dmn_path = "bpmn/quality_decision.dmn"
            
            # Load DMN file first
            parser.add_dmn_file(dmn_path)
            
            # Load BPMN file
            parser.add_bpmn_file(bpmn_path)
            
            specs = parser.find_all_specs()
            spec = specs["DspyDmnWorkflow"]
            
            wf = Workflow(spec)
            wf.set_data(**test_case['context'])
            
            # Execute just the DMN task
            ready_tasks = wf.get_tasks(state=TaskState.READY)
            for task in ready_tasks:
                if hasattr(task.task_spec, 'decision_table'):
                    print(f"   Executing DMN task: {task.task_spec.name}")
                    decision_result = task.task_spec.decision_table.evaluate(wf.data)
                    print(f"   ✅ DMN Result: {decision_result}")
                    break
                    
        except Exception as e:
            print(f"   ❌ Error: {e}")

def test_dspy_signatures():
    """Test DSPy signatures separately"""
    print("\n🧠 Testing DSPy Signatures")
    print("=" * 30)
    
    # Get parser with signatures
    parser = DspyBpmnParser()
    bpmn_path = "bpmn/dspy_dmn_workflow.bpmn"
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
        # Test DMN parsing
        test_dmn_parsing()
        
        # Test DMN execution
        test_dmn_execution()
        
        # Test DSPy signatures
        test_dspy_signatures()
        
        # Test complete integration
        test_dspy_dmn_integration()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc() 