#!/usr/bin/env python3
"""
Test that simulates user interactions with a BPMN workflow.
This demonstrates how SpiffWorkflow handles interactive workflows with user tasks.
"""
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.defaults import UserTask, ManualTask, NoneTask
import time
import random

# Create a BPMN workflow that simulates a customer service interaction
USER_INTERACTION_BPMN_XML = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="CustomerServiceWorkflow" isExecutable="true">
    
    <!-- Start Event -->
    <bpmn:startEvent id="StartEvent_1" name="Customer Contact">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <!-- Initial Assessment -->
    <bpmn:userTask id="UserTask_1" name="Assess Customer Issue">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Decision Gateway -->
    <bpmn:exclusiveGateway id="Gateway_1" name="Issue Type">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
      <bpmn:outgoing>Flow_4</bpmn:outgoing>
      <bpmn:outgoing>Flow_5</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Technical Support Path -->
    <bpmn:userTask id="UserTask_2" name="Technical Support">
      <bpmn:incoming>Flow_3</bpmn:incoming>
      <bpmn:outgoing>Flow_6</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Billing Support Path -->
    <bpmn:userTask id="UserTask_3" name="Billing Support">
      <bpmn:incoming>Flow_4</bpmn:incoming>
      <bpmn:outgoing>Flow_7</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- General Inquiry Path -->
    <bpmn:userTask id="UserTask_4" name="General Inquiry">
      <bpmn:incoming>Flow_5</bpmn:incoming>
      <bpmn:outgoing>Flow_8</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Resolution Check -->
    <bpmn:userTask id="UserTask_5" name="Check Resolution">
      <bpmn:incoming>Flow_6</bpmn:incoming>
      <bpmn:incoming>Flow_7</bpmn:incoming>
      <bpmn:incoming>Flow_8</bpmn:incoming>
      <bpmn:outgoing>Flow_9</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- Final Decision -->
    <bpmn:exclusiveGateway id="Gateway_2" name="Issue Resolved?">
      <bpmn:incoming>Flow_9</bpmn:incoming>
      <bpmn:outgoing>Flow_10</bpmn:outgoing>
      <bpmn:outgoing>Flow_11</bpmn:outgoing>
    </bpmn:exclusiveGateway>
    
    <!-- Escalation -->
    <bpmn:userTask id="UserTask_6" name="Escalate to Supervisor">
      <bpmn:incoming>Flow_11</bpmn:incoming>
      <bpmn:outgoing>Flow_12</bpmn:outgoing>
    </bpmn:userTask>
    
    <!-- End Events -->
    <bpmn:endEvent id="EndEvent_1" name="Issue Resolved">
      <bpmn:incoming>Flow_10</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:endEvent id="EndEvent_2" name="Escalated">
      <bpmn:incoming>Flow_12</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="UserTask_1"/>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="UserTask_1" targetRef="Gateway_1"/>
    <bpmn:sequenceFlow id="Flow_3" sourceRef="Gateway_1" targetRef="UserTask_2">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "technical"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_4" sourceRef="Gateway_1" targetRef="UserTask_3">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "billing"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_5" sourceRef="Gateway_1" targetRef="UserTask_4">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">issue_type == "general"</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_6" sourceRef="UserTask_2" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_7" sourceRef="UserTask_3" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_8" sourceRef="UserTask_4" targetRef="UserTask_5"/>
    <bpmn:sequenceFlow id="Flow_9" sourceRef="UserTask_5" targetRef="Gateway_2"/>
    <bpmn:sequenceFlow id="Flow_10" sourceRef="Gateway_2" targetRef="EndEvent_1">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">resolved == True</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_11" sourceRef="Gateway_2" targetRef="UserTask_6">
      <bpmn:conditionExpression xsi:type="bpmn:tFormalExpression">resolved == False</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    <bpmn:sequenceFlow id="Flow_12" sourceRef="UserTask_6" targetRef="EndEvent_2"/>
    
  </bpmn:process>
  
</bpmn:definitions>'''

class UserInteractionSimulator:
    def __init__(self):
        self.parser = BpmnParser()
        self.workflow = None
        self.step_count = 0
        self.max_steps = 20
        self.user_responses = {
            "Assess Customer Issue": {
                "issue_type": "technical",
                "customer_name": "John Doe",
                "issue_description": "Cannot access email account"
            },
            "Technical Support": {
                "troubleshooting_steps": "Reset password, cleared cache",
                "technical_notes": "User had outdated browser"
            },
            "Billing Support": {
                "billing_issue": "Overcharged for premium plan",
                "resolution": "Applied credit to account"
            },
            "General Inquiry": {
                "inquiry_type": "Product information",
                "response": "Sent product catalog"
            },
            "Check Resolution": {
                "resolved": True,
                "satisfaction_rating": 4,
                "follow_up_notes": "Customer satisfied with resolution"
            },
            "Escalate to Supervisor": {
                "escalation_reason": "Complex technical issue",
                "supervisor_notes": "Assigned to senior technician"
            }
        }
        
    def simulate_user_thinking(self, task_name):
        """Simulate realistic user thinking time"""
        print(f"    🤔 User is thinking about {task_name}...")
        time.sleep(0.5)  # Simulate thinking time
        
    def simulate_user_typing(self, task_name):
        """Simulate user typing responses"""
        print(f"    ⌨️  User is typing response for {task_name}...")
        time.sleep(0.3)  # Simulate typing time
        
    def simulate_user_decision(self, task_name, options):
        """Simulate user making a decision"""
        print(f"    🤷 User is deciding between options for {task_name}...")
        time.sleep(0.4)  # Simulate decision time
        return random.choice(options)
        
    def handle_user_task(self, task):
        """Handle user tasks with realistic user interaction simulation"""
        task_name = task.task_spec.name
        print(f"    👤 Handling user task: {task_name}")
        
        # Simulate user thinking
        self.simulate_user_thinking(task_name)
        
        # Get user response for this task
        # Map task names to response keys
        task_response_map = {
            "UserTask_1": "Assess Customer Issue",
            "UserTask_2": "Technical Support", 
            "UserTask_3": "Billing Support",
            "UserTask_4": "General Inquiry",
            "UserTask_5": "Check Resolution",
            "UserTask_6": "Escalate to Supervisor"
        }
        
        response_key = task_response_map.get(task_name, task_name)
        if response_key in self.user_responses:
            response = self.user_responses[response_key]
            
            # Simulate user typing
            self.simulate_user_typing(task_name)
            
            # Update task data with user response
            task.data.update(response)
            
            # Simulate some variability in user responses
            if task_name == "Check Resolution":
                # Sometimes users are not satisfied
                if random.random() < 0.2:  # 20% chance of escalation
                    task.data["resolved"] = False
                    print(f"    😞 User is not satisfied - escalation needed")
                else:
                    print(f"    😊 User is satisfied with resolution")
                    
        else:
            print(f"    ⚠️  No predefined response for {task_name}, using defaults")
            task.data.update({
                "default_response": "User provided information",
                "timestamp": time.time()
            })
        
        print(f"    ✅ User completed task: {task_name}")
        task.complete()
        
    def handle_manual_task(self, task):
        """Handle manual tasks with user interaction simulation"""
        task_name = task.task_spec.name
        print(f"    🛠️  Manual task: {task_name}")
        
        # Simulate manual work time
        print(f"    ⏱️  User is performing manual work...")
        time.sleep(0.2)
        
        task.complete()
        print(f"    ✅ Manual task completed: {task_name}")
        
    def run_workflow(self):
        """Run the user interaction workflow with realistic simulation"""
        print("🎭 Simulating Customer Service User Interaction Workflow")
        print("=" * 60)
        
        # Parse BPMN XML
        from lxml import etree
        bpmn_tree = etree.fromstring(USER_INTERACTION_BPMN_XML.encode('utf-8'))
        self.parser.add_bpmn_xml(bpmn_tree, "user_interaction_workflow.bpmn")
        
        # Get process specs
        specs = self.parser.find_all_specs()
        print(f"📋 Found process IDs: {list(specs.keys())}")
        
        if not specs:
            print("❌ No process specs found!")
            return False
            
        # Get the process
        process_id = "CustomerServiceWorkflow"
        if process_id not in specs:
            print(f"❌ Process '{process_id}' not found in specs!")
            return False
            
        spec = specs[process_id]
        self.workflow = BpmnWorkflow(spec)
        
        print(f"✅ Parsed process: {process_id}")
        print(f"🎯 Starting customer service interaction simulation...")
        print()
        
        # Execute workflow
        while not self.workflow.is_completed() and self.step_count < self.max_steps:
            self.step_count += 1
            ready_tasks = self.workflow.get_tasks(state=TaskState.READY)
            
            print(f"📞 Step {self.step_count}: {len(ready_tasks)} ready tasks")
            
            if not ready_tasks:
                print("    ⏸️  No ready tasks - checking workflow state...")
                waiting_tasks = self.workflow.get_tasks(state=TaskState.WAITING)
                if waiting_tasks:
                    print(f"    🔄 Found {len(waiting_tasks)} waiting tasks: {[t.task_spec.name for t in waiting_tasks]}")
                else:
                    print("    🚫 No waiting tasks found - workflow may be stuck")
                    break
                continue
                
            for task in ready_tasks:
                task_name = task.task_spec.name
                task_type = type(task.task_spec).__name__
                
                print(f"  🎯 Running: {task_name} ({task_type})")
                print(f"    📊 Task data before: {task.data}")
                
                # Handle different task types
                if isinstance(task.task_spec, UserTask):
                    self.handle_user_task(task)
                elif isinstance(task.task_spec, (ManualTask, NoneTask)):
                    self.handle_manual_task(task)
                else:
                    # For script tasks, gateways, etc.
                    # Ensure required data is available in task context
                    if "Gateway" in task_name:
                        if "issue_type" not in task.data and "issue_type" in self.workflow.data:
                            task.data["issue_type"] = self.workflow.data["issue_type"]
                        if "resolved" not in task.data and "resolved" in self.workflow.data:
                            task.data["resolved"] = self.workflow.data["resolved"]
                    
                    try:
                        task.run()
                        print(f"    ✅ Task completed successfully")
                    except Exception as e:
                        print(f"    ❌ ERROR running task: {e}")
                        import traceback
                        traceback.print_exc()
                        return False
                
                print(f"    📊 Task data after: {task.data}")
                print()
        
        # Check completion
        if self.workflow.is_completed():
            print(f"🎉 Workflow completed successfully in {self.step_count} steps!")
            print(f"📋 Final workflow data: {self.workflow.data}")
            return True
        else:
            print(f"⏰ Workflow did not complete in {self.max_steps} steps")
            print(f"📋 Current workflow data: {self.workflow.data}")
            return False

def main():
    print("🎭 Customer Service User Interaction Simulation")
    print("=" * 60)
    print("This test simulates a realistic customer service workflow")
    print("with user interactions, decision points, and human-like responses.")
    print()
    
    simulator = UserInteractionSimulator()
    success = simulator.run_workflow()
    
    if success:
        print("\n🎉 User interaction simulation completed successfully!")
        print("✅ All user tasks were handled with realistic interactions")
    else:
        print("\n❌ User interaction simulation failed.")
        exit(1)

if __name__ == "__main__":
    main() 