#!/usr/bin/env python3
"""
Test Enhanced Telemetry Engine
Demonstrates proper task-level telemetry integration with consistent workflow IDs
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.enhanced_telemetry_engine import create_enhanced_telemetry_engine
from autotel.core.telemetry import TelemetryManager, TelemetryConfig
from SpiffWorkflow.util.task import TaskState

def test_enhanced_telemetry_engine():
    """Test the enhanced telemetry engine with proper workflow-task span hierarchy"""
    print("🚀 Testing Enhanced Telemetry Engine")
    print("=" * 50)
    
    # Create telemetry manager
    telemetry_manager = TelemetryManager(TelemetryConfig(require_linkml_validation=False))
    
    # Create enhanced engine
    engine = create_enhanced_telemetry_engine(telemetry_manager)
    
    # Test BPMN file
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print(f"[ERROR] BPMN file not found: {bpmn_file}")
        return False
    
    try:
        # Add BPMN file
        engine.add_bpmn_file(bpmn_file)
        print(f"✅ Added BPMN file: {bpmn_file}")
        
        # List available processes
        processes = engine.list_processes()
        print(f"📋 Available processes: {list(processes.keys())}")
        
        # Get first process ID
        process_id = list(processes.keys())[0]
        print(f"🎯 Using process ID: {process_id}")
        
        # Create workflow with initial data
        initial_data = {
            "input_data": "Test data for enhanced telemetry",
            "test_parameter": "enhanced_test"
        }
        
        print(f"📊 Creating workflow with initial data: {initial_data}")
        workflow = engine.create_workflow(process_id, initial_data)
        
        # Get workflow ID from active workflows
        workflow_id = None
        for wf_id, context in engine.active_workflows.items():
            if context.workflow_instance == workflow:
                workflow_id = wf_id
                break
        
        if workflow_id:
            print(f"🆔 Workflow created with ID: {workflow_id}")
            
            # Get initial status
            status = engine.get_workflow_status(workflow_id)
            print(f"📈 Initial workflow status: {json.dumps(status, indent=2)}")
        
        # Execute workflow with enhanced telemetry
        print("\n🎯 Executing workflow with enhanced telemetry...")
        result = engine.execute_workflow(workflow, run_until_user_input=False)
        
        print(f"✅ Workflow execution completed!")
        print(f"📊 Final result: {json.dumps(result, indent=2)}")
        
        # Get final status
        if workflow_id:
            final_status = engine.get_workflow_status(workflow_id)
            if final_status:
                print(f"📈 Final workflow status: {json.dumps(final_status, indent=2)}")
            else:
                print("📈 Workflow completed and removed from active workflows")
        
        return True
        
    except Exception as e:
        print(f"❌ Error during enhanced telemetry test: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_task_level_execution():
    """Test individual task execution with telemetry"""
    print("\n🔧 Testing Task-Level Execution with Telemetry")
    print("=" * 50)
    
    # Create enhanced engine
    engine = create_enhanced_telemetry_engine()
    
    # Test BPMN file
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print(f"[ERROR] BPMN file not found: {bpmn_file}")
        return False
    
    try:
        # Add BPMN file
        engine.add_bpmn_file(bpmn_file)
        
        # Get first process ID
        processes = engine.list_processes()
        process_id = list(processes.keys())[0]
        
        # Create workflow
        workflow = engine.create_workflow(process_id, {"test_data": "task_level_test"})
        
        # Get workflow ID
        workflow_id = None
        for wf_id, context in engine.active_workflows.items():
            if context.workflow_instance == workflow:
                workflow_id = wf_id
                break
        
        if not workflow_id:
            print("❌ Could not find workflow ID")
            return False
        
        print(f"🆔 Executing tasks for workflow: {workflow_id}")
        
        # Execute tasks one by one with telemetry
        task_count = 0
        while not workflow.is_completed():
            ready_tasks = workflow.get_tasks(state=TaskState.READY)
            if not ready_tasks:
                break
            
            for task in ready_tasks:
                task_count += 1
                print(f"\n🔧 Executing task {task_count}: {task.task_spec.name}")
                
                try:
                    # Execute task with telemetry
                    engine.execute_task_with_telemetry(task, workflow_id)
                    print(f"✅ Task {task_count} completed successfully")
                    
                    # Get updated status
                    status = engine.get_workflow_status(workflow_id)
                    if status:
                        print(f"📈 Completed tasks: {status['completed_tasks']}")
                        print(f"📈 Failed tasks: {status['failed_tasks']}")
                    
                except Exception as e:
                    print(f"❌ Task {task_count} failed: {e}")
                    return False
        
        print(f"\n✅ All {task_count} tasks executed with telemetry")
        return True
        
    except Exception as e:
        print(f"❌ Error during task-level test: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_multiple_workflows():
    """Test multiple concurrent workflows with separate telemetry contexts"""
    print("\n🔄 Testing Multiple Concurrent Workflows")
    print("=" * 50)
    
    # Create enhanced engine
    engine = create_enhanced_telemetry_engine()
    
    # Test BPMN file
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print(f"[ERROR] BPMN file not found: {bpmn_file}")
        return False
    
    try:
        # Add BPMN file
        engine.add_bpmn_file(bpmn_file)
        
        # Get first process ID
        processes = engine.list_processes()
        process_id = list(processes.keys())[0]
        
        # Create multiple workflows
        workflows = []
        workflow_ids = []
        
        for i in range(3):
            workflow = engine.create_workflow(process_id, {f"workflow_num": i})
            workflows.append(workflow)
            
            # Get workflow ID
            for wf_id, context in engine.active_workflows.items():
                if context.workflow_instance == workflow:
                    workflow_ids.append(wf_id)
                    break
        
        print(f"🆔 Created {len(workflows)} workflows with IDs: {workflow_ids}")
        
        # Execute all workflows
        results = []
        for i, (workflow, workflow_id) in enumerate(zip(workflows, workflow_ids)):
            print(f"\n🎯 Executing workflow {i+1} (ID: {workflow_id})")
            
            try:
                result = engine.execute_workflow(workflow, run_until_user_input=False)
                results.append(result)
                print(f"✅ Workflow {i+1} completed successfully")
                
            except Exception as e:
                print(f"❌ Workflow {i+1} failed: {e}")
                return False
        
        print(f"\n✅ All {len(workflows)} workflows completed successfully")
        print(f"📊 Results summary: {len(results)} successful executions")
        
        return True
        
    except Exception as e:
        print(f"❌ Error during multiple workflows test: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("🚀 Enhanced Telemetry Engine Test Suite")
    print("=" * 60)
    
    # Test 1: Basic enhanced telemetry
    print("\n📊 Test 1: Basic Enhanced Telemetry")
    print("-" * 40)
    success1 = test_enhanced_telemetry_engine()
    print(f"[RESULT] Basic enhanced telemetry: {'✅ PASSED' if success1 else '❌ FAILED'}")
    
    # Test 2: Task-level execution
    print("\n📊 Test 2: Task-Level Execution")
    print("-" * 40)
    success2 = test_task_level_execution()
    print(f"[RESULT] Task-level execution: {'✅ PASSED' if success2 else '❌ FAILED'}")
    
    # Test 3: Multiple workflows
    print("\n📊 Test 3: Multiple Workflows")
    print("-" * 40)
    success3 = test_multiple_workflows()
    print(f"[RESULT] Multiple workflows: {'✅ PASSED' if success3 else '❌ FAILED'}")
    
    # Overall result
    overall_success = success1 and success2 and success3
    print(f"\n🎯 OVERALL RESULT: {'✅ ALL TESTS PASSED' if overall_success else '❌ SOME TESTS FAILED'}")
    
    exit(0 if overall_success else 1) 