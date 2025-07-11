#!/usr/bin/env python3
"""
DMN Integration Example for AutoTel

This example demonstrates how to use DMN (Decision Model and Notation) 
with the AutoTel framework, based on patterns from spiff-example-cli.

The example shows:
1. Loading DMN files
2. Creating workflows with DMN decisions
3. Executing DMN decisions with telemetry
4. Integrating with BPMN workflows
"""

import os
import sys
import logging
from pathlib import Path
from typing import Dict, Any

# Add the project root to the path
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from autotel.core.telemetry import TelemetryManager
from autotel.schemas.validation import SchemaValidator
from autotel.workflows.dmn_integration import (
    create_dmn_integration,
    create_dmn_orchestrator
)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


def setup_telemetry() -> TelemetryManager:
    """Set up telemetry manager for the example."""
    return TelemetryManager(
        service_name="autotel-dmn-example",
        service_version="1.0.0"
    )


def setup_schema_validator() -> SchemaValidator:
    """Set up schema validator for the example."""
    return SchemaValidator()


def create_sample_dmn_file() -> str:
    """Create a sample DMN file for testing."""
    dmn_content = '''<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/" 
             xmlns:dmndi="https://www.omg.org/spec/DMN/20191111/DMNDI/" 
             xmlns:dc="http://www.omg.org/spec/DMN/20180521/DC/" 
             id="approval_decision" name="DRD" 
             namespace="http://camunda.org/schema/1.0/dmn" 
             exporter="Camunda Modeler" exporterVersion="4.11.1">
  
  <decision id="approval_decision" name="Approval Decision">
    <decisionTable id="DecisionTable_1" hitPolicy="FIRST">
      <!-- Input: confidence (number) -->
      <input id="Input_1">
        <inputExpression id="InputExpression_1" typeRef="number" expressionLanguage="python">
          <text>confidence</text>
        </inputExpression>
      </input>
      
      <!-- Input: recommendation (string) -->
      <input id="Input_2">
        <inputExpression id="InputExpression_2" typeRef="string" expressionLanguage="python">
          <text>recommendation</text>
        </inputExpression>
      </input>
      
      <!-- Output: action (string) -->
      <output id="Output_1" label="action" typeRef="string" />
      
      <!-- Decision Rules -->
      <rule id="Rule_1">
        <description>High confidence, proceed recommendation</description>
        <inputEntry id="InputEntry_1">
          <text>&gt;= 80</text>
        </inputEntry>
        <inputEntry id="InputEntry_2">
          <text>"proceed"</text>
        </inputEntry>
        <outputEntry id="OutputEntry_1">
          <text>"APPROVE"</text>
        </outputEntry>
      </rule>
      
      <rule id="Rule_2">
        <description>Medium confidence, review recommendation</description>
        <inputEntry id="InputEntry_3">
          <text>&gt;= 50</text>
        </inputEntry>
        <inputEntry id="InputEntry_4">
          <text>"review"</text>
        </inputEntry>
        <outputEntry id="OutputEntry_2">
          <text>"REVIEW"</text>
        </outputEntry>
      </rule>
      
      <rule id="Rule_3">
        <description>Low confidence or any other case</description>
        <inputEntry id="InputEntry_5">
          <text>&lt; 50</text>
        </inputEntry>
        <inputEntry id="InputEntry_6">
          <text>-</text>
        </inputEntry>
        <outputEntry id="OutputEntry_3">
          <text>"REJECT"</text>
        </outputEntry>
      </rule>
    </decisionTable>
  </decision>
  
  <!-- Diagram information -->
  <dmndi:DMNDI>
    <dmndi:DMNDiagram>
      <dmndi:DMNShape dmnElementRef="approval_decision">
        <dc:Bounds height="80" width="180" x="160" y="100" />
      </dmndi:DMNShape>
    </dmndi:DMNDiagram>
  </dmndi:DMNDI>
</definitions>'''
    
    # Write to temporary file
    dmn_file = Path("temp_approval_decision.dmn")
    dmn_file.write_text(dmn_content)
    return str(dmn_file)


def create_sample_bpmn_file() -> str:
    """Create a sample BPMN file that references the DMN decision."""
    bpmn_content = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL" 
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI" 
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC" 
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn" 
                  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI" 
                  id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  
  <bpmn:process id="approval_process" isExecutable="true">
    
    <bpmn:startEvent id="StartEvent_1" name="Start Approval">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    
    <bpmn:serviceTask id="ServiceTask_1" name="Analyze Data">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:serviceTask>
    
    <bpmn:businessRuleTask id="BusinessRuleTask_1" name="Make Decision" 
                          camunda:decisionRef="approval_decision">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
    </bpmn:businessRuleTask>
    
    <bpmn:endEvent id="EndEvent_1" name="End Approval">
      <bpmn:incoming>Flow_3</bpmn:incoming>
    </bpmn:endEvent>
    
    <!-- Sequence Flows -->
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="ServiceTask_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="ServiceTask_1" targetRef="BusinessRuleTask_1" />
    <bpmn:sequenceFlow id="Flow_3" sourceRef="BusinessRuleTask_1" targetRef="EndEvent_1" />
    
  </bpmn:process>
  
  <!-- Diagram -->
  <bpmndi:BPMNDiagram id="BPMNDiagram_1">
    <bpmndi:BPMNPlane id="BPMNPlane_1" bpmnElement="approval_process">
      <bpmndi:BPMNShape id="StartEvent_1_di" bpmnElement="StartEvent_1">
        <dc:Bounds x="152" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="ServiceTask_1_di" bpmnElement="ServiceTask_1">
        <dc:Bounds x="240" y="80" width="100" height="80" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="BusinessRuleTask_1_di" bpmnElement="BusinessRuleTask_1">
        <dc:Bounds x="400" y="80" width="100" height="80" />
      </bpmndi:BPMNShape>
      <bpmndi:BPMNShape id="EndEvent_1_di" bpmnElement="EndEvent_1">
        <dc:Bounds x="560" y="102" width="36" height="36" />
      </bpmndi:BPMNShape>
    </bpmndi:BPMNPlane>
  </bpmndi:BPMNDiagram>
  
