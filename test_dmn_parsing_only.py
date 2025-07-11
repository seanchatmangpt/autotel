#!/usr/bin/env python3
"""
Test DMN parsing directly without BPMN integration.
"""
import xml.etree.ElementTree as ET

def test_dmn_parsing():
    print("🧪 Testing DMN XML parsing directly")
    print("=" * 40)
    
    # Parse the DMN XML directly
    tree = ET.parse("bpmn/simple_dmn_only_example.bpmn")
    root = tree.getroot()
    
    # Find DMN definitions
    dmn_ns = {'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/'}
    dmn_definitions = root.findall('.//dmn:definitions', dmn_ns)
    
    print(f"📋 Found {len(dmn_definitions)} DMN definitions")
    
    for dmn_def in dmn_definitions:
        decisions = dmn_def.findall('.//dmn:decision', dmn_ns)
        print(f"📋 Found {len(decisions)} decisions in DMN definition")
        
        for decision in decisions:
            decision_id = decision.get('id')
            name = decision.get('name', decision_id)
            print(f"🔍 Decision: {name} (ID: {decision_id})")
            
            # Find decision table
            decision_table = decision.find('.//dmn:decisionTable', dmn_ns)
            if decision_table:
                print(f"   ✅ Has decision table")
                
                # Count rules
                rules = decision_table.findall('.//dmn:rule', dmn_ns)
                print(f"   📊 Has {len(rules)} rules")
                
                # Show first rule as example
                if rules:
                    first_rule = rules[0]
                    input_entries = first_rule.findall('.//dmn:inputEntry', dmn_ns)
                    output_entries = first_rule.findall('.//dmn:outputEntry', dmn_ns)
                    print(f"   📝 First rule: {len(input_entries)} inputs, {len(output_entries)} outputs")
    
    print("\n✅ DMN parsing test completed!")

if __name__ == "__main__":
    test_dmn_parsing() 