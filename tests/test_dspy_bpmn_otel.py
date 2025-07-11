#!/usr/bin/env python3
"""
Test DSPy BPMN integration: prove <dspy:service> in BPMN XML calls DSPy and interprets OTEL spans.
"""
from autotel.workflows.spiff import run_dspy_bpmn_process
import json

def main():
    # Minimal OTEL span input
    otel_spans = {
        "resource": {"attributes": {"service.name": "autotel-service"}},
        "spans": [
            {
                "name": "execute_process",
                "status": {"code": "ERROR", "message": "Process failed"},
                "events": [{"name": "exception", "attributes": {"exception.type": "AttributeError"}}]
            }
        ]
    }
    context = {"otel_spans_json": json.dumps(otel_spans)}
    result = run_dspy_bpmn_process("bpmn/otel_dspy_test.bpmn", "OtelDspyTest", context)
    print(json.loads(result["otel_interpretation"])['interpretation'])

if __name__ == "__main__":
    main() 