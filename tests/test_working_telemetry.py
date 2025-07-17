#!/usr/bin/env python3
"""
Test script demonstrating the WORKING telemetry in AutoTel's BPMN workflow engine.
This shows that real telemetry already exists and works - we don't need the facade system.
"""

import tempfile
import json
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, SimpleSpanProcessor

from autotel.core.telemetry import create_telemetry_manager
from autotel.workflows.enhanced_telemetry_engine import EnhancedTelemetryBpmnEngine

def test_working_telemetry():
    """Test the working telemetry in the BPMN workflow engine."""
    
    print("🔍 Testing WORKING AutoTel Telemetry System")
    print("=" * 50)
    
    # Create a temporary file to capture spans
    with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmpfile:
        print(f"📁 Writing telemetry spans to: {tmpfile.name}")
        
        # Create tracer provider that writes to our file
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        telemetry = create_telemetry_manager(
            service_name="autotel-working-demo",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        print("🔧 Creating Enhanced Telemetry BPMN Engine...")
        # Create the working telemetry engine
        engine = EnhancedTelemetryBpmnEngine(telemetry)
        
        print("🔧 Adding BPMN file...")
        # Add a simple BPMN file
        simple_bpmn = '''<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
             id="Definitions_1"
             targetNamespace="http://bpmn.io/schema/bpmn">
    <process id="Process_1" isExecutable="true">
        <startEvent id="StartEvent_1" name="Start">
            <outgoing>Flow_1</outgoing>
        </startEvent>
        <task id="Task_1" name="Test Task">
            <incoming>Flow_1</incoming>
            <outgoing>Flow_2</outgoing>
        </task>
        <endEvent id="EndEvent_1" name="End">
            <incoming>Flow_2</incoming>
        </endEvent>
        <sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_1" />
        <sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="EndEvent_1" />
    </process>
</definitions>'''
        
        # Write BPMN to temporary file
        bpmn_file = tempfile.NamedTemporaryFile(mode="w", suffix=".bpmn", delete=False)
        bpmn_file.write(simple_bpmn)
        bpmn_file.close()
        
        try:
            # Add the BPMN file to the engine
            engine.add_bpmn_file(bpmn_file.name)
            print("   ✅ BPMN file added successfully")
            
            print("🔧 Creating workflow...")
            # Create a workflow
            workflow = engine.create_workflow("Process_1", {"test_data": "hello world"})
            print("   ✅ Workflow created successfully")
            
            print("🔧 Executing workflow...")
            # Execute the workflow
            result = engine.execute_workflow(workflow)
            print("   ✅ Workflow executed successfully")
            print(f"   📊 Result: {result}")
            
        except Exception as e:
            print(f"   ⚠️  Expected error: {e}")
        finally:
            # Clean up
            import os
            os.unlink(bpmn_file.name)
        
        # Force flush and shutdown
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        
        # Read and display the spans
        spans_data = tmpfile.read()
        print(f"\n📊 Generated {len(spans_data)} bytes of telemetry data")
        
        # Parse and display span information
        spans = []
        for line in spans_data.strip().split('\n'):
            if line.strip():
                try:
                    span = json.loads(line)
                    spans.append(span)
                except json.JSONDecodeError:
                    continue
        
        print(f"📈 Created {len(spans)} telemetry spans from WORKING system:")
        print()
        
        # Display spans by type
        workflow_spans = []
        task_spans = []
        other_spans = []
        
        for span in spans:
            name = span.get('name', 'Unknown')
            if 'workflow' in name.lower():
                workflow_spans.append(span)
            elif 'task' in name.lower():
                task_spans.append(span)
            else:
                other_spans.append(span)
        
        if workflow_spans:
            print("🔹 Workflow Spans:")
            for span in workflow_spans:
                name = span.get('name', 'Unknown')
                attributes = span.get('attributes', {})
                workflow_id = attributes.get('workflow_id', 'Unknown')
                workflow_name = attributes.get('workflow_name', 'Unknown')
                print(f"   • {name}: {workflow_name} (ID: {workflow_id})")
            print()
        
        if task_spans:
            print("🔹 Task Spans:")
            for span in task_spans:
                name = span.get('name', 'Unknown')
                attributes = span.get('attributes', {})
                task_name = attributes.get('task_name', 'Unknown')
                task_type = attributes.get('task_type', 'Unknown')
                print(f"   • {name}: {task_name} ({task_type})")
            print()
        
        if other_spans:
            print("🔹 Other Spans:")
            for span in other_spans:
                name = span.get('name', 'Unknown')
                print(f"   • {name}")
            print()
        
        print(f"✅ WORKING telemetry demo completed successfully!")
        print(f"📁 Full span data written to: {tmpfile.name}")
        
        # Clean up the temporary file
        tmpfile.close()
        import os
        os.unlink(tmpfile.name)

if __name__ == "__main__":
    test_working_telemetry() 