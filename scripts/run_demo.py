
from bpmn_orchestrator import BPMNOrchestrator
import os

# Create BPMN directory if it doesn't exist
os.makedirs('bpmn', exist_ok=True)

# Initialize orchestrator
orchestrator = BPMNOrchestrator(bpmn_files_path='bpmn')

# Get statistics
stats = orchestrator.get_process_statistics()
print(f'Available processes: {stats["process_definitions"]}')

# Start a sample process if available
if 'Process_1' in stats['process_definitions']:
    instance = orchestrator.start_process('Process_1', {'input': 'demo'})
    print(f'Started process instance: {instance.instance_id}')
    
    result = orchestrator.execute_process(instance.instance_id)
    print(f'Process completed with status: {result.status.value}')
    
    # Show final statistics
    final_stats = orchestrator.get_process_statistics()
    print(f'Final statistics: {final_stats}')
else:
    print('No sample processes found. Check bpmn/ directory for BPMN files.')
