#!/usr/bin/env python3
"""
Demo script showing AutoTel telemetry spans in action.
This script demonstrates how the processors create OpenTelemetry spans
with real dynamic data and write them to a file.
"""

import tempfile
import json
import os
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, SimpleSpanProcessor

from autotel.core.telemetry import create_telemetry_manager
from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.dmn_processor import DMNProcessor

def demo_telemetry_spans():
    """Demonstrate telemetry spans being created with real data."""
    
    # Create a temporary file to capture spans
    with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmpfile:
        print(f"📁 Writing telemetry spans to: {tmpfile.name}")
        
        # Create tracer provider that writes to our file
        tracer_provider = TracerProvider()
        exporter = ConsoleSpanExporter(out=tmpfile)
        tracer_provider.add_span_processor(SimpleSpanProcessor(exporter))
        
        # Create telemetry manager with our provider
        telemetry = create_telemetry_manager(
            service_name="autotel-demo",
            require_linkml_validation=False,
            tracer_provider=tracer_provider,
            span_exporter=exporter
        )
        
        print("🔧 Testing SHACL Processor...")
        # Test SHACL processor
        shacl_processor = SHACLProcessor()
        shacl_xml = '''
        <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                 xmlns:sh="http://www.w3.org/ns/shacl#">
            <sh:NodeShape rdf:about="http://example.com/Person">
                <sh:property>
                    <sh:PropertyShape>
                        <sh:path rdf:resource="http://example.com/name"/>
                        <sh:datatype rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
                    </sh:PropertyShape>
                </sh:property>
            </sh:NodeShape>
        </rdf:RDF>
        '''
        try:
            shacl_processor.parse(shacl_xml)
            print("   ✅ SHACL parsing successful")
        except Exception as e:
            print(f"   ⚠️  Expected error: {e}")
        
        print("🔧 Testing DSPy Processor...")
        # Test DSPy processor
        dspy_processor = DSPyProcessor()
        dspy_xml = '''
        <dspy:root xmlns:dspy="http://autotel.ai/dspy">
            <dspy:signature name="test_signature" description="A test signature">
                <dspy:input name="input1" type="string"/>
                <dspy:output name="output1" type="string"/>
            </dspy:signature>
        </dspy:root>
        '''
        try:
            dspy_processor.parse(dspy_xml)
            print("   ✅ DSPy parsing successful")
        except Exception as e:
            print(f"   ⚠️  Expected error: {e}")
        
        print("🔧 Testing OWL Processor...")
        # Test OWL processor
        owl_processor = OWLProcessor()
        owl_xml = '''
        <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                 xmlns:owl="http://www.w3.org/2002/07/owl#">
            <owl:Ontology rdf:about="http://example.com/ontology"/>
        </rdf:RDF>
        '''
        try:
            owl_processor.parse_ontology_definition(owl_xml)
            print("   ✅ OWL parsing successful")
        except Exception as e:
            print(f"   ⚠️  Expected error: {e}")
        
        print("🔧 Testing DMN Processor...")
        # Test DMN processor
        dmn_processor = DMNProcessor()
        dmn_xml = '''
        <definitions xmlns="https://www.omg.org/spec/DMN/20191111/MODEL/">
            <decision id="decision1" name="Test Decision">
                <decisionTable>
                    <input id="input1" label="Input 1"/>
                    <output id="output1" label="Output 1"/>
                </decisionTable>
            </decision>
        </definitions>
        '''
        try:
            dmn_processor.parse(dmn_xml)
            print("   ✅ DMN parsing successful")
        except Exception as e:
            print(f"   ⚠️  Expected error: {e}")
        
        # Force flush and shutdown
        tracer_provider.shutdown()
        tmpfile.flush()
        tmpfile.seek(0)
        
        # Read and display the spans
        spans_data = tmpfile.read()
        print(f"\n📊 Generated {len(spans_data)} bytes of telemetry data")
        
        # Parse and display span information
        spans = []
        for line in spans_data.strip().split('\n'):
            if line.strip():
                try:
                    span = json.loads(line)
                    spans.append(span)
                except json.JSONDecodeError:
                    continue
        
        print(f"📈 Created {len(spans)} telemetry spans:")
        print()
        
        # Group spans by processor type
        processor_spans = {}
        for span in spans:
            name = span.get('name', 'Unknown')
            if 'shacl' in name.lower():
                processor = 'SHACL'
            elif 'dspy' in name.lower():
                processor = 'DSPy'
            elif 'owl' in name.lower():
                processor = 'OWL'
            elif 'dmn' in name.lower():
                processor = 'DMN'
            else:
                processor = 'Other'
            
            if processor not in processor_spans:
                processor_spans[processor] = []
            processor_spans[processor].append(span)
        
        # Display summary by processor
        for processor, processor_span_list in processor_spans.items():
            print(f"🔹 {processor} Processor ({len(processor_span_list)} spans):")
            for span in processor_span_list:
                name = span.get('name', 'Unknown')
                operation_type = span.get('attributes', {}).get('operation_type', 'Unknown')
                success = span.get('attributes', {}).get('parse_success', 'Unknown')
                input_size = span.get('attributes', {}).get('input_size_bytes', 'Unknown')
                status = span.get('status', {}).get('status_code', 'UNSET')
                
                # Show additional metrics based on processor type
                if processor == 'SHACL' and 'triples_count' in span.get('attributes', {}):
                    triples = span['attributes']['triples_count']
                    namespaces = span['attributes']['namespaces_count']
                    print(f"   • {name}: {operation_type} - Success: {success}, Triples: {triples}, Namespaces: {namespaces}")
                elif processor == 'DSPy' and 'signatures_parsed' in span.get('attributes', {}):
                    signatures = span['attributes']['signatures_parsed']
                    print(f"   • {name}: {operation_type} - Success: {success}, Signatures: {signatures}")
                else:
                    print(f"   • {name}: {operation_type} - Success: {success}, Status: {status}")
            print()
        
        print(f"✅ Telemetry demo completed successfully!")
        print(f"📁 Full span data written to: {tmpfile.name}")
        
        # Clean up the temporary file
        tmpfile.close()
        os.unlink(tmpfile.name)

if __name__ == "__main__":
    demo_telemetry_spans() 