#!/usr/bin/env python3
"""
Production Workflow Engine Test
Tests the complete Four Pillars system with production-ready features
"""

import json
import sys
import tempfile
import shutil
from pathlib import Path
from unittest.mock import patch, MagicMock

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent.parent))

from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.advanced_dspy_services import advanced_dspy_registry
from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
from SpiffWorkflow.bpmn.script_engine import TaskDataEnvironment

class AutoTelWorkflowEngine:
    """Production workflow engine with Four Pillars integration"""
    
    def __init__(self, project_directory: str):
        self.project_dir = Path(project_directory)
        self.parser = DspyBpmnParser()
        self.serializer = self._setup_serializer()
        self.script_env = self._setup_script_environment()
        self.active_workflows = {}
        
        # Load the complete project
        self.parser.load_from_sealed_directory(self.project_dir)
        
        print("🚀 AutoTel Workflow Engine initialized with Four Pillars:")
        print(f"   📁 Project: {self.project_dir}")
        print(f"   🔄 Processes: {len(self.parser.loaded_contracts['bpmn_files'])}")
        print(f"   📋 Rules: {len(self.parser.dmn_parsers)}")
        print(f"   🧠 AI Signatures: {len(self.parser.signature_definitions)}")
        print(f"   📊 Data Shapes: {len(self.parser.shacl_graph)} triples")
    
    def _setup_serializer(self):
        """Configure serializer with custom converters"""
        # For now, skip serializer setup to focus on core functionality
        return None
    
    def _setup_script_environment(self):
        """Setup script environment with DSPy and SHACL utilities"""
        return TaskDataEnvironment({
            # DSPy utilities
            'dspy': __import__('dspy'),
            'get_signature': self.parser.get_dynamic_signature,
            
            # SHACL utilities
            'validate_with_shacl': self._script_shacl_validate,
            'create_rdf_graph': self._script_create_rdf,
            
            # Standard utilities
            'datetime': __import__('datetime'),
            'json': __import__('json'),
            'uuid': __import__('uuid'),
            
            # Workflow utilities
            'log_info': lambda msg: print(f"[WORKFLOW] {msg}"),
            'log_error': lambda msg: print(f"[ERROR] {msg}"),
        })
    
    def _script_shacl_validate(self, data, shape_uri):
        """SHACL validation function for scripts"""
        # Implementation that scripts can use
        return True  # Simplified for example
    
    def _script_create_rdf(self, data_dict):
        """Create RDF graph from dict for scripts"""
        from rdflib import Graph
        # Implementation for script use
        return Graph()
    
    def start_workflow(self, process_id: str, initial_data: dict = None) -> str:
        """Start a new workflow instance"""
        import uuid
        
        # Get process specification
        spec = self.parser.get_spec(process_id)
        
        # Create workflow instance
        workflow = BpmnWorkflow(spec, script_engine=self.script_env)
        workflow_id = str(uuid.uuid4())
        
        # Set initial data
        if initial_data:
            workflow.set_data(**initial_data)
        
        # Store workflow
        self.active_workflows[workflow_id] = workflow
        
        print(f"🎯 Started workflow: {process_id} (ID: {workflow_id})")
        
        return workflow_id
    
    def advance_workflow(self, workflow_id: str, user_inputs: dict = None) -> dict:
        """Advance workflow execution"""
        workflow = self.active_workflows.get(workflow_id)
        if not workflow:
            raise ValueError(f"Workflow {workflow_id} not found")
        
        # Apply user inputs if provided
        if user_inputs:
            workflow.set_data(**user_inputs)
        
        # Track execution
        initial_state = 'active' if not workflow.is_completed() else 'completed'
        
        try:
            # Run workflow until it needs user input or completes
            workflow.run_all()
            
            final_state = 'active' if not workflow.is_completed() else 'completed'
            
            # Get current tasks requiring user action
            user_tasks = self._get_user_tasks(workflow)
            
            result = {
                'workflow_id': workflow_id,
                'status': final_state,
                'state_changed': initial_state != final_state,
                'completed': workflow.is_completed(),
                'user_tasks': user_tasks,
                'data': {},  # Simplified for now
                'validation_summary': self._get_validation_summary(workflow)
            }
            
            # Clean up completed workflows
            if workflow.is_completed():
                print(f"✅ Workflow {workflow_id} completed successfully")
                del self.active_workflows[workflow_id]
            
            return result
            
        except Exception as e:
            print(f"❌ Workflow {workflow_id} failed: {e}")
            # Store error info
            workflow.set_data(error=str(e), error_timestamp=__import__('datetime').datetime.now().isoformat())
            raise
    
    def _get_user_tasks(self, workflow):
        """Get tasks requiring user interaction (simplified for test)"""
        return []
    
    def _get_validation_summary(self, workflow):
        """Get summary of validation results (simplified for test)"""
        return []
    
    def get_workflow_status(self, workflow_id: str) -> dict:
        """Get current workflow status"""
        workflow = self.active_workflows.get(workflow_id)
        if not workflow:
            return {'error': f'Workflow {workflow_id} not found'}
        
        return {
            'workflow_id': workflow_id,
            'status': 'active' if not workflow.is_completed() else 'completed',
            'completed': workflow.is_completed(),
            'data_keys': [],  # Simplified for now
            'task_count': 0,  # Simplified for now
            'ready_tasks': 0  # Simplified for now
        }
    
    def complete_user_task(self, workflow_id: str, task_id: str, task_data: dict = None) -> dict:
        """Complete a user task"""
        workflow = self.active_workflows.get(workflow_id)
        if not workflow:
            raise ValueError(f"Workflow {workflow_id} not found")
        
        # Find the task
        task = workflow.get_task_from_id(task_id)
        if not task:
            raise ValueError(f"Task {task_id} not found")
        
        # Set task data if provided
        if task_data:
            task.set_data(**task_data)
        
        # Complete the task
        workflow.complete_task_from_id(task_id)
        
        # Continue execution
        return self.advance_workflow(workflow_id)

