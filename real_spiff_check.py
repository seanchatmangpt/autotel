#!/usr/bin/env python3
"""
Real Spiff Check - No BS
Just check if the actual features work
"""

print("🔍 REAL SPIFF CHECK")
print("=" * 30)

# Core SpiffWorkflow
try:
    import SpiffWorkflow
    print("✅ SpiffWorkflow available")
    print(f"   Version: {getattr(SpiffWorkflow, '__version__', 'unknown')}")
except ImportError:
    print("❌ SpiffWorkflow not installed")
    print("   Install: pip install SpiffWorkflow")
    exit(1)

# BPMN Support
try:
    from SpiffWorkflow.bpmn import BpmnWorkflow
    from SpiffWorkflow.bpmn.parser.BpmnParser import BpmnParser
    parser = BpmnParser()
    print("✅ BPMN workflows available")
except ImportError as e:
    print(f"❌ BPMN not available: {e}")
except Exception as e:
    print(f"❌ BPMN broken: {e}")

# DMN Decision Support  
try:
    from SpiffWorkflow.dmn.parser.BpmnDmnParser import BpmnDmnParser
    dmn_parser = BpmnDmnParser()
    print("✅ DMN decisions available")
except ImportError as e:
    print(f"❌ DMN not available: {e}")
except Exception as e:
    print(f"❌ DMN broken: {e}")

# Python Script Tasks
try:
    from SpiffWorkflow.bpmn.specs.bpmn_task_spec import BpmnTaskSpec
    print("✅ Python script tasks available")
except ImportError as e:
    print(f"❌ Script tasks not available: {e}")
except Exception as e:
    print(f"❌ Script tasks broken: {e}")

print("\nDone.")
