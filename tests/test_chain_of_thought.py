import dspy
ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.7)
dspy.configure(lm=ollama_lm)
from autotel.factory.pipeline import AutoTelPipeline
from autotel.core.telemetry import TelemetryManager

def test_chain_of_thought():
    bpmn_file = "examples/chain_of_thought_example.bpmn"
    telemetry = TelemetryManager()
    pipeline = AutoTelPipeline(telemetry_manager=telemetry)
    result = pipeline.execute_workflow(
        bpmn_files=[bpmn_file],
        dmn_files=[],
        variables={
            "user_input": "I want a recommendation for a healthy lunch."
        }
    )
    print("Workflow result:", result["variables"])
    print("\n--- OpenTelemetry Trace ---")
    import json
    print(json.dumps(result["telemetry"], indent=2))

if __name__ == "__main__":
    test_chain_of_thought() 