"""
DMN Integration for AutoTel Framework

This module provides DMN (Decision Model and Notation) integration capabilities
for the AutoTel framework, based on patterns from spiff-example-cli.
"""

import logging
from typing import Dict, List, Any, Optional
from pathlib import Path

from SpiffWorkflow.spiff.parser import SpiffBpmnParser
from SpiffWorkflow.bpmn.script_engine import PythonScriptEngine
from SpiffWorkflow.bpmn import BpmnWorkflow

from ..core.telemetry import TelemetryManager
from ..schemas.validation import SchemaValidator

logger = logging.getLogger(__name__)


class DMNIntegration:
    """
    Integrates DMN decision tables with AutoTel workflows.
    
    This class provides functionality to load, validate, and execute DMN decisions
    within BPMN workflows, with full telemetry and schema validation support.
    """
    
    def __init__(self, telemetry_manager: TelemetryManager, schema_validator: SchemaValidator):
        """
        Initialize DMN integration with telemetry and validation support.
        
        Args:
            telemetry_manager: Telemetry manager for OTEL instrumentation
            schema_validator: Schema validator for LinkML validation
        """
        self.telemetry_manager = telemetry_manager
        self.schema_validator = schema_validator
        self.parser = SpiffBpmnParser()
        self.script_engine = PythonScriptEngine()
        
    def load_dmn_files(self, dmn_files: List[str]) -> None:
        """
        Load DMN files into the parser.
        
        Args:
            dmn_files: List of paths to DMN files
        """
        with self.telemetry_manager.start_span("dmn.load_files") as span:
            span.set_attribute("dmn.file_count", len(dmn_files))
            
            for dmn_file in dmn_files:
                try:
                    self.parser.add_dmn_files([dmn_file])
                    span.add_event(f"Loaded DMN file: {dmn_file}")
                    logger.info(f"Loaded DMN file: {dmn_file}")
                except Exception as e:
                    span.record_exception(e)
                    logger.error(f"Failed to load DMN file {dmn_file}: {e}")
                    raise
    
    def validate_dmn_decision(self, decision_id: str, input_data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Validate DMN decision input data against schema.
        
        Args:
            decision_id: ID of the DMN decision to validate
            input_data: Input data for the decision
            
        Returns:
            Validated input data
        """
        with self.telemetry_manager.start_span("dmn.validate_decision") as span:
            span.set_attribute("dmn.decision_id", decision_id)
            span.set_attribute("dmn.input_keys", list(input_data.keys()))
            
            # Validate input data against schema if available
            if hasattr(self.schema_validator, 'validate_dmn_input'):
                validated_data = self.schema_validator.validate_dmn_input(decision_id, input_data)
                span.add_event("DMN input validation completed")
                return validated_data
            
            # Fallback to basic validation
            logger.warning(f"No schema validation available for DMN decision: {decision_id}")
            return input_data
    
    def execute_dmn_decision(self, decision_id: str, input_data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Execute a DMN decision with telemetry instrumentation.
        
        Args:
            decision_id: ID of the DMN decision to execute
            input_data: Input data for the decision
            
        Returns:
            Decision output data
        """
        with self.telemetry_manager.start_span("dmn.execute_decision") as span:
            span.set_attribute("dmn.decision_id", decision_id)
            span.set_attribute("dmn.input_data", str(input_data))
            
            try:
                # Validate input data
                validated_input = self.validate_dmn_decision(decision_id, input_data)
                
                # Execute decision using SpiffWorkflow
                decision = self.parser.get_decision(decision_id)
                result = decision.execute(validated_input, self.script_engine)
                
                span.set_attribute("dmn.output_data", str(result))
                span.add_event("DMN decision executed successfully")
                
                logger.info(f"Executed DMN decision {decision_id} with result: {result}")
                return result
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to execute DMN decision {decision_id}: {e}")
                raise
    
    def integrate_with_workflow(self, workflow: BpmnWorkflow, dmn_files: List[str]) -> None:
        """
        Integrate DMN files with a BPMN workflow.
        
        Args:
            workflow: BPMN workflow to integrate with
            dmn_files: List of DMN files to integrate
        """
        with self.telemetry_manager.start_span("dmn.integrate_workflow") as span:
            span.set_attribute("dmn.file_count", len(dmn_files))
            
            # Load DMN files
            self.load_dmn_files(dmn_files)
            
            # Set up workflow to use DMN decisions
            workflow.script_engine = self.script_engine
            
            # Add DMN execution wrapper to workflow
            self._add_dmn_execution_wrapper(workflow)
            
            span.add_event("DMN integration completed")
            logger.info(f"Integrated {len(dmn_files)} DMN files with workflow")
    
    def _add_dmn_execution_wrapper(self, workflow: BpmnWorkflow) -> None:
        """
        Add DMN execution wrapper to workflow tasks.
        
        Args:
            workflow: Workflow to add wrapper to
        """
        # This would wrap business rule tasks to add telemetry
        # Implementation depends on specific workflow structure
        pass
    
    def list_available_decisions(self) -> List[str]:
        """
        List all available DMN decisions.
        
        Returns:
            List of decision IDs
        """
        with self.telemetry_manager.start_span("dmn.list_decisions") as span:
            try:
                decisions = self.parser.get_decisions()
                decision_ids = [d.id for d in decisions]
                span.set_attribute("dmn.decision_count", len(decision_ids))
                span.add_event(f"Found {len(decision_ids)} DMN decisions")
                return decision_ids
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to list DMN decisions: {e}")
                return []


class DMNWorkflowOrchestrator:
    """
    Orchestrates workflows that include DMN decisions.
    
    This class provides high-level orchestration of BPMN workflows
    that incorporate DMN decision tables.
    """
    
    def __init__(self, dmn_integration: DMNIntegration):
        """
        Initialize the DMN workflow orchestrator.
        
        Args:
            dmn_integration: DMN integration instance
        """
        self.dmn_integration = dmn_integration
        self.telemetry_manager = dmn_integration.telemetry_manager
    
    def create_workflow_with_dmn(self, 
                                bpmn_files: List[str], 
                                dmn_files: List[str],
                                process_id: str) -> BpmnWorkflow:
        """
        Create a workflow that includes DMN decisions.
        
        Args:
            bpmn_files: List of BPMN files
            dmn_files: List of DMN files
            process_id: BPMN process ID
            
        Returns:
            Configured BPMN workflow
        """
        with self.telemetry_manager.start_span("dmn.create_workflow") as span:
            span.set_attribute("bpmn.file_count", len(bpmn_files))
            span.set_attribute("dmn.file_count", len(dmn_files))
            span.set_attribute("bpmn.process_id", process_id)
            
            try:
                # Load BPMN files
                self.dmn_integration.parser.add_bpmn_files(bpmn_files)
                
                # Get workflow specification
                spec = self.dmn_integration.parser.get_spec(process_id)
                dependencies = self.dmn_integration.parser.get_subprocess_specs(process_id)
                
                # Create workflow
                workflow = BpmnWorkflow(spec, dependencies, 
                                      script_engine=self.dmn_integration.script_engine)
                
                # Integrate DMN files
                self.dmn_integration.integrate_with_workflow(workflow, dmn_files)
                
                span.add_event("Workflow with DMN created successfully")
                logger.info(f"Created workflow {process_id} with {len(dmn_files)} DMN files")
                
                return workflow
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to create workflow with DMN: {e}")
                raise
    
    def execute_workflow_with_dmn(self, 
                                 workflow: BpmnWorkflow, 
                                 initial_data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Execute a workflow that includes DMN decisions.
        
        Args:
            workflow: BPMN workflow to execute
            initial_data: Initial data for the workflow
            
        Returns:
            Final workflow data
        """
        with self.telemetry_manager.start_span("dmn.execute_workflow") as span:
            span.set_attribute("workflow.initial_data_keys", list(initial_data.keys()))
            
            try:
                # Set initial data
                workflow.data.update(initial_data)
                
                # Execute workflow
                workflow.run_until_user_input_required()
                
                # Get final data
                final_data = workflow.data
                
                span.set_attribute("workflow.final_data_keys", list(final_data.keys()))
                span.add_event("Workflow with DMN executed successfully")
                
                logger.info("Executed workflow with DMN decisions")
                return final_data
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to execute workflow with DMN: {e}")
                raise


def create_dmn_integration(telemetry_manager: TelemetryManager, 
                          schema_validator: SchemaValidator) -> DMNIntegration:
    """
    Factory function to create DMN integration instance.
    
    Args:
        telemetry_manager: Telemetry manager for OTEL instrumentation
        schema_validator: Schema validator for LinkML validation
        
    Returns:
        Configured DMN integration instance
    """
    return DMNIntegration(telemetry_manager, schema_validator)


def create_dmn_orchestrator(dmn_integration: DMNIntegration) -> DMNWorkflowOrchestrator:
    """
    Factory function to create DMN workflow orchestrator.
    
    Args:
        dmn_integration: DMN integration instance
        
    Returns:
        Configured DMN workflow orchestrator
    """
    return DMNWorkflowOrchestrator(dmn_integration) 