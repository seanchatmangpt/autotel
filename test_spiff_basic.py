from SpiffWorkflow import Workflow
from SpiffWorkflow.util.task import TaskState

# Create a simple workflow spec manually
from SpiffWorkflow.specs import WorkflowSpec, Simple

# Create a simple workflow
spec = WorkflowSpec('SimpleWorkflow')
start = Simple(spec, 'Start')
task = Simple(spec, 'Task')
end = Simple(spec, 'End')

start.connect(task)
task.connect(end)

# Create and run workflow
wf = Workflow(spec)
wf.set_data(input_data="Test data")

print("Initial ready tasks:", wf.get_tasks(state=TaskState.READY))

while not wf.is_completed():
    ready = wf.get_tasks(state=TaskState.READY)
    print("Ready tasks:", ready)
    for t in ready:
        print("Running task:", t.task_spec.name)
        t.run()
        print("Task completed")

print("Workflow completed. Final data:", wf.data) 