</bpmn:definitions>'''
    
    # Write to temporary file
    bpmn_file = Path("temp_approval_process.bpmn")
    bpmn_file.write_text(bpmn_content)
    return str(bpmn_file)


def demonstrate_dmn_execution(dmn_integration) -> None:
    """Demonstrate direct DMN decision execution."""
    logger.info("=== Demonstrating Direct DMN Decision Execution ===")
    
    # Test cases for the approval decision
    test_cases = [
        {
            "name": "High confidence, proceed",
            "input": {"confidence": 85, "recommendation": "proceed"},
            "expected": "APPROVE"
        },
        {
            "name": "Medium confidence, review",
            "input": {"confidence": 65, "recommendation": "review"},
            "expected": "REVIEW"
        },
        {
            "name": "Low confidence",
            "input": {"confidence": 30, "recommendation": "proceed"},
            "expected": "REJECT"
        }
    ]
    
    for test_case in test_cases:
        logger.info(f"Testing: {test_case['name']}")
        try:
            result = dmn_integration.execute_dmn_decision(
                "approval_decision", 
                test_case["input"]
            )
            action = result.get("action", "UNKNOWN")
            logger.info(f"  Input: {test_case['input']}")
            logger.info(f"  Output: {action}")
            logger.info(f"  Expected: {test_case['expected']}")
            logger.info(f"  Status: {'✓' if action == test_case['expected'] else '✗'}")
        except Exception as e:
            logger.error(f"  Error: {e}")


def demonstrate_workflow_orchestration(dmn_orchestrator, bpmn_file: str, dmn_file: str) -> None:
    """Demonstrate workflow orchestration with DMN."""
    logger.info("=== Demonstrating Workflow Orchestration with DMN ===")
    
    try:
        # Create workflow with DMN
        workflow = dmn_orchestrator.create_workflow_with_dmn(
            bpmn_files=[bpmn_file],
            dmn_files=[dmn_file],
            process_id="approval_process"
        )
        
        logger.info("✓ Created workflow with DMN integration")
        
        # Execute workflow with sample data
        initial_data = {
            "confidence": 75,
            "recommendation": "review"
        }
        
        logger.info(f"Executing workflow with initial data: {initial_data}")
        final_data = dmn_orchestrator.execute_workflow_with_dmn(workflow, initial_data)
        
        logger.info(f"✓ Workflow completed")
        logger.info(f"Final data: {final_data}")
        
    except Exception as e:
        logger.error(f"✗ Workflow orchestration failed: {e}")


def cleanup_temp_files() -> None:
    """Clean up temporary files created during the example."""
    temp_files = [
        "temp_approval_decision.dmn",
        "temp_approval_process.bpmn"
    ]
    
    for temp_file in temp_files:
        if os.path.exists(temp_file):
            os.remove(temp_file)
            logger.info(f"Cleaned up: {temp_file}")


def main():
    """Main function to run the DMN integration example."""
    logger.info("Starting DMN Integration Example")
    
    # Set up components
    telemetry_manager = setup_telemetry()
    schema_validator = setup_schema_validator()
    
    # Create DMN integration
    dmn_integration = create_dmn_integration(telemetry_manager, schema_validator)
    dmn_orchestrator = create_dmn_orchestrator(dmn_integration)
    
    # Create sample files
    dmn_file = create_sample_dmn_file()
    bpmn_file = create_sample_bpmn_file()
    
    try:
        # Load DMN files
        logger.info("Loading DMN files...")
        dmn_integration.load_dmn_files([dmn_file])
        logger.info("✓ DMN files loaded successfully")
        
        # List available decisions
        decisions = dmn_integration.list_available_decisions()
        logger.info(f"Available decisions: {decisions}")
        
        # Demonstrate direct DMN execution
        demonstrate_dmn_execution(dmn_integration)
        
        # Demonstrate workflow orchestration
        demonstrate_workflow_orchestration(dmn_orchestrator, bpmn_file, dmn_file)
        
        logger.info("=== DMN Integration Example Completed Successfully ===")
        
    except Exception as e:
        logger.error(f"Example failed: {e}")
        raise
    
    finally:
        # Clean up
        cleanup_temp_files()


if __name__ == "__main__":
    main() 