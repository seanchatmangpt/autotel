#!/usr/bin/env python3
"""
Simple test script to verify BpmnParser works with basic BPMN files and referenced DMN files.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'spiff-example-cli'))

from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.util.task import TaskState

def test_bpmn_parser():
    """Test the BpmnParser with a simple BPMN file and its DMN dependency."""
    print("Testing BpmnParser...")
    
    # Create parser
    parser = BpmnParser()
    
    # Load BPMN and DMN files (use tutorial BPMN/DMN for pure Spiff example)
    bpmn_file = "spiff-example-cli/bpmn/tutorial/top_level.bpmn"
    dmn_file = "spiff-example-cli/bpmn/tutorial/product_prices.dmn"
    print(f"Loading BPMN file: {bpmn_file}")
    print(f"Loading DMN file: {dmn_file}")
    
    try:
        # Add DMN file first, then BPMN file
        parser.add_dmn_file(dmn_file)
        parser.add_bpmn_file(bpmn_file)
        
        # Get process specs
        process_specs = parser.find_all_specs()
        print(f"Found {len(process_specs)} process specifications")
        
        for process_id, process_spec in process_specs.items():
            print(f"Process ID: {process_id}")
            print(f"Process spec type: {type(process_spec)}")
            
            # Create workflow instance
            workflow = BpmnWorkflow(process_spec)
            print(f"Created workflow: {workflow}")
            print(f"Workflow completed: {workflow.is_completed()}")
            
            # Get ready tasks
            ready_tasks = workflow.get_tasks(TaskState.READY)
            print(f"Ready tasks: {len(ready_tasks)}")
            
            for task in ready_tasks:
                print(f"  Task: {task.task_spec.bpmn_name} (ID: {task.id})")
            
            print("BpmnParser test completed successfully!")
            return True
            
    except Exception as e:
        print(f"Error testing BpmnParser: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_bpmn_parser()
    sys.exit(0 if success else 1) 