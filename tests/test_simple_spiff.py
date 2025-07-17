#!/usr/bin/env python3
"""
Simple test to verify basic Spiff BPMN functionality works.
"""

from SpiffWorkflow import Workflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow

def test_simple_bpmn():
    """Test basic BPMN workflow execution."""
    print("Testing basic Spiff BPMN functionality...")
    
    # Use a simple BPMN file without DSPy extensions
    bpmn_file = "bpmn/simple_process.bpmn"
    
    try:
        # Parse BPMN file
        parser = BpmnParser()
        parser.add_bpmn_file(bpmn_file)
        specs = parser.find_all_specs()
        
        # Get the first process spec
        process_id = list(specs.keys())[0]
        spec = specs[process_id]
        
        # Create workflow
        workflow = BpmnWorkflow(spec)
        print(f"✓ Successfully parsed BPMN file: {bpmn_file}")
        print(f"✓ Process ID: {process_id}")
        
        # Set some test data
        workflow.set_data(input_data="Test input data")
        print("✓ Set workflow data")
        
        # Get initial ready tasks
        ready_tasks = workflow.get_tasks(state=TaskState.READY)
        print(f"✓ Initial ready tasks: {len(ready_tasks)}")
        
        # Execute workflow
        step_count = 0
        while not workflow.is_completed() and step_count < 10:
            ready_tasks = workflow.get_tasks(state=TaskState.READY)
            print(f"Step {step_count + 1}: {len(ready_tasks)} ready tasks")
            
            for task in ready_tasks:
                print(f"  Running task: {task.task_spec.name}")
                task.run()
                print(f"  ✓ Task completed: {task.task_spec.name}")
            
            step_count += 1
        
        if workflow.is_completed():
            print("✓ Workflow completed successfully!")
            print(f"Final workflow data: {workflow.data}")
            return True
        else:
            print("✗ Workflow did not complete within step limit")
            return False
            
    except Exception as e:
        print(f"✗ Error testing BPMN functionality: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_simple_bpmn()
    if success:
        print("\n🎉 Basic Spiff BPMN functionality is working!")
    else:
        print("\n❌ Basic Spiff BPMN functionality has issues.")
        exit(1) 