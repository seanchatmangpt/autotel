#!/usr/bin/env python3
"""
Complex test for SpiffWorkflow BpmnParser with a complex workflow.
This demonstrates complex BPMN functionality including gateways, user tasks, and script tasks.
"""
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.defaults import UserTask, ManualTask, NoneTask

# Create a complex BPMN workflow programmatically
COMPLEX_BPMN_XML = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="ComplexWorkflow" isExecutable="true">
    
    <!-- Start Event -->
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <!-- User Task -->
    <bpmn:userTask id="UserTask_1" name="Enter Data">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Exclusive Gateway -->
    <bpmn:exclusiveGateway id="Gateway_1" name="Check Condition">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
      <bpmn:outgoing>Flow_4</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Script Task -->
    <bpmn:scriptTask id="ScriptTask_1" name="Process Data">
      <bpmn:incoming>Flow_3</bpmn:incoming>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
      <bpmn:script>result = input_data * 2
processed = True</bpmn:script>
    </bpmn:scriptTask>
    
    <!-- Manual Task -->
    <bpmn:manualTask id="ManualTask_1" name="Manual Review">
      <bpmn:incoming>Flow_4</bpmn:incoming>
      <bpmn:outgoing>Flow_6</bpmn:outgoing>
    </bpmn:manualTask>
    
    <!-- Parallel Gateway -->
    <bpmn:parallelGateway id="Gateway_2" name="Merge">
      <bpmn:incoming>Flow_5</bpmn:incoming>
      <bpmn:incoming>Flow_6</bpmn:incoming>
      <bpmn:outgoing>Flow_7</bpmn:outgoing>
    </bpmn:parallelGateway>
    
    <!-- Service Task -->
    <bpmn:serviceTask id="ServiceTask_1" name="Final Processing">
      <bpmn:incoming>Flow_7</bpmn:incoming>
      <bpmn:outgoing>Flow_8</bpmn:outgoing>
    </bpmn:serviceTask>
    
    <!-- End Event -->
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_8</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="UserTask_1"/>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="UserTask_1" targetRef="Gateway_1"/>
         <bpmn:sequenceFlow id="Flow_3" sourceRef="Gateway_1" targetRef="ScriptTask_1">
       <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">input_data &gt; 10</bpmn:conditionExpression>
     </bpmn:sequenceFlow>
     <bpmn:sequenceFlow id="Flow_4" sourceRef="Gateway_1" targetRef="ManualTask_1">
       <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">input_data &lt;= 10</bpmn:conditionExpression>
     </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_5" sourceRef="ScriptTask_1" targetRef="Gateway_2"/>
    <bpmn:sequenceFlow id="Flow_6" sourceRef="ManualTask_1" targetRef="Gateway_2"/>
    <bpmn:sequenceFlow id="Flow_7" sourceRef="Gateway_2" targetRef="ServiceTask_1"/>
    <bpmn:sequenceFlow id="Flow_8" sourceRef="ServiceTask_1" targetRef="EndEvent_1"/>
    
  </bpmn:process>
  
</bpmn:definitions>'''

class ComplexBpmnTest:
    def __init__(self):
        self.parser = BpmnParser()
        self.workflow = None
        self.step_count = 0
        self.max_steps = 50
        
    def handle_user_task(self, task):
        """Handle user tasks by providing appropriate form data"""
        task_name = task.task_spec.name
        print(f"    Handling user task: {task_name}")
        
        if "Enter Data" in task_name:
            task.data.update({
                "input_data": 15  # This will trigger the script task path
            })
            # Also update the workflow data to ensure it's available for gateway evaluation
            self.workflow.set_data(input_data=15)
        
        task.complete()
        
    def handle_manual_task(self, task):
        """Handle manual tasks by auto-completing them"""
        print(f"    Auto-completing manual task: {task.task_spec.name}")
        task.complete()
        
    def run_workflow(self):
        """Run the complex workflow with proper task handling"""
        print("Testing complex BPMN workflow with gateways, user tasks, and script tasks")
        
        # Parse BPMN XML
        from lxml import etree
        bpmn_tree = etree.fromstring(COMPLEX_BPMN_XML.encode('utf-8'))
        self.parser.add_bpmn_xml(bpmn_tree, "complex_workflow.bpmn")
        
        # Get process specs
        specs = self.parser.find_all_specs()
        print(f"Found process IDs: {list(specs.keys())}")
        
        if not specs:
            print("✗ No process specs found!")
            return False
            
        # Get the process
        process_id = "ComplexWorkflow"
        if process_id not in specs:
            print(f"✗ Process '{process_id}' not found in specs!")
            return False
            
        spec = specs[process_id]
        self.workflow = BpmnWorkflow(spec)
        
        print(f"✓ Parsed process: {process_id}")
        
        # Execute workflow
        while not self.workflow.is_completed() and self.step_count < self.max_steps:
            self.step_count += 1
            ready_tasks = self.workflow.get_tasks(state=TaskState.READY)
            
            print(f"\nStep {self.step_count}: {len(ready_tasks)} ready tasks")
            
            if not ready_tasks:
                print("    No ready tasks - checking workflow state...")
                # Check if there are any waiting tasks
                waiting_tasks = self.workflow.get_tasks(state=TaskState.WAITING)
                if waiting_tasks:
                    print(f"    Found {len(waiting_tasks)} waiting tasks: {[t.task_spec.name for t in waiting_tasks]}")
                else:
                    print("    No waiting tasks found - workflow may be stuck")
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
                    # For script tasks, service tasks, gateways, etc.
                    # Ensure required data is available in task context
                    if "Gateway" in task_name and "input_data" not in task.data:
                        task.data["input_data"] = 15
                    
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
    test = ComplexBpmnTest()
    success = test.run_workflow()
    
    if success:
        print("\n🎉 Complex BPMN workflow test passed!")
    else:
        print("\n❌ Complex BPMN workflow test failed.")
        exit(1)

if __name__ == "__main__":
    main() 