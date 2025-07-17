#!/usr/bin/env python3
"""
Simple Camunda example test - extracts core functionality from spiff-example-cli
without using runner.py or curses UI.
"""

import os
import sys
from pathlib import Path

# Add spiff-example-cli to path so we can import from it
sys.path.insert(0, str(Path(__file__).parent / "spiff-example-cli"))

from SpiffWorkflow.camunda.parser import CamundaParser

def test_camunda_parser():
    """Test CamundaParser with example BPMN and DMN files"""
    
    # Use example files from spiff-example-cli
    bpmn_file = "spiff-example-cli/bpmn/camunda/task_types.bpmn"
    dmn_file = "spiff-example-cli/bpmn/camunda/product_prices.dmn"
    
    print(f"Testing CamundaParser with:")
    print(f"  BPMN: {bpmn_file}")
    print(f"  DMN: {dmn_file}")
    print()
    
    # Check if files exist
    if not os.path.exists(bpmn_file):
        print(f"❌ BPMN file not found: {bpmn_file}")
        return False
    if not os.path.exists(dmn_file):
        print(f"❌ DMN file not found: {dmn_file}")
        return False
    
    try:
        # Create parser
        parser = CamundaParser()
        
        # Load DMN file first
        parser.add_dmn_file(dmn_file)
        print(f"✅ Loaded DMN file: {dmn_file}")
        
        # Load BPMN file
        parser.add_bpmn_file(bpmn_file)
        print(f"✅ Loaded BPMN file: {bpmn_file}")
        
        # Get process specs
        specs = parser.find_all_specs()
        print(f"📋 Found {len(specs)} process specifications")
        
        for process_id, spec in specs.items():
            print(f"  Process ID: {process_id}")
            print(f"  Process spec type: {type(spec)}")
            print(f"  Task specs count: {len(spec.task_specs)}")
        
        # Check DMN decisions
        print(f"\n📊 DMN Decisions:")
        for decision_id, dmn_parser in parser.dmn_parsers.items():
            dmn_parser.parse()
            decision = dmn_parser.decision
            print(f"  Decision ID: {decision.id}, Name: {decision.name}")
            print(f"  Decision tables: {len(decision.decisionTables)}")
        
        print("\n✅ CamundaParser test completed successfully!")
        print("✅ BPMN and DMN parsing validation passed!")
        return True
        
    except Exception as e:
        print(f"❌ Error testing CamundaParser: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    success = test_camunda_parser()
    sys.exit(0 if success else 1) 