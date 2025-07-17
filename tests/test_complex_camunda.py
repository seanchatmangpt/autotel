#!/usr/bin/env python3
"""
Complex test for SpiffWorkflow CamundaParser with gateway_types.bpmn.
This workflow includes various gateway types and user tasks.
"""
from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.camunda.specs import UserTask
from SpiffWorkflow.bpmn.specs.defaults import ManualTask, NoneTask

BPMN_FILE = "spiff-example-cli/bpmn/tutorial/gateway_types.bpmn"

# Initial data for the workflow
INITIAL_DATA = {
    "product_name": "product_a",
    "product_quantity": 1,
    "shipping_method": "standard",
    "shipping_address": "123 Main St, Anytown, USA",
    "card_number": "4111111111111111",
    "place_order": "Y"
}

class ComplexCamundaTest:
    def __init__(self):
        self.parser = CamundaParser()
        self.workflow = None
        self.step_count = 0
        self.max_steps = 50
        
    def handle_user_task(self, task):
        """Handle user tasks by providing appropriate form data"""
        task_name = task.task_spec.name
        print(f"    Handling user task: {task_name}")
        
        if "Enter Payment Info" in task_name:
            task.data.update({
                "card_number": INITIAL_DATA["card_number"]
            })
        elif "Select Shipping Method" in task_name:
            task.data.update({
                "shipping_method": INITIAL_DATA["shipping_method"]
            })
        elif "Enter Shipping Address" in task_name:
            task.data.update({
                "shipping_address": INITIAL_DATA["shipping_address"]
            })
        elif "Review Order" in task_name:
            task.data.update({
                "place_order": INITIAL_DATA["place_order"]
            })
        else:
            # For any other user task, provide default values
            print(f"    Warning: Unknown user task '{task_name}', using defaults")
        
        task.complete()
        
    def handle_manual_task(self, task):
        """Handle manual tasks by auto-completing them"""
        print(f"    Auto-completing manual task: {task.task_spec.name}")
        task.complete()
        
    def run_workflow(self):
        """Run the complex workflow with proper task handling"""
        print(f"Testing complex Camunda workflow: {BPMN_FILE}")
        
        # Load BPMN file only (no DMN)
        self.parser.add_bpmn_file(BPMN_FILE)
        
        # Get process specs
        specs = self.parser.find_all_specs()
        print(f"Found process IDs: {list(specs.keys())}")
        
        if not specs:
            print("✗ No process specs found!")
            return False
            
        # Get the first available process
        process_id = list(specs.keys())[0]
        spec = specs[process_id]
        self.workflow = BpmnWorkflow(spec)
        self.workflow.set_data(**INITIAL_DATA)
        
        print(f"✓ Parsed process: {process_id}")
        print(f"✓ Initial data: {INITIAL_DATA}")
        
        # Execute workflow
        while not self.workflow.is_completed() and self.step_count < self.max_steps:
            self.step_count += 1
            ready_tasks = self.workflow.get_tasks(state=TaskState.READY)
            
            print(f"\nStep {self.step_count}: {len(ready_tasks)} ready tasks")
            
            if not ready_tasks:
                print("    No ready tasks - workflow may be stuck")
                break
                
            for task in ready_tasks:
                task_name = task.task_spec.name
                task_type = type(task.task_spec).__name__
                
                print(f"  Running: {task_name} ({task_type})")
                print(f"    Task data before: {task.data}")
                
                # Handle different task types
                if isinstance(task.task_spec, UserTask):
                    self.handle_user_task(task)
                elif isinstance(task.task_spec, (ManualTask, NoneTask)):
                    self.handle_manual_task(task)
                else:
                    # For script tasks, business rule tasks, etc.
                    try:
                        task.run()
                        print(f"    ✓ Task completed successfully")
                    except Exception as e:
                        print(f"    ERROR running task: {e}")
                        import traceback
                        traceback.print_exc()
                        return False
                
                print(f"    Task data after: {task.data}")
        
        # Check completion
        if self.workflow.is_completed():
            print(f"\n✓ Workflow completed successfully in {self.step_count} steps!")
            print(f"Final workflow data: {self.workflow.data}")
            return True
        else:
            print(f"\n✗ Workflow did not complete in {self.max_steps} steps")
            print(f"Current workflow data: {self.workflow.data}")
            return False

def main():
    test = ComplexCamundaTest()
    success = test.run_workflow()
    
    if success:
        print("\n🎉 Complex Camunda workflow test passed!")
    else:
        print("\n❌ Complex Camunda workflow test failed.")
        exit(1)

if __name__ == "__main__":
    main() 