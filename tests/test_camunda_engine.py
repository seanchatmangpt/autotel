import pytest
from autotel.workflows.camunda_engine import CamundaEngine

def test_camunda_engine_basic():
    """Test that the Camunda engine can load and run a trivial BPMN workflow."""
    engine = CamundaEngine()
    
    # Use the trivial BPMN file
    bpmn_path = "schemas/trivial.bpmn"
    
    try:
        # Load the BPMN file
        engine.load_bpmn_file(bpmn_path)
        
        # Create a workflow (process id is 'trivial_process')
        workflow = engine.create_workflow("trivial_process", {"input_data": "test"})
        
        # Run the workflow
        result = engine.run_workflow(workflow)
        
        # Assert the workflow completed successfully
        assert workflow.is_completed()
        assert result is not None
        
        print(f"Workflow completed successfully. Final data: {result}")
        
    except Exception as e:
        pytest.fail(f"Camunda engine test failed: {e}")

if __name__ == "__main__":
    test_camunda_engine_basic() 