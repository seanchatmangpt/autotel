#!/usr/bin/env python3
"""
Test for Orchestrator OTEL Integration
Shows the actual OpenTelemetry span output with task outputs
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.core.orchestrator import Orchestrator

def test_orchestrator_otel():
    """Test the orchestrator with OTEL tracing"""
    print("🚀 Testing Orchestrator with OTEL Tracing")
    print("=" * 50)
    
    # Create orchestrator with only the DSPy-only BPMN file
    orchestrator = Orchestrator(bpmn_files_path="bpmn_only", enable_telemetry=True)
    
    # Set up initial context
    initial_context = {
        "input_data": "Sample data for analysis"
    }
    
    print(f"📋 Initial context: {json.dumps(initial_context, indent=2)}")
    
    try:
        # Start a process - use the simple DSPy workflow
        process_id = "SimpleDspyWorkflow"
        instance = orchestrator.start_process(process_id, initial_context)
        print(f"✅ Started process instance: {instance.instance_id}")
        
        # Execute the process
        print(f"🎯 Executing process...")
        result = orchestrator.execute_process(instance.instance_id, max_steps=10)
        
        print(f"\n✅ Process completed!")
        print("=" * 30)
        print(f"📊 Final Results:")
        print(f"   Status: {result.status.value}")
        print(f"   Variables: {json.dumps(result.variables, indent=2)}")
        
        # The OTEL spans should be printed to console by ConsoleSpanExporter
        print(f"\n📋 Check the console output above for OTEL spans with task outputs!")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    test_orchestrator_otel() 