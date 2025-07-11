
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
from opentelemetry import trace

class TelemetryTaskSpec(BpmnTaskSpec):
    """
    A custom task spec that adds telemetry to tasks.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.telemetry_context = None
        self.workflow_id = None

    def _on_ready(self, task):
        super()._on_ready(task)
        if self.telemetry_context and self.workflow_id:
            self.telemetry_context.start_task_execution(self.workflow_id, self, task)

    def _on_trigger(self, task):
        super()._on_trigger(task)

    def _on_complete(self, task):
        super()._on_complete(task)
        if self.telemetry_context and self.workflow_id:
            self.telemetry_context.end_task_execution(None, self.workflow_id, self.name, success=True)
