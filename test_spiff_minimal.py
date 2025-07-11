from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from SpiffWorkflow import Workflow
from SpiffWorkflow.util.task import TaskState

bpmn_path = "bpmn_only/ONLY_simple_dspy_workflow.bpmn"
parser = DspyBpmnParser()
parser.add_bpmn_file(bpmn_path)
specs = parser.find_all_specs()
spec = specs["SimpleDspyWorkflow"]

wf = Workflow(spec)
wf.set_data(input_data="Sample data for analysis")

print("Ready tasks:", wf.get_tasks(state=TaskState.READY))

while not wf.is_completed():
    ready = wf.get_tasks(state=TaskState.READY)
    print("Ready:", ready)
    for t in ready:
        print("Running:", t)
        t.run()
        print("Task data after run:", t.data)

print("Workflow data:", wf.data) 