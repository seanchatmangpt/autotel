#!/usr/bin/env python3
"""
Simple capability test for BPMN tasks in SpiffWorkflow.
Tests what actually works in the current implementation.
"""

from pathlib import Path
from autotel.core.telemetry import TelemetryManager, TelemetryConfig
from functools import wraps
from opentelemetry import trace
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor
import xml.etree.ElementTree as ET

# Force OTEL tracer provider setup at script start
trace.set_tracer_provider(TracerProvider())
trace.get_tracer_provider().add_span_processor(BatchSpanProcessor(ConsoleSpanExporter()))

def otel_wrapper_factory(attr_value, name):
    @wraps(attr_value)
    def otel_wrapped(self, *args, **kwargs):
        if not hasattr(self, "telemetry_manager"):
            self.telemetry_manager = TelemetryManager(TelemetryConfig(require_linkml_validation=False))
        tracer = self.telemetry_manager.tracer
        print(f"[OTEL] Starting span: {name}.{attr_value.__name__}")
        with tracer.start_as_current_span(f"{name}.{attr_value.__name__}"):
            result = attr_value(self, *args, **kwargs)
        print(f"[OTEL] Finished span: {name}.{attr_value.__name__}")
        return result
    return otel_wrapped

# --- OtelAutoMeta metaclass ---
class OtelAutoMeta(type):
    def __new__(mcs, name, bases, namespace):
        for attr_name, attr_value in list(namespace.items()):
            if callable(attr_value) and not attr_name.startswith("_"):
                namespace[attr_name] = otel_wrapper_factory(attr_value, name)
        cls = super().__new__(mcs, name, bases, namespace)
        if hasattr(cls, "run_all"):
            orig = getattr(cls, "run_all")
            if not getattr(orig, "__wrapped__", False):
                setattr(cls, "run_all", otel_wrapper_factory(orig, name))
        return cls


def get_first_process_id(bpmn_file):
    tree = ET.parse(bpmn_file)
    root = tree.getroot()
    ns = {'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL'}
    process = root.find('bpmn:process', ns)
    if process is not None:
        return process.attrib['id']
    return None


def test_spiff_capability():
    """Test what actually works in SpiffWorkflow"""
    bpmn_file = "bpmn/simple_process.bpmn"
    if not Path(bpmn_file).exists():
        print("[ERROR] BPMN file not found.")
        return False

    from SpiffWorkflow.bpmn.workflow import BpmnWorkflow
    from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser

    class OtelBpmnWorkflow(BpmnWorkflow, metaclass=OtelAutoMeta):
        pass

    parser = BpmnParser()
    parser.add_bpmn_file(bpmn_file)
    specs = parser.find_all_specs()
    process_id = get_first_process_id(bpmn_file)
    if not process_id or process_id not in specs:
        print(f"[ERROR] Process id '{process_id}' not found in specs.")
        return False
    workflow_spec = specs[process_id]
    workflow = OtelBpmnWorkflow(workflow_spec)
    print("[TEST] About to run workflow.run_all()...")
    workflow.run_all()
    print("[TEST] run_all() complete.")
    return workflow.is_completed()


if __name__ == "__main__":
    success = test_spiff_capability()
    print(f"[RESULT] Workflow completed: {success}")
    exit(0 if success else 1) 