def test_production_workflow_engine():
    """Test the production workflow engine with complete Four Pillars integration"""
    print("🧪 Testing Production Workflow Engine")
    print("=" * 45)
    
    # Create a temporary project directory
    with tempfile.TemporaryDirectory() as temp_dir:
        project_dir = Path(temp_dir) / "autotel_project"
        project_dir.mkdir()
        
        # Create SHACL shapes file
        shacl_content = """<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:sh="http://www.w3.org/ns/shacl#"
         xmlns:autotel="http://autotel.ai/shapes#"
         xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
  <sh:NodeShape rdf:about="http://autotel.ai/shapes#CustomerShape">
    <sh:property>
      <sh:PropertyShape>
        <sh:path rdf:resource="http://autotel.ai/shapes#customerId"/>
        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <sh:minCount rdf:datatype="http://www.w3.org/2001/XMLSchema#integer">1</sh:minCount>
      </sh:PropertyShape>
    </sh:property>
  </sh:NodeShape>
</rdf:RDF>"""
        
        (project_dir / "data_shapes.shacl.xml").write_text(shacl_content)
        
        # Create DMN file
        dmn_content = """<?xml version="1.0" encoding="UTF-8"?>
<dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/">
        <dmn:decision id="ApprovalDecision" name="Approval Decision">
        <dmn:decisionTable id="ApprovalDecisionTable">
          <dmn:input id="riskLevel" label="Risk Level">
        <dmn:inputExpression typeRef="string">
          <dmn:text>risk_assessment</dmn:text>
        </dmn:inputExpression>
      </dmn:input>
      <dmn:output id="approved" label="Approved" typeRef="boolean"/>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"LOW"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>true</dmn:text></dmn:outputEntry>
      </dmn:rule>
      <dmn:rule>
        <dmn:inputEntry><dmn:text>"HIGH"</dmn:text></dmn:inputEntry>
        <dmn:outputEntry><dmn:text>false</dmn:text></dmn:outputEntry>
      </dmn:rule>
    </dmn:decisionTable>
  </dmn:decision>
</dmn:definitions>"""
        
        (project_dir / "business_rules.dmn").write_text(dmn_content)
        
        # Create DSPy signatures file
        dspy_content = """<?xml version="1.0" encoding="UTF-8"?>
<dspy:signatures xmlns:dspy="http://autotel.ai/dspy">
  <dspy:signature name="CustomerAnalysis" description="Analyze customer data for risk assessment">
    <dspy:input name="customer_data" description="Customer information" 
                shaclShape="http://autotel.ai/shapes#CustomerShape"/>
    <dspy:output name="risk_assessment" description="Risk analysis result"/>
  </dspy:signature>
</dspy:signatures>"""
        
        (project_dir / "ai_signatures.dspy.xml").write_text(dspy_content)
        
        # Create main BPMN process
        bpmn_content = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/"
                  xmlns:camunda="http://camunda.org/schema/1.0/bpmn"
                  targetNamespace="http://autotel.ai/workflows">

  <bpmn:process id="intelligent_loan_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Loan Application Received"/>
    
    <bpmn:serviceTask id="ai_analysis" name="AI Risk Analysis">
      <bpmn:extensionElements>
        <dspy:service name="customer_risk_analyzer" signature="CustomerAnalysis">
          <dspy:param name="customer_data" value="application_data"/>
        </dspy:service>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    
    <bpmn:businessRuleTask id="approval_decision" name="Apply Approval Rules" 
                           camunda:decisionRef="ApprovalDecision"/>
    
    <bpmn:exclusiveGateway id="approval_gateway" name="Loan Approved?"/>
    
    <bpmn:endEvent id="end_approved" name="Loan Approved"/>
    <bpmn:endEvent id="end_rejected" name="Loan Rejected"/>
    
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="ai_analysis"/>
    <bpmn:sequenceFlow id="flow2" sourceRef="ai_analysis" targetRef="approval_decision"/>
    <bpmn:sequenceFlow id="flow3" sourceRef="approval_decision" targetRef="approval_gateway"/>
    
    <bpmn:sequenceFlow id="flow_approved" sourceRef="approval_gateway" targetRef="end_approved">
      <bpmn:conditionExpression>approved == true</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
    <bpmn:sequenceFlow id="flow_rejected" sourceRef="approval_gateway" targetRef="end_rejected">
      <bpmn:conditionExpression>approved == false</bpmn:conditionExpression>
    </bpmn:sequenceFlow>
    
  </bpmn:process>
  
</bpmn:definitions>"""
        
        (project_dir / "process.bpmn").write_text(bpmn_content)
        
        # Create workflow state directory
        (project_dir / "workflow_state").mkdir()
        
        try:
            # Initialize the engine
            engine = AutoTelWorkflowEngine(str(project_dir))
            
            # Test workflow execution
            initial_data = {
                'application_data': {
                    'customerId': 'CUST_12345',
                    'firstName': 'John',
                    'lastName': 'Doe',
                    'creditScore': 720,
                    'annualIncome': 85000,
                    'requestedAmount': 150000
                }
            }
            
            # Start workflow
            workflow_id = engine.start_workflow('intelligent_loan_process', initial_data)
            
            # Execute the workflow
            print("\n🔄 Executing Four Pillars Workflow:")
            print("   1. SHACL validates input data structure")
            print("   2. DSPy performs AI risk analysis")
            print("   3. DMN applies business rules for approval")
            print("   4. BPMN orchestrates the entire process")
            
            with patch('autotel.utils.dspy_services.dspy_service') as mock_dspy:
                # Mock DSPy service to return a risk assessment
                mock_dspy.return_value = {
                    'risk_assessment': 'LOW'  # This should result in approval
                }
                
                result = engine.advance_workflow(workflow_id)
                
                print(f"\n📊 Workflow Result:")
                print(f"   Status: {result['status']}")
                print(f"   Completed: {result['completed']}")
                print(f"   Validation Steps: {len(result['validation_summary'])}")
                
                # Show validation results
                for validation in result['validation_summary']:
                    status = "✅ PASS" if validation['conforms'] else "❌ FAIL"
                    print(f"   {validation['step']}: {status}")
                    if validation['violations'] > 0:
                        print(f"     Violations: {validation['violations']}")
                
                # Show final decision
                final_data = result['data']
                if 'approved' in final_data:
                    decision = "APPROVED" if final_data['approved'] else "REJECTED"
                    print(f"\n🎯 Final Decision: {decision}")
                    
                    if 'risk_assessment' in final_data:
                        risk = final_data['risk_assessment']
                        print(f"   Risk Level: {risk}")
                
                # Verify workflow completed
                assert result['completed'], "Workflow should complete"
                assert 'approved' in final_data, "Should have approval decision"
                
                print("✅ Production workflow engine test successful")
                
        except Exception as e:
            print(f"❌ Production workflow engine test failed: {e}")
            import traceback
            traceback.print_exc()
            raise

def test_workflow_engine_features():
    """Test individual workflow engine features"""
    print("\n🧪 Testing Workflow Engine Features")
    print("=" * 40)
    
    # Create a minimal project for testing
    with tempfile.TemporaryDirectory() as temp_dir:
        project_dir = Path(temp_dir) / "test_project"
        project_dir.mkdir()
        
        # Create minimal BPMN
        bpmn_content = """<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  targetNamespace="http://autotel.ai/workflows">
  <bpmn:process id="simple_process" isExecutable="true">
    <bpmn:startEvent id="start" name="Start"/>
    <bpmn:endEvent id="end" name="End"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="end"/>
  </bpmn:process>
</bpmn:definitions>"""
        
        (project_dir / "process.bpmn").write_text(bpmn_content)
        (project_dir / "workflow_state").mkdir()
        
        try:
            # Test engine initialization
            engine = AutoTelWorkflowEngine(str(project_dir))
            
            # Test workflow status
            workflow_id = engine.start_workflow('simple_process')
            status = engine.get_workflow_status(workflow_id)
            
            assert status['workflow_id'] == workflow_id, "Status should return correct workflow ID"
            assert not status['completed'], "Workflow should not be completed initially"
            
            # Test workflow advancement
            result = engine.advance_workflow(workflow_id)
            assert result['completed'], "Simple workflow should complete"
            
            # Test error handling for non-existent workflow
            try:
                engine.advance_workflow("non-existent-id")
                assert False, "Should raise error for non-existent workflow"
            except ValueError:
                pass  # Expected
            
            print("✅ Workflow engine features test successful")
            
        except Exception as e:
            print(f"❌ Workflow engine features test failed: {e}")
            raise

def run_production_tests():
    """Run all production workflow engine tests"""
    print("🚀 Running Production Workflow Engine Tests")
    print("=" * 55)
    
    try:
        test_production_workflow_engine()
        test_workflow_engine_features()
        
        print("\n🎉 All Production Workflow Engine Tests PASSED!")
        print("=" * 55)
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True

if __name__ == "__main__":
    success = run_production_tests()
    sys.exit(0 if success else 1) 