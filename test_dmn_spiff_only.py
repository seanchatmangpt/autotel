#!/usr/bin/env python3
"""
Test DMN using SpiffWorkflow's standard BpmnDmnParser (not DspyBpmnParser).
"""
from SpiffWorkflow.dmn.parser import BpmnDmnParser
from SpiffWorkflow import Workflow
from SpiffWorkflow.util.task import TaskState

def main():
    print("🧪 Testing DMN with SpiffWorkflow's BpmnDmnParser")
    print("=" * 50)
    
    # Use standard BpmnDmnParser (not DspyBpmnParser)
    parser = BpmnDmnParser()
    parser.add_bpmn_file("bpmn/simple_dmn_only_example.bpmn")
    
    specs = parser.find_all_specs()
    print(f"📋 Found {len(specs)} process specs")
    
    for process_id, spec in specs.items():
        print(f"🔍 Process: {process_id}")
        for task_id, task_spec in spec.task_specs.items():
            print(f"   Task: {task_spec.name} ({task_spec.__class__.__name__})")
            if hasattr(task_spec, 'decision_table'):
                print(f"   ✅ Has DMN decision table")
    
    # Test workflow execution
    spec = specs["SimpleDMNExample"]
    wf = Workflow(spec)
    
    test_cases = [
        {"product_name": "product_a", "expected": "15.00"},
        {"product_name": "product_f", "expected": "30.00"},
    ]
    
    for case in test_cases:
        print(f"\n🧪 Testing: {case['product_name']}")
        wf.set_data(**case)
        
        # Execute the workflow
        while not wf.is_completed():
            ready_tasks = wf.get_tasks(state=TaskState.READY)
            for task in ready_tasks:
                print(f"   Executing: {task.task_spec.name}")
                if hasattr(task.task_spec, 'decision_table'):
                    result = task.task_spec.decision_table.evaluate(wf.data)
                    print(f"   ✅ DMN Result: {result}")
                task.complete()
        
        # Check result
        result = wf.data
        product_price = result.get("Output_1")
        print(f"   Final result: {product_price} (expected: {case['expected']})")
        
        if product_price == case["expected"]:
            print("   ✅ PASS")
        else:
            print("   ❌ FAIL")
    
    print("\n🎉 DMN test completed!")

if __name__ == "__main__":
    main() 