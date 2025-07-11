#!/usr/bin/env python3
"""
Test DMN-only evaluation using SpiffWorkflow's built-in DMN support.
This test is XML-driven - it reads the DMN decision table to determine test cases.
"""
from autotel.workflows.spiff import run_dspy_bpmn_process
import xml.etree.ElementTree as ET

def extract_test_cases_from_dmn(bpmn_file):
    """Extract test cases from the DMN decision table in the BPMN file"""
    tree = ET.parse(bpmn_file)
    root = tree.getroot()
    
    # Find DMN rules
    dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
    rules = root.findall('.//dmn:rule', dmn_ns)
    
    test_cases = []
    for rule in rules:
        # Get input value (product name)
        input_entry = rule.find('.//dmn:inputEntry', dmn_ns)
        if input_entry is not None:
            input_text = input_entry.find('.//dmn:text', dmn_ns)
            if input_text is not None and input_text.text:
                # Remove quotes from the input value
                product_name = input_text.text.strip('"')
                
                # Get output value (price)
                output_entry = rule.find('.//dmn:outputEntry', dmn_ns)
                if output_entry is not None:
                    output_text = output_entry.find('.//dmn:text', dmn_ns)
                    if output_text is not None and output_text.text:
                        expected_price = output_text.text.strip()
                        
                        test_cases.append({
                            "input_value": product_name,
                            "expected": expected_price
                        })
    
    return test_cases

def main():
    bpmn_file = "bpmn/simple_dmn_only_example.bpmn"
    
    # Extract test cases from the DMN decision table
    test_cases = extract_test_cases_from_dmn(bpmn_file)
    print(f"📋 Found {len(test_cases)} test cases from DMN decision table")
    
    for i, case in enumerate(test_cases, 1):
        print(f"\n🧪 Test case {i}: {case['input_value']}")
        context = {"product_name": case["input_value"]}
        
        try:
            # Run the workflow using SpiffWorkflow's built-in DMN
            result = run_dspy_bpmn_process(
                bpmn_file, 
                "SimpleDMNExample", 
                context
            )
            
            # Get the DMN result
            product_price = result.get("product_price")
            # Format to match expected string format (DMN returns numeric values)
            product_price_str = f"{product_price:.2f}" if product_price is not None else None
            print(f"   Input: {case['input_value']} => DMN result: {product_price_str} (expected: {case['expected']})")
            
            if product_price_str == case["expected"]:
                print("   ✅ PASS")
            else:
                print("   ❌ FAIL")
                
        except Exception as e:
            print(f"   ❌ Error: {e}")
            import traceback
            traceback.print_exc()
    
    print("\n🎉 DMN test completed!")

if __name__ == "__main__":
    main() 