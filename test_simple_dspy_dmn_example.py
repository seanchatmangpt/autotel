#!/usr/bin/env python3
"""
Simple BPMN + DMN + DSPy Example
Demonstrates a complete workflow using all three technologies:
1. DSPy analyzes data and provides insights
2. DMN makes decisions based on DSPy results
3. BPMN orchestrates the entire process
"""

import json
import sys
from pathlib import Path

# Add the project root to the path
sys.path.insert(0, str(Path(__file__).parent))

from autotel.workflows.spiff import run_dspy_bpmn_process
from autotel.workflows.dspy_bpmn_parser import DspyBpmnParser
from autotel.utils.dspy_services import dspy_registry

def test_simple_dspy_dmn_example():
    """Test the simple BPMN + DMN + DSPy integration"""
    print("🚀 Simple BPMN + DMN + DSPy Example")
    print("=" * 50)
    
    # Test data - different scenarios
    test_scenarios = [
        {
            "name": "High Confidence - Should Approve",
            "data": "Customer has excellent credit score (850), stable job for 10 years, and no late payments"
        },
        {
            "name": "Medium Confidence - Should Review", 
            "data": "Customer has good credit score (720), job for 2 years, one late payment 6 months ago"
        },
        {
            "name": "Low Confidence - Should Reject",
            "data": "Customer has poor credit score (580), new job, multiple late payments in last year"
        }
    ]
    
    for scenario in test_scenarios:
        print(f"\n📋 Testing: {scenario['name']}")
        print("-" * 40)
        
        # Set up context
        context = {
            "input_data": scenario["data"]
        }
        
        print(f"📥 Input: {scenario['data']}")
        
        try:
            # Run the workflow
            result = run_dspy_bpmn_process(
                "bpmn/simple_dspy_dmn_example.bpmn", 
                "SimpleDspyDmnExample", 
                context
            )
            
            print("✅ Workflow completed successfully!")
            print("📊 Results:")
            
            # Parse and display results
            if "analysis_result" in result:
                analysis = json.loads(result["analysis_result"])
                print(f"   🤖 DSPy Analysis: {analysis.get('analysis', 'N/A')}")
                print(f"   📈 Confidence: {analysis.get('confidence', 'N/A')}")
                print(f"   💡 Recommendation: {analysis.get('recommendation', 'N/A')}")
            
            if "Action" in result:
                print(f"   🎯 DMN Decision: {result['Action']}")
            
        except Exception as e:
            print(f"❌ Error: {e}")
            import traceback
            traceback.print_exc()

def test_dspy_signature_creation():
    """Test that DSPy signatures are created correctly from XML"""
    print("\n🧪 Testing DSPy Signature Creation")
    print("=" * 40)
    
    # Create parser and load BPMN file
    parser = DspyBpmnParser()
    parser.add_bpmn_file("bpmn/simple_dspy_dmn_example.bpmn")
    
    # Check signature definitions
    signatures = parser.list_signatures()
    print(f"📋 Found {len(signatures)} signature definitions:")
    
    for name, sig_def in signatures.items():
        print(f"\n🔍 Signature: {name}")
        print(f"   Description: {sig_def.description}")
        print(f"   Inputs: {list(sig_def.inputs.keys())}")
        print(f"   Outputs: {list(sig_def.outputs.keys())}")
    
    # Check dynamic signatures
    dynamic_sigs = parser.dynamic_signatures
    print(f"\n⚡ Created {len(dynamic_sigs)} dynamic DSPy signature classes:")
    for name in dynamic_sigs.keys():
        print(f"   ✅ {name}")
    
    # Register signatures
    dspy_registry.register_parser_signatures(parser.dynamic_signatures)
    
    # Test the signature directly
    print(f"\n🧪 Testing DSPy signature directly:")
    test_data = "Customer has excellent credit score (850), stable job for 10 years"
    try:
        result = dspy_registry.call_signature("AnalyzeData", data=test_data)
        print(f"   ✅ Success: {result}")
    except Exception as e:
        print(f"   ❌ Error: {e}")

def main():
    """Run the complete example"""
    try:
        # Test DSPy signature creation first
        test_dspy_signature_creation()
        
        # Test the complete workflow
        test_simple_dspy_dmn_example()
        
        print("\n🎉 All tests completed!")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main() 