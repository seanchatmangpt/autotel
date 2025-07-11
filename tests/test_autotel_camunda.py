#!/usr/bin/env python3
"""
Test AutoTel Camunda-style Implementation
Demonstrates DSPy + DMN integration following spiff-example-cli patterns.
"""

import json
import logging
from typing import Dict, Any

from autotel.core.telemetry import TelemetryManager
from autotel.schemas.validation import SchemaValidator
from autotel.workflows.autotel_camunda import create_autotel_camunda_engine

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


def setup_components():
    """Set up telemetry and validation components"""
    telemetry_manager = TelemetryManager()
    schema_validator = SchemaValidator()
    return telemetry_manager, schema_validator


def test_autotel_camunda_engine():
    """Test the AutoTel Camunda engine"""
    logger.info("=== Testing AutoTel Camunda Engine ===")
    
    # Set up components
    telemetry_manager, schema_validator = setup_components()
    
    # Create engine
    engine = create_autotel_camunda_engine(telemetry_manager, schema_validator)
    
    # Add BPMN file
    bpmn_path = "bpmn/autotel_camunda_example.bpmn"
    logger.info(f"Adding BPMN file: {bpmn_path}")
    engine.add_bpmn_file(bpmn_path)
    
    # List processes
    processes = engine.list_processes()
    logger.info(f"Available processes: {processes}")
    
    # List DMN decisions
    decisions = engine.list_dmn_decisions()
    logger.info(f"Available DMN decisions: {decisions}")
    
    return engine


def test_workflow_execution(engine):
    """Test workflow execution with DSPy + DMN"""
    logger.info("=== Testing Workflow Execution ===")
    
    # Test data
    initial_data = {
        "input_data": "This is a sample text for analysis. It contains positive sentiment and should be approved."
    }
    
    logger.info(f"Initial data: {initial_data}")
    
    try:
        # Create workflow
        workflow = engine.create_workflow("AutoTelCamundaExample", initial_data)
        logger.info("✓ Workflow created successfully")
        
        # Execute workflow
        result = engine.execute_workflow(workflow, run_until_user_input=False)
        logger.info("✓ Workflow executed successfully")
        logger.info(f"Final result: {result}")
        
        # Analyze results
        if "analysis_result" in result:
            analysis = result["analysis_result"]
            logger.info(f"DSPy Analysis: {analysis}")
        
        if "Action" in result:
            action = result["Action"]
            logger.info(f"DMN Decision: {action}")
        
        return result
        
    except Exception as e:
        logger.error(f"✗ Workflow execution failed: {e}")
        raise


def test_dspy_signatures():
    """Test DSPy signature parsing and registration"""
    logger.info("=== Testing DSPy Signatures ===")
    
    from autotel.utils.dspy_services import dspy_registry
    
    # List registered signatures
    signatures = dspy_registry.list_signatures()
    logger.info(f"Registered signatures: {list(signatures.keys())}")
    
    # List parser signatures
    parser_signatures = dspy_registry.list_parser_signatures()
    logger.info(f"Parser signatures: {list(parser_signatures.keys())}")
    
    # Test calling a signature
    if "AnalyzeData" in parser_signatures:
        logger.info("Testing AnalyzeData signature...")
        try:
            result = dspy_registry.call_signature("AnalyzeData", data="Test data for analysis")
            logger.info(f"✓ AnalyzeData result: {result}")
        except Exception as e:
            logger.error(f"✗ AnalyzeData failed: {e}")


def test_dmn_integration():
    """Test DMN integration"""
    logger.info("=== Testing DMN Integration ===")
    
    # This would test the DMN decision table execution
    # For now, we'll just verify that DMN decisions are available
    from autotel.workflows.autotel_camunda import create_autotel_camunda_engine
    from autotel.core.telemetry import TelemetryManager
    
    telemetry_manager = TelemetryManager()
    schema_validator = SchemaValidator()
    engine = create_autotel_camunda_engine(telemetry_manager, schema_validator)
    
    # Add BPMN file with DMN
    engine.add_bpmn_file("bpmn/autotel_camunda_example.bpmn")
    
    # List DMN decisions
    decisions = engine.list_dmn_decisions()
    logger.info(f"DMN decisions found: {decisions}")
    
    if "Decision_1" in decisions:
        logger.info("✓ DMN decision 'Decision_1' found")
    else:
        logger.warning("✗ DMN decision 'Decision_1' not found")


def main():
    """Main test function"""
    logger.info("Starting AutoTel Camunda Tests")
    
    try:
        # Test engine creation
        engine = test_autotel_camunda_engine()
        
        # Test DSPy signatures
        test_dspy_signatures()
        
        # Test DMN integration
        test_dmn_integration()
        
        # Test workflow execution
        result = test_workflow_execution(engine)
        
        logger.info("=== All Tests Completed Successfully ===")
        logger.info(f"Final workflow result: {json.dumps(result, indent=2)}")
        
    except Exception as e:
        logger.error(f"Test failed: {e}")
        raise


if __name__ == "__main__":
    main() 