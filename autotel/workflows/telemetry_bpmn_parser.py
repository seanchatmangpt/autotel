
from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
from .telemetry_task import TelemetryTaskSpec

class TelemetryBpmnParser(BpmnParser):
    """
    A custom BPMN parser that injects telemetry into task specs.
    """

    def __init__(self, telemetry_context):
        super().__init__()
        self.telemetry_context = telemetry_context

    def make_task_spec(self, process_spec, name, lane=None, description=""):
        """
        Creates a TelemetryTaskSpec instead of a regular BpmnTaskSpec.
        """
        spec = TelemetryTaskSpec(process_spec, name, lane, description)
        spec.telemetry_context = self.telemetry_context
        return spec
