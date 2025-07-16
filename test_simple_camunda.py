#!/usr/bin/env python3
"""
Minimal test for SpiffWorkflow CamundaParser with a real Camunda BPMN file and its DMN dependency.
"""
from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState

BPMN_FILE = "spiff-example-cli/bpmn/camunda/task_types.bpmn"
DMN_FILE = "spiff-example-cli/bpmn/camunda/product_prices.dmn"

# Provide required input data for the workflow
INITIAL_DATA = {
    "product_name": "product_a",
    "quantity": 1,
    "product_quantity": 1,
    # Add more fields if needed by the DMN/BPMN
}

def test_camunda_parser():
    print(f"Testing CamundaParser with: {BPMN_FILE} and DMN: {DMN_FILE}")
    parser = CamundaParser()
    parser.add_bpmn_file(BPMN_FILE)
    parser.add_dmn_file(DMN_FILE)
    specs = parser.find_all_specs()
    print(f"Found process IDs: {list(specs.keys())}")
    if not specs:
        print("✗ No process specs found!")
        return False
    process_id = list(specs.keys())[0]
    spec = specs[process_id]
    workflow = BpmnWorkflow(spec)
    workflow.set_data(**INITIAL_DATA)
    print(f"✓ Parsed process: {process_id}")
    step = 0
    while not workflow.is_completed() and step < 20:
        ready = workflow.get_tasks(state=TaskState.READY)
        print(f"Step {step+1}: {len(ready)} ready tasks")
        for t in ready:
            print(f"  Running: {t.task_spec.name}")
            print(f"    Task data before run: {t.data}")
            # Patch: ensure all required variables are present in task data
            for k, v in INITIAL_DATA.items():
                if k not in t.data:
                    t.data[k] = v
            try:
                t.run()
            except Exception as e:
                print(f"    ERROR running task: {e}")
                import traceback
                traceback.print_exc()
                return False
        step += 1
    if workflow.is_completed():
        print("✓ Workflow completed!")
        print(f"Final workflow data: {workflow.data}")
        return True
    else:
        print("✗ Workflow did not complete in 20 steps.")
        return False

if __name__ == "__main__":
    success = test_camunda_parser()
    if success:
        print("\n🎉 CamundaParser basic test passed!")
    else:
        print("\n❌ CamundaParser basic test failed.")
        exit(1) 