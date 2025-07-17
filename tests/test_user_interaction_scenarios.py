#!/usr/bin/env python3
"""
Test that demonstrates multiple user interaction scenarios with different outcomes.
This shows how SpiffWorkflow handles various customer service paths and escalations.
"""
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.defaults import UserTask, ManualTask, NoneTask
import time
import random

# Use the same BPMN workflow as the previous test
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

class UserInteractionScenarioSimulator:
    def __init__(self):
        self.parser = BpmnParser()
        self.workflow = None
        self.step_count = 0
        self.max_steps = 20
        
        # Define different scenarios
        self.scenarios = {
            "technical_resolved": {
                "UserTask_1": {
                    "issue_type": "technical",
                    "customer_name": "Alice Johnson",
                    "issue_description": "Software installation failed"
                },
                "UserTask_2": {
                    "troubleshooting_steps": "Updated drivers, restarted system",
                    "technical_notes": "Driver compatibility issue resolved"
                },
                "UserTask_5": {
                    "resolved": True,
                    "satisfaction_rating": 5,
                    "follow_up_notes": "Customer very satisfied"
                }
            },
            "billing_escalated": {
                "UserTask_1": {
                    "issue_type": "billing",
                    "customer_name": "Bob Smith",
                    "issue_description": "Disputed charge for unused service"
                },
                "UserTask_3": {
                    "billing_issue": "Customer claims they cancelled service",
                    "resolution": "Could not verify cancellation"
                },
                "UserTask_5": {
                    "resolved": False,
                    "satisfaction_rating": 1,
                    "follow_up_notes": "Customer very upset, needs supervisor"
                },
                "UserTask_6": {
                    "escalation_reason": "Customer dispute requires management review",
                    "supervisor_notes": "Scheduled call with customer and manager"
                }
            },
            "general_inquiry": {
                "UserTask_1": {
                    "issue_type": "general",
                    "customer_name": "Carol Davis",
                    "issue_description": "Product information request"
                },
                "UserTask_4": {
                    "inquiry_type": "Product comparison",
                    "response": "Provided detailed comparison chart"
                },
                "UserTask_5": {
                    "resolved": True,
                    "satisfaction_rating": 4,
                    "follow_up_notes": "Customer appreciates the information"
                }
            }
        }
        
    def simulate_user_interaction(self, task_name, scenario_data):
        """Simulate realistic user interaction with timing and responses"""
        print(f"    👤 User interaction: {task_name}")
        
        # Simulate thinking time
        print(f"    🤔 User is thinking...")
        time.sleep(0.3)
        
        # Simulate typing
        print(f"    ⌨️  User is typing response...")
        time.sleep(0.2)
        
        # Get scenario-specific response
        if task_name in scenario_data:
            response = scenario_data[task_name]
            print(f"    💬 User provided: {list(response.keys())}")
            return response
        else:
            print(f"    ⚠️  No specific response for {task_name}")
            return {"default_response": "User provided information"}
        
    def run_scenario(self, scenario_name):
        """Run a specific user interaction scenario"""
        print(f"\n🎭 Running Scenario: {scenario_name.upper()}")
        print("=" * 50)
        
        if scenario_name not in self.scenarios:
            print(f"❌ Scenario '{scenario_name}' not found!")
            return False
            
        scenario_data = self.scenarios[scenario_name]
        
        # Create a new parser for each scenario to avoid duplicate process IDs
        parser = BpmnParser()
        
        # Parse BPMN XML
        from lxml import etree
        bpmn_tree = etree.fromstring(USER_INTERACTION_BPMN_XML.encode('utf-8'))
        parser.add_bpmn_xml(bpmn_tree, f"user_interaction_workflow_{scenario_name}.bpmn")
        
        # Get process specs
        specs = parser.find_all_specs()
        process_id = "CustomerServiceWorkflow"
        spec = specs[process_id]
        self.workflow = BpmnWorkflow(spec)
        
        print(f"✅ Starting {scenario_name} scenario...")
        
        # Execute workflow
        self.step_count = 0
        while not self.workflow.is_completed() and self.step_count < self.max_steps:
            self.step_count += 1
            ready_tasks = self.workflow.get_tasks(state=TaskState.READY)
            
            print(f"📞 Step {self.step_count}: {len(ready_tasks)} ready tasks")
            
            if not ready_tasks:
                print("    ⏸️  No ready tasks - workflow may be stuck")
                break
                
            for task in ready_tasks:
                task_name = task.task_spec.name
                task_type = type(task.task_spec).__name__
                
                print(f"  🎯 Running: {task_name} ({task_type})")
                
                # Handle different task types
                if isinstance(task.task_spec, UserTask):
                    # Simulate user interaction
                    response = self.simulate_user_interaction(task_name, scenario_data)
                    task.data.update(response)
                    task.complete()
                    print(f"    ✅ User completed task")
                    
                elif isinstance(task.task_spec, (ManualTask, NoneTask)):
                    print(f"    🛠️  Manual task completed")
                    task.complete()
                    
                else:
                    # For gateways, script tasks, etc.
                    # Ensure required data is available
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
                        return False
                
                print(f"    📊 Task data: {list(task.data.keys())}")
        
        # Check completion
        if self.workflow.is_completed():
            print(f"🎉 Scenario completed in {self.step_count} steps!")
            
            # Determine outcome
            if "resolved" in self.workflow.data and self.workflow.data["resolved"]:
                print(f"✅ Outcome: Issue RESOLVED")
            else:
                print(f"⚠️  Outcome: Issue ESCALATED")
                
            return True
        else:
            print(f"⏰ Scenario did not complete in {self.max_steps} steps")
            return False

def main():
    print("🎭 Customer Service User Interaction Scenarios")
    print("=" * 60)
    print("This test demonstrates different customer service scenarios")
    print("with various outcomes including resolution and escalation.")
    print()
    
    simulator = UserInteractionScenarioSimulator()
    
    # Run different scenarios
    scenarios = ["technical_resolved", "billing_escalated", "general_inquiry"]
    
    for scenario in scenarios:
        success = simulator.run_scenario(scenario)
        if not success:
            print(f"❌ Scenario {scenario} failed!")
            exit(1)
        print()
    
    print("🎉 All user interaction scenarios completed successfully!")
    print("✅ Demonstrated:")
    print("   - Technical issue resolution")
    print("   - Billing issue escalation") 
    print("   - General inquiry handling")

if __name__ == "__main__":
    main() 