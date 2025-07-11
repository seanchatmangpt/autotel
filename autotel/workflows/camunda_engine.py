import logging
from SpiffWorkflow.camunda.parser import CamundaParser
from SpiffWorkflow.camunda.specs import UserTask
from SpiffWorkflow.bpmn.specs.defaults import ManualTask, NoneTask
from SpiffWorkflow.bpmn import BpmnWorkflow
from SpiffWorkflow.bpmn.util.subworkflow import BpmnSubWorkflow
from SpiffWorkflow.bpmn.specs import BpmnProcessSpec
from SpiffWorkflow.util.task import TaskState
from SpiffWorkflow.bpmn.specs.defaults import ServiceTask

logger = logging.getLogger('autotel_camunda_engine')
logger.setLevel(logging.INFO)

class CamundaEngine:
    """Simplified Camunda engine for AutoTel without UI dependencies."""
    
    def __init__(self):
        self.parser = CamundaParser()
        self.handlers = {
            UserTask: self._handle_user_task,
            ManualTask: self._handle_manual_task,
            NoneTask: self._handle_manual_task,
            ServiceTask: self._handle_service_task,
        }
    
    def _handle_user_task(self, task):
        """Handle user tasks by auto-completing them with default values."""
        logger.info(f"Auto-completing user task: {task.task_spec.bpmn_name}")
        # For now, just complete the task with empty data
        task.complete()
    
    def _handle_manual_task(self, task):
        """Handle manual tasks by auto-completing them."""
        logger.info(f"Auto-completing manual task: {task.task_spec.bpmn_name}")
        task.complete()
    
    def _handle_service_task(self, task):
        """Handle service tasks by setting a dummy result and completing the task."""
        logger.info(f"Auto-completing service task: {task.task_spec.bpmn_name}")
        task.data['result'] = 'mocked'
        task.complete()
    
    def load_bpmn_files(self, file_paths):
        """Load multiple BPMN files at once to register all process specs."""
        self.parser.add_bpmn_files(file_paths)

    def load_bpmn_file(self, file_path):
        """Load a single BPMN file (for compatibility)."""
        self.load_bpmn_files([file_path])
    
    def create_workflow(self, spec_name, data=None):
        """Create a workflow instance from a spec."""
        specs = self.parser.find_all_specs()
        if spec_name not in specs:
            raise ValueError(f"Workflow spec '{spec_name}' not found")
        
        spec = specs[spec_name]
        workflow = BpmnWorkflow(spec)
        
        if data:
            workflow.data = data
        
        return workflow
    
    def run_workflow(self, workflow):
        """Run a workflow to completion, auto-running all non-manual tasks and auto-completing manual/user tasks."""
        logger.info("Starting workflow execution")
        
        while not workflow.is_completed():
            # Run all READY, non-manual tasks automatically
            task = workflow.get_next_task(state=TaskState.READY, manual=False)
            while task is not None:
                logger.info(f"Auto-running engine task: {task.task_spec.bpmn_name}")
                task.run()
                # Refresh waiting/event tasks
                workflow.refresh_waiting_tasks()
                task = workflow.get_next_task(state=TaskState.READY, manual=False)
            
            # Auto-complete all READY manual/user tasks
            manual_tasks = workflow.get_tasks(state=TaskState.READY, manual=True)
            for task in manual_tasks:
                logger.info(f"Auto-completing manual/user task: {task.task_spec.bpmn_name}")
                task.complete()
            
            # If no tasks are READY, break to avoid infinite loop
            if not workflow.get_tasks(state=TaskState.READY):
                logger.warning("No ready tasks found, workflow may be stuck")
                break
        
        logger.info("Workflow execution completed")
        return workflow.data 