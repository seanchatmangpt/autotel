"""
Factory Boy Demo - Simple demonstration of dynamic test data generation

This test file demonstrates how to use Factory Boy to replace hardcoded test data
with dynamically generated content.
"""

import pytest
from factories import (
    BPMNXMLFactory, DMNXMLFactory, OWLXMLFactory, SHACLXMLFactory, DSPyXMLFactory,
    TelemetryDataFactory, WorkflowContextFactory
)


def test_factory_boy_demo():
    """Demonstrate Factory Boy usage for dynamic test data"""
    
    # Generate dynamic BPMN XML
    bpmn_factory = BPMNXMLFactory()
    bpmn_xml = bpmn_factory['xml_content']
    
    assert "<?xml version=" in bpmn_xml
    assert "<bpmn:definitions" in bpmn_xml
    assert "<bpmn:process" in bpmn_xml
    
    # Generate dynamic DMN XML
    dmn_factory = DMNXMLFactory()
    dmn_xml = dmn_factory['xml_content']
    
    assert "<?xml version=" in dmn_xml
    assert "<definitions" in dmn_xml
    assert "<decision" in dmn_xml
    
    # Generate dynamic OWL XML
    owl_factory = OWLXMLFactory()
    owl_xml = owl_factory['xml_content']
    
    assert "<?xml version=" in owl_xml
    assert "<rdf:RDF" in owl_xml
    assert "<owl:Ontology" in owl_xml
    
    # Generate dynamic SHACL XML
    shacl_factory = SHACLXMLFactory()
    shacl_xml = shacl_factory['xml_content']
    
    assert "<?xml version=" in shacl_xml
    assert "<rdf:RDF" in shacl_xml
    assert "<sh:NodeShape" in shacl_xml
    
    # Generate dynamic DSPy XML
    dspy_factory = DSPyXMLFactory()
    dspy_xml = dspy_factory['xml_content']
    
    assert "<?xml version=" in dspy_xml
    assert "<root xmlns:dspy=" in dspy_xml
    assert "<dspy:signature" in dspy_xml
    
    # Generate dynamic telemetry data
    telemetry_data = TelemetryDataFactory()
    
    assert 'trace_id' in telemetry_data
    assert 'span_id' in telemetry_data
    assert 'timestamp' in telemetry_data
    assert 'attributes' in telemetry_data
    
    # Generate dynamic workflow context
    workflow_context = WorkflowContextFactory()
    
    assert 'context_data' in workflow_context
    assert 'workflow_id' in workflow_context['context_data']
    assert 'timestamp' in workflow_context['context_data']
    assert 'status' in workflow_context['context_data']
    
    print("✅ All Factory Boy factories working correctly!")
    print(f"📊 Generated {len(bpmn_xml)} characters of BPMN XML")
    print(f"📊 Generated {len(dmn_xml)} characters of DMN XML")
    print(f"📊 Generated {len(owl_xml)} characters of OWL XML")
    print(f"📊 Generated {len(shacl_xml)} characters of SHACL XML")
    print(f"📊 Generated {len(dspy_xml)} characters of DSPy XML")
    print(f"📊 Generated telemetry data with {len(telemetry_data)} fields")
    print(f"📊 Generated workflow context with {len(workflow_context)} fields")


def test_factory_customization():
    """Demonstrate Factory Boy customization with parameters"""
    
    # Custom BPMN with specific task count
    bpmn_factory = BPMNXMLFactory(task_count=10)
    bpmn_xml = bpmn_factory['xml_content']
    
    # Count the number of tasks in the generated XML
    task_count = bpmn_xml.count('<bpmn:task')
    assert task_count == 10
    
    # Custom DSPy with specific signature count
    dspy_factory = DSPyXMLFactory(signature_count=5)
    dspy_xml = dspy_factory['xml_content']
    
    # Count the number of signatures
    signature_count = dspy_xml.count('<dspy:signature')
    assert signature_count == 5
    
    print("✅ Factory customization working correctly!")
    print(f"📊 Generated BPMN with {task_count} tasks")
    print(f"📊 Generated DSPy with {signature_count} signatures")


def test_factory_uniqueness():
    """Demonstrate that factories generate unique data"""
    
    # Generate multiple instances
    bpmn_1 = BPMNXMLFactory()['xml_content']
    bpmn_2 = BPMNXMLFactory()['xml_content']
    bpmn_3 = BPMNXMLFactory()['xml_content']
    
    # They should be different (though not guaranteed due to randomness)
    # At least some elements should be different
    assert bpmn_1 != bpmn_2 or bpmn_2 != bpmn_3 or bpmn_1 != bpmn_3
    
    # Test telemetry data uniqueness
    telemetry_1 = TelemetryDataFactory()
    telemetry_2 = TelemetryDataFactory()
    
    assert telemetry_1['trace_id'] != telemetry_2['trace_id']
    assert telemetry_1['span_id'] != telemetry_2['span_id']
    
    print("✅ Factory uniqueness working correctly!")
    print(f"📊 Generated unique trace IDs: {telemetry_1['trace_id'][:8]}... vs {telemetry_2['trace_id'][:8]}...")


def test_fixture_integration(sample_bpmn_xml, sample_dmn_xml, sample_telemetry_data):
    """Test that fixtures work with Factory Boy"""
    
    # This test uses the fixtures from conftest.py
    # which now use Factory Boy internally
    
    assert "<?xml version=" in sample_bpmn_xml
    assert "<?xml version=" in sample_dmn_xml
    assert 'trace_id' in sample_telemetry_data
    
    print("✅ Fixture integration working correctly!")
    print(f"📊 Fixture BPMN: {len(sample_bpmn_xml)} characters")
    print(f"📊 Fixture DMN: {len(sample_dmn_xml)} characters")
    print(f"📊 Fixture telemetry: {len(sample_telemetry_data)} fields")


if __name__ == "__main__":
    # Run the demo tests
    test_factory_boy_demo()
    test_factory_customization()
    test_factory_uniqueness()
    test_fixture_integration()
    print("\n🎉 Factory Boy integration complete!") 