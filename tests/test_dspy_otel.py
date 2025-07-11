#!/usr/bin/env python3
"""
Test script to demonstrate DSPy with Ollama interpreting OTEL spans
"""

import json
import dspy
from typing import Dict, Any

# Sample OTEL span data (similar to what AutoTel generates)
sample_otel_spans = {
    "resource": {
        "attributes": {
            "service.name": "autotel-service",
            "service.version": "1.0.0"
        }
    },
    "spans": [
        {
            "trace_id": "0x659b0fb63a4b6077e38650a3a7314dab",
            "span_id": "0x94bc7ecbe1ccbcfb",
            "name": "init_orchestrator",
            "start_time": "2024-01-15T10:30:00.000Z",
            "end_time": "2024-01-15T10:30:05.000Z",
            "attributes": {
                "process.id": "SimpleProcess",
                "instance.id": "test-instance-001",
                "instance.status": "running"
            },
            "events": [
                {
                    "name": "Loaded process definition: Process_1",
                    "timestamp": "2024-01-15T10:30:01.000Z"
                },
                {
                    "name": "Loaded process definition: SimpleProcess",
                    "timestamp": "2024-01-15T10:30:02.000Z"
                }
            ]
        },
        {
            "trace_id": "0x1234567890abcdef1234567890abcdef",
            "span_id": "0xabcdef1234567890abcdef1234567890",
            "name": "execute_process",
            "start_time": "2024-01-15T10:30:10.000Z",
            "end_time": "2024-01-15T10:30:15.000Z",
            "attributes": {
                "instance.id": "test-instance-001",
                "max.steps": 10
            },
            "status": {
                "code": "ERROR",
                "message": "Process execution failed"
            },
            "events": [
                {
                    "name": "exception",
                    "attributes": {
                        "exception.type": "AttributeError",
                        "exception.message": "'int' object has no attribute 'workflow'",
                        "exception.stacktrace": "Traceback (most recent call last):\n  File \"orchestrator.py\", line 45, in <module>\n    result = orchestrator.execute_process(instance_id)\nAttributeError: 'int' object has no attribute 'workflow'"
                    }
                }
            ]
        }
    ]
}

class OTELInterpreter(dspy.Signature):
    """Interpret OpenTelemetry spans and provide insights"""
    
    otel_spans = dspy.InputField(desc="OpenTelemetry span data in JSON format")
    interpretation = dspy.OutputField(desc="Human-friendly interpretation of the OTEL spans")

def test_dspy_ollama_otel():
    """Test DSPy with Ollama interpreting OTEL spans"""
    
    print("🚀 Testing DSPy with Ollama on OTEL spans...")
    print("=" * 60)
    
    # Configure DSPy to use Ollama via dspy.LM as per official docs
    try:
        ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.7)
        dspy.configure(lm=ollama_lm)
        print("✅ Connected to Ollama successfully via dspy.LM")
    except Exception as e:
        print(f"❌ Failed to connect to Ollama: {e}")
        return
    
    # Use the signature as a callable, not as an instance
    spans_json = json.dumps(sample_otel_spans, indent=2)
    
    print("\n📊 Sample OTEL Spans:")
    print("-" * 40)
    print(json.dumps(sample_otel_spans, indent=2))
    
    print("\n🤖 DSPy + Ollama Interpretation:")
    print("-" * 40)
    
    try:
        # Wrap the signature in a Predict module
        predict_module = dspy.Predict(OTELInterpreter)
        result = predict_module(otel_spans=spans_json)
        print("🎯 INTERPRETATION:")
        print(result.interpretation)
    except Exception as e:
        print(f"❌ Error during interpretation: {e}")
        print("This might be due to:")
        print("- Ollama server not running")
        print("- Model not available")
        print("- Network connectivity issues")

if __name__ == "__main__":
    test_dspy_ollama_otel() 