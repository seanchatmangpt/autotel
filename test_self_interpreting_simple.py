#!/usr/bin/env python3
"""
Simple Test for Self-Interpreting BPMN Workflow
Uses the working SpiffWorkflow integration approach
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.utils.dspy_services import dspy_registry

def test_self_interpreting_workflow_simple():
    """Test the self-interpreting workflow using the simple approach"""
    print("🚀 Testing Self-Interpreting BPMN Workflow (Simple Approach)")
    print("=" * 60)
    
    # Set up initial context for the workflow
    initial_context = {
        "current_iteration": "1",
        "execution_history": json.dumps([]),
        "max_iterations": "5"
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

def test_dspy_services_individually():
    """Test each DSPy service individually"""
    print("\n🧪 Testing DSPy Services Individually")
    print("=" * 40)
    
    # Test 1: Work Performer
    print("\n1. Testing Work Performer:")
    work_result = dspy_registry.call_signature("perform_work", 
                                             work_type="initial_work", 
                                             iteration="1")
    print(f"   ✅ Work Result: {work_result}")
    
    # Test 2: Workflow Interpreter
    print("\n2. Testing Workflow Interpreter:")
    interpretation = dspy_registry.call_signature("interpret_workflow_results",
                                                work_result=json.dumps(work_result),
                                                execution_history=json.dumps([]),
                                                current_iteration="1")
    print(f"   ✅ Interpretation: {interpretation}")
    
    # Test 3: Workflow Summarizer
    print("\n3. Testing Workflow Summarizer:")
    summary = dspy_registry.call_signature("generate_workflow_summary",
                                         work_result=json.dumps(work_result),
                                         interpretation=json.dumps(interpretation),
                                         execution_history=json.dumps([]),
                                         current_iteration="1")
    print(f"   ✅ Summary: {summary}")
    
    return work_result, interpretation, summary

def test_workflow_with_iterations():
    """Test the workflow with multiple iterations"""
    print("\n🔄 Testing Workflow with Multiple Iterations")
    print("=" * 50)
    
    # Simulate multiple iterations
    execution_history = []
    current_iteration = 1
    max_iterations = 3
    
    for iteration in range(1, max_iterations + 1):
        print(f"\n🔄 Iteration {iteration}")
        print("-" * 20)
        
        # Perform work
        work_result = dspy_registry.call_signature("perform_work", 
                                                 work_type="continued_work" if iteration > 1 else "initial_work", 
                                                 iteration=str(iteration))
        print(f"💼 Work Result: {work_result.get('work_result', 'No result')}")
        
        # Interpret results
        interpretation = dspy_registry.call_signature("interpret_workflow_results",
                                                    work_result=json.dumps(work_result),
                                                    execution_history=json.dumps(execution_history),
                                                    current_iteration=str(iteration))
        print(f"🧠 Interpretation: {interpretation.get('reasoning', 'No reasoning')}")
        print(f"🎯 Should continue: {interpretation.get('should_continue', 'Unknown')}")
        
        # Add to history
        execution_history.append({
            "iteration": iteration,
            "work_result": work_result,
            "interpretation": interpretation
        })
        
        # Check if we should continue
        should_continue = interpretation.get('should_continue', 'false').lower() == 'true'
        if not should_continue:
            print(f"🛑 Stopping at iteration {iteration} based on interpretation")
            break
    
    # Generate final summary
    print(f"\n📋 Generating Final Summary")
    print("-" * 30)
    final_summary = dspy_registry.call_signature("generate_workflow_summary",
                                               work_result=json.dumps(work_result),
                                               interpretation=json.dumps(interpretation),
                                               execution_history=json.dumps(execution_history),
                                               current_iteration=str(len(execution_history)))
    
    print(f"📊 Final Summary: {final_summary.get('final_summary', 'No summary')}")
    print(f"🔍 Key Insights: {final_summary.get('key_insights', 'No insights')}")
    print(f"📈 Performance Metrics: {final_summary.get('performance_metrics', 'No metrics')}")

if __name__ == "__main__":
    try:
        # Test DSPy services individually first
        test_dspy_services_individually()
        
        # Test workflow with iterations
        test_workflow_with_iterations()
        
        # Test the full BPMN workflow
        test_self_interpreting_workflow_simple()
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc() 