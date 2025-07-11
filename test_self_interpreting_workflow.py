#!/usr/bin/env python3
"""
Test Self-Interpreting BPMN Workflow
Demonstrates a BPMN workflow that uses DSPy to interpret its own results
and decide whether to continue running or complete.
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.core.orchestrator import Orchestrator
from autotel.utils.dspy_services import dspy_registry

def test_self_interpreting_workflow():
    """Test the self-interpreting workflow"""
    print("🚀 Testing Self-Interpreting BPMN Workflow")
    print("=" * 50)
    
    # Initialize the orchestrator
    orchestrator = Orchestrator(
        bpmn_files_path="bpmn",
        enable_telemetry=True,
        enable_persistence=False
    )
    
    # Set up initial variables for the workflow
    initial_variables = {
        "current_iteration": "1",
        "execution_history": json.dumps([]),
        "max_iterations": "5"  # Limit to prevent infinite loops
    }
    
    print(f"📋 Available process definitions: {list(orchestrator.process_definitions.keys())}")
    
    # Start the self-interpreting workflow
    process_id = "SelfInterpretingWorkflow"
    if process_id not in orchestrator.process_definitions:
        print(f"❌ Process '{process_id}' not found!")
        print(f"Available processes: {list(orchestrator.process_definitions.keys())}")
        return
    
    print(f"🎯 Starting process: {process_id}")
    instance = orchestrator.start_process(
        process_id=process_id,
        variables=initial_variables
    )
    
    print(f"📝 Process instance created: {instance.instance_id}")
    print(f"📊 Initial variables: {instance.variables}")
    
    # Execute the process with iteration tracking
    iteration_count = 0
    max_iterations = 10  # Safety limit
    
    while instance.status.value in ['pending', 'running'] and iteration_count < max_iterations:
        iteration_count += 1
        print(f"\n🔄 Execution Iteration {iteration_count}")
        print("-" * 30)
        
        # Execute the process
        instance = orchestrator.execute_process(instance.instance_id, max_steps=10)
        
        # Get current variables
        current_vars = orchestrator.get_process_variables(instance.instance_id)
        print(f"📊 Current variables: {json.dumps(current_vars, indent=2)}")
        
        # Check if we have interpretation results
        if 'interpretation' in current_vars:
            interpretation = json.loads(current_vars['interpretation'])
            print(f"🧠 Interpretation: {interpretation.get('reasoning', 'No reasoning provided')}")
            print(f"🎯 Should continue: {interpretation.get('should_continue', 'Unknown')}")
        
        # Check if we have work results
        if 'work_result' in current_vars:
            work_result = json.loads(current_vars['work_result'])
            print(f"💼 Work result: {work_result.get('work_result', 'No result')}")
        
        # Update iteration counter
        orchestrator.set_process_variables(instance.instance_id, {
            "current_iteration": str(iteration_count + 1)
        })
        
        # Check if process is completed
        if instance.status.value == 'completed':
            print(f"✅ Process completed after {iteration_count} iterations")
            break
        elif instance.status.value == 'failed':
            print(f"❌ Process failed after {iteration_count} iterations")
            break
    
    # Get final results
    final_vars = orchestrator.get_process_variables(instance.instance_id)
    print(f"\n🏁 Final Results:")
    print("=" * 50)
    
    if 'final_summary' in final_vars:
        final_summary = json.loads(final_vars['final_summary'])
        print(f"📋 Final Summary: {final_summary.get('final_summary', 'No summary')}")
        print(f"🔍 Key Insights: {final_summary.get('key_insights', 'No insights')}")
        print(f"📈 Performance Metrics: {final_summary.get('performance_metrics', 'No metrics')}")
    
    print(f"📊 All Variables: {json.dumps(final_vars, indent=2)}")
    
    # Get process statistics
    stats = orchestrator.get_process_statistics()
    print(f"\n📈 Process Statistics: {stats}")

def test_dspy_services():
    """Test the DSPy services used by the workflow"""
    print("\n🧪 Testing DSPy Services")
    print("=" * 30)
    
    # Test work performer
    print("\n1. Testing Work Performer:")
    work_result = dspy_registry.call_signature("perform_work", 
                                             work_type="initial_work", 
                                             iteration="1")
    print(f"   Work Result: {work_result}")
    
    # Test workflow interpreter
    print("\n2. Testing Workflow Interpreter:")
    interpretation = dspy_registry.call_signature("interpret_workflow_results",
                                                work_result=json.dumps(work_result),
                                                execution_history=json.dumps([]),
                                                current_iteration="1")
    print(f"   Interpretation: {interpretation}")
    
    # Test workflow summarizer
    print("\n3. Testing Workflow Summarizer:")
    summary = dspy_registry.call_signature("generate_workflow_summary",
                                         work_result=json.dumps(work_result),
                                         interpretation=json.dumps(interpretation),
                                         execution_history=json.dumps([]),
                                         current_iteration="1")
    print(f"   Summary: {summary}")

if __name__ == "__main__":
    try:
        # Test DSPy services first
        test_dspy_services()
        
        # Test the full workflow
        test_self_interpreting_workflow()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc() 