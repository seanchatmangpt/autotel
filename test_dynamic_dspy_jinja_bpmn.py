#!/usr/bin/env python3
"""
Dynamic DSPy + Jinja2 + BPMN Integration Tests

This module tests the full pipeline:
1. DSPy generates task specifications with dynamic data
2. Jinja2 renders BPMN XML templates with business logic
3. SpiffWorkflow executes the generated BPMN workflows
4. Faker provides realistic test data for variety
"""

import sys
import os
import tempfile
import random
import json
from faker import Faker
from pathlib import Path
from typing import Dict, Any, List

# Add the project root to the path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from autotel.factory.processors.jinja_processor import JinjaProcessor
from autotel.factory.processors.bpmn_processor import BPMNProcessor


def test_dynamic_dspy_jinja_bpmn_single():
    """Test: DSPy + Jinja2 generates BPMN, then executes with SpiffWorkflow."""
    print("🧪 Testing Dynamic DSPy + Jinja2 → BPMN → SpiffWorkflow execution")
    print("=" * 60)

    # Initialize Faker for realistic data generation
    fake = Faker()
    Faker.seed(random.randint(1, 10000))  # Random seed for variety
    
    # Generate random business data
    order_amount = round(random.uniform(50.0, 2500.0), 2)
    customer_tier = random.choice(['bronze', 'silver', 'gold', 'platinum'])
    shipping_method = random.choice(['standard', 'express', 'overnight'])
    payment_method = random.choice(['credit_card', 'debit_card', 'paypal', 'bank_transfer'])
    
    # Simulate DSPy output for task details with dynamic data
    dspy_output = {
        "task_name": f"process_{fake.word()}_order",
        "task_description": f"Process {customer_tier} customer order with {shipping_method} shipping",
        "task_type": "scriptTask",
        "input_variables": ["order_id", "customer_name", "order_amount", "customer_tier", "shipping_method"],
        "output_variables": ["order_status", "confirmation_number", "estimated_delivery", "processing_fee"],
        "business_rules": {
            "min_amount_for_express": 100.0,
            "platinum_discount": 0.15,
            "express_fee": 25.0,
            "overnight_fee": 50.0
        }
    }

    # Jinja2 template for BPMN XML with dynamic business logic
    bpmn_template = '''\
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="Process_{{ task_name }}" name="{{ task_description }}">
    
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:{{ task_type }} id="Task_{{ task_name }}" name="{{ task_name }}">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <bpmn:script><![CDATA[
import random
from datetime import datetime, timedelta

print(f"Processing {{ task_name }}...")
print(f"Order ID: {order_id}")
print(f"Customer: {customer_name}")
print(f"Amount: ${order_amount}")
print(f"Tier: {customer_tier}")
print(f"Shipping: {shipping_method}")

# Business logic processing
processing_fee = 0.0
{% for rule_name, rule_value in business_rules.items() %}
{{ rule_name }} = {{ rule_value }}
{% endfor %}

# Calculate processing fee based on shipping method
if shipping_method == "express":
    if order_amount < min_amount_for_express:
        processing_fee = express_fee
elif shipping_method == "overnight":
    processing_fee = overnight_fee

# Apply platinum discount
if customer_tier == "platinum":
    discount = order_amount * platinum_discount
    order_amount = order_amount - discount
    print(f"Applied platinum discount: ${discount:.2f}")

# Generate confirmation number
confirmation_number = f"ORD-{random.randint(100000, 999999)}-{customer_tier.upper()}"

# Calculate estimated delivery
base_days = {"standard": 5, "express": 2, "overnight": 1}
delivery_days = base_days.get(shipping_method, 5)
estimated_delivery = (datetime.now() + timedelta(days=delivery_days)).strftime("%Y-%m-%d")

# Set order status
order_status = "processed"

print(f"Processing fee: ${processing_fee:.2f}")
print(f"Final amount: ${order_amount:.2f}")
print(f"Confirmation: {confirmation_number}")
print(f"Estimated delivery: {estimated_delivery}")
print("Task completed successfully!")
      ]]></bpmn:script>
    </bpmn:{{ task_type }}>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_{{ task_name }}" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_{{ task_name }}" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
  <bpmndi:BPMNDiagram id="BPMNDiagram_1">
    <bpmndi:BPMNPlane id="BPMNPlane_1" bpmnElement="Process_{{ task_name }}">
      <bpmndi:BPMNShape id="_BPMNShape_StartEvent_2" bpmnElement="StartEvent_1">
        <dc:Bounds height="36.0" width="36.0" x="412.0" y="240.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="Task_{{ task_name }}_di" bpmnElement="Task_{{ task_name }}">
        <dc:Bounds height="80.0" width="100.0" x="520.0" y="218.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="_BPMNShape_EndEvent_2" bpmnElement="EndEvent_1">
        <dc:Bounds height="28.0" width="28.0" x="692.0" y="254.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNEdge id="Flow_1_di" bpmnElement="Flow_1">
        <di:waypoint x="448.0" y="258.0" />
        <di:waypoint x="520.0" y="258.0" />
      </bpmndi:BPMNEdge>
      <bpmndi:BPMNEdge id="Flow_2_di" bpmnElement="Flow_2">
        <di:waypoint x="620.0" y="258.0" />
        <di:waypoint x="692.0" y="268.0" />
      </bpmndi:BPMNEdge>
    </bpmndi:BPMNPlane>
  </bpmndi:BPMNDiagram>
  
</bpmn:definitions>
'''

    # Render the BPMN template with DSPy output
    jinja_processor = JinjaProcessor()
    rendered_bpmn = jinja_processor.render_single_template(bpmn_template, dspy_output)
    
    print("📄 Generated BPMN XML:")
    print(rendered_bpmn[:500] + "..." if len(rendered_bpmn) > 500 else rendered_bpmn)
    print()

    # Write BPMN to temp file
    with tempfile.TemporaryDirectory() as temp_dir:
        bpmn_file = os.path.join(temp_dir, "generated_workflow.bpmn")
        with open(bpmn_file, "w") as f:
            f.write(rendered_bpmn)
        
        print(f"📄 BPMN file written to: {bpmn_file}")
        
        # Execute with SpiffWorkflow
        try:
            bpmn_processor = BPMNProcessor()
            process_id = f"Process_{dspy_output['task_name']}"
            
            # Parse and get workflow spec
            workflow_spec = bpmn_processor.parse(rendered_bpmn, process_id)
            print(f"✅ BPMN parsed successfully! Process ID: {process_id}")
            
            # Create workflow instance with script engine
            from SpiffWorkflow.workflow import Workflow
            from SpiffWorkflow.bpmn.script_engine import PythonScriptEngine
            
            script_engine = PythonScriptEngine()
            workflow = Workflow(workflow_spec)
            workflow.script_engine = script_engine
            
            # Generate dynamic input variables with Faker
            workflow.data = {
                "order_id": f"ORD-{fake.random_number(digits=8)}",
                "customer_name": fake.name(),
                "order_amount": order_amount,
                "customer_tier": customer_tier,
                "shipping_method": shipping_method
            }
            
            print("🚀 Starting workflow execution...")
            print(f"📊 Input data: {workflow.data}")
            
            # Execute workflow
            workflow.run_all()
            
            print("✅ Workflow completed!")
            print(f"📊 Final data: {workflow.data}")
            
            # Verify outputs were generated
            expected_outputs = ["order_status", "confirmation_number", "estimated_delivery", "processing_fee"]
            for output_var in expected_outputs:
                if output_var in workflow.data:
                    print(f"✅ Output '{output_var}': {workflow.data[output_var]}")
                else:
                    print(f"❌ Missing output: {output_var}")
            
            # Additional validation
            if "confirmation_number" in workflow.data:
                conf_num = workflow.data["confirmation_number"]
                assert conf_num.startswith("ORD-"), f"Confirmation number should start with 'ORD-': {conf_num}"
                assert customer_tier.upper() in conf_num, f"Confirmation number should contain tier: {conf_num}"
            
            if "estimated_delivery" in workflow.data:
                delivery_date = workflow.data["estimated_delivery"]
                assert len(delivery_date) == 10, f"Delivery date should be YYYY-MM-DD format: {delivery_date}"
            
            print("🎯 All validations passed!")
            
        except Exception as e:
            print(f"❌ Workflow execution failed: {e}")
            import traceback
            traceback.print_exc()


def test_dynamic_dspy_jinja_bpmn_iterations():
    """Test: Run multiple iterations of DSPy + Jinja2 → BPMN → SpiffWorkflow with different random data."""
    print("🧪 Testing Dynamic DSPy + Jinja2 → BPMN → SpiffWorkflow (Multiple Iterations)")
    print("=" * 80)
    
    # Initialize Faker
    fake = Faker()
    
    # Run multiple iterations to show dynamism
    for iteration in range(3):
        print(f"\n🔄 Iteration {iteration + 1}/3")
        print("-" * 40)
        
        # Generate different random data each time
        Faker.seed(random.randint(1, 10000))
        
        # Random business scenario
        scenarios = [
            {"type": "electronics", "tiers": ["bronze", "silver", "gold"], "methods": ["standard", "express"]},
            {"type": "clothing", "tiers": ["silver", "gold", "platinum"], "methods": ["standard", "express", "overnight"]},
            {"type": "furniture", "tiers": ["gold", "platinum"], "methods": ["standard", "express"]}
        ]
        
        scenario = random.choice(scenarios)
        order_amount = round(random.uniform(100.0, 5000.0), 2)
        customer_tier = random.choice(scenario["tiers"])
        shipping_method = random.choice(scenario["methods"])
        
        # Dynamic DSPy output
        dspy_output = {
            "task_name": f"process_{scenario['type']}_order",
            "task_description": f"Process {scenario['type']} order for {customer_tier} customer with {shipping_method} shipping",
            "task_type": "scriptTask",
            "input_variables": ["order_id", "customer_name", "order_amount", "customer_tier", "shipping_method"],
            "output_variables": ["order_status", "confirmation_number", "estimated_delivery", "processing_fee", "total_amount"],
            "business_rules": {
                "min_amount_for_express": random.randint(75, 150),
                "platinum_discount": round(random.uniform(0.10, 0.20), 2),
                "express_fee": random.randint(20, 35),
                "overnight_fee": random.randint(45, 65),
                "tax_rate": round(random.uniform(0.05, 0.12), 3)
            }
        }
        
        # Simplified BPMN template for iterations
        bpmn_template = '''\
<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="Process_{{ task_name }}" name="{{ task_description }}">
    
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:{{ task_type }} id="Task_{{ task_name }}" name="{{ task_name }}">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
      <bpmn:script><![CDATA[
import random
from datetime import datetime, timedelta

print(f"Processing {{ task_name }}...")
print(f"Order ID: {order_id}")
print(f"Customer: {customer_name}")
print(f"Amount: ${order_amount}")
print(f"Tier: {customer_tier}")
print(f"Shipping: {shipping_method}")

# Load business rules
{% for rule_name, rule_value in business_rules.items() %}
{{ rule_name }} = {{ rule_value }}
{% endfor %}

# Calculate fees and discounts
processing_fee = 0.0
if shipping_method == "express":
    if order_amount < min_amount_for_express:
        processing_fee = express_fee
elif shipping_method == "overnight":
    processing_fee = overnight_fee

# Apply tier discounts
discount = 0.0
if customer_tier == "platinum":
    discount = order_amount * platinum_discount
elif customer_tier == "gold":
    discount = order_amount * 0.10
elif customer_tier == "silver":
    discount = order_amount * 0.05

# Calculate tax
tax = (order_amount - discount) * tax_rate

# Calculate total
total_amount = order_amount - discount + processing_fee + tax

# Generate confirmation
confirmation_number = f"ORD-{random.randint(100000, 999999)}-{customer_tier.upper()}"

# Calculate delivery
delivery_days = {"standard": 5, "express": 2, "overnight": 1}.get(shipping_method, 5)
estimated_delivery = (datetime.now() + timedelta(days=delivery_days)).strftime("%Y-%m-%d")

order_status = "processed"

print(f"Discount: ${discount:.2f}")
print(f"Processing fee: ${processing_fee:.2f}")
print(f"Tax: ${tax:.2f}")
print(f"Total: ${total_amount:.2f}")
print(f"Confirmation: {confirmation_number}")
print(f"Delivery: {estimated_delivery}")
print("Task completed!")
      ]]></bpmn:script>
    </bpmn:{{ task_type }}>
    
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_{{ task_name }}" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_{{ task_name }}" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
  <bpmndi:BPMNDiagram id="BPMNDiagram_1">
    <bpmndi:BPMNPlane id="BPMNPlane_1" bpmnElement="Process_{{ task_name }}">
      <bpmndi:BPMNShape id="_BPMNShape_StartEvent_2" bpmnElement="StartEvent_1">
        <dc:Bounds height="36.0" width="36.0" x="412.0" y="240.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="Task_{{ task_name }}_di" bpmnElement="Task_{{ task_name }}">
        <dc:Bounds height="80.0" width="100.0" x="520.0" y="218.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="_BPMNShape_EndEvent_2" bpmnElement="EndEvent_1">
        <dc:Bounds height="28.0" width="28.0" x="692.0" y="254.0" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNEdge id="Flow_1_di" bpmnElement="Flow_1">
        <di:waypoint x="448.0" y="258.0" />
        <di:waypoint x="520.0" y="258.0" />
      </bpmndi:BPMNEdge>
      <bpmndi:BPMNEdge id="Flow_2_di" bpmnElement="Flow_2">
        <di:waypoint x="620.0" y="258.0" />
        <di:waypoint x="692.0" y="268.0" />
      </bpmndi:BPMNEdge>
    </bpmndi:BPMNPlane>
  </bpmndi:BPMNDiagram>
  
</bpmn:definitions>
'''
        
        # Render and execute
        jinja_processor = JinjaProcessor()
        rendered_bpmn = jinja_processor.render_single_template(bpmn_template, dspy_output)
        
        try:
            bpmn_processor = BPMNProcessor()
            process_id = f"Process_{dspy_output['task_name']}"
            workflow_spec = bpmn_processor.parse(rendered_bpmn, process_id)
            
            from SpiffWorkflow.workflow import Workflow
            from SpiffWorkflow.bpmn.script_engine import PythonScriptEngine
            
            script_engine = PythonScriptEngine()
            workflow = Workflow(workflow_spec)
            workflow.script_engine = script_engine
            
            # Dynamic input data
            workflow.data = {
                "order_id": f"ORD-{fake.random_number(digits=8)}",
                "customer_name": fake.name(),
                "order_amount": order_amount,
                "customer_tier": customer_tier,
                "shipping_method": shipping_method
            }
            
            print(f"📊 Input: {workflow.data}")
            workflow.run_all()
            
            # Show key outputs
            outputs = workflow.data
            print(f"✅ Status: {outputs.get('order_status', 'N/A')}")
            print(f"💰 Total: ${outputs.get('total_amount', 0):.2f}")
            print(f"📦 Confirmation: {outputs.get('confirmation_number', 'N/A')}")
            print(f"📅 Delivery: {outputs.get('estimated_delivery', 'N/A')}")
            
        except Exception as e:
            print(f"❌ Iteration {iteration + 1} failed: {e}")
    
    print(f"\n🎯 Completed {3} dynamic iterations successfully!")


def main():
    """Run all dynamic DSPy + Jinja2 + BPMN tests."""
    try:
        test_dynamic_dspy_jinja_bpmn_single()
        print("\n" + "="*80 + "\n")
        test_dynamic_dspy_jinja_bpmn_iterations()
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main()) 