#!/usr/bin/env python3
"""
Test Ontology-Based DSPy Integration

Demonstrates the clean separation between:
- Processors: Parse XML → Data structures
- Compiler: Transform data → Execution-ready signatures
"""

from autotel.factory.processors.owl_processor import OWLProcessor
from autotel.factory.processors.shacl_processor import SHACLProcessor
from autotel.factory.processors.dspy_processor import DSPyProcessor
from autotel.factory.dspy_compiler import DSPyCompiler
from autotel.factory.linker import SemanticLinker


def test_ontology_dspy_integration():
    """Test the complete flow from XML to execution-ready DSPy signature."""
    
    # Test XML with OWL, SHACL, and DSPy definitions
    xml_fragment = """
    <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
             xmlns:owl="http://www.w3.org/2002/07/owl#"
             xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
        
        <!-- OWL Ontology -->
        <owl:Class rdf:about="#UserInput">
            <rdfs:label>User Input</rdfs:label>
            <rdfs:comment>Input data from user</rdfs:comment>
        </owl:Class>
        
        <owl:Class rdf:about="#Recommendation">
            <rdfs:label>Recommendation</rdfs:label>
            <rdfs:comment>AI-generated recommendation</rdfs:comment>
        </owl:Class>
        
        <owl:DataProperty rdf:about="#hasText">
            <rdfs:domain rdf:resource="#UserInput"/>
            <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
            <rdfs:label>has text</rdfs:label>
        </owl:DataProperty>
        
        <owl:DataProperty rdf:about="#hasConfidence">
            <rdfs:domain rdf:resource="#Recommendation"/>
            <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#float"/>
            <rdfs:label>has confidence</rdfs:label>
        </owl:DataProperty>
        
        <!-- SHACL Shapes -->
        <shacl:shapes>
            <rdf:RDF xmlns:shacl="http://www.w3.org/ns/shacl#">
                <shacl:NodeShape rdf:about="#UserInputShape">
                    <shacl:targetClass rdf:resource="#UserInput"/>
                    <shacl:property>
                        <shacl:PropertyShape>
                            <shacl:path rdf:resource="#hasText"/>
                            <shacl:minCount>1</shacl:minCount>
                            <shacl:maxCount>1</shacl:maxCount>
                        </shacl:PropertyShape>
                    </shacl:property>
                </shacl:NodeShape>
            </rdf:RDF>
        </shacl:shapes>
        
        <!-- DSPy Definitions -->
        <dspy:signature name="recommendation_signature" 
                       description="Generate recommendations from user input">
            <dspy:input name="user_input" description="User input text" optional="false"/>
            <dspy:output name="recommendation" description="AI recommendation"/>
        </dspy:signature>
        
        <dspy:model model="ollama/qwen3:latest" temperature="0.7" max_tokens="2000"/>
        
        <dspy:predict signature="recommendation_signature">
            <dspy:param name="max_length" value="100"/>
        </dspy:predict>
    </rdf:RDF>
    """
    
    print("=== Ontology-Based DSPy Integration Test ===\n")
    
    # Step 1: Parse XML with processors
    print("1. Parsing XML with processors...")
    
    owl_processor = OWLProcessor()
    shacl_processor = SHACLProcessor()
    dspy_processor = DSPyProcessor()
    
    # Parse OWL ontology
    ontology = owl_processor.parse_ontology_definition(xml_fragment, "test")
    print(f"   OWL: Found {len(ontology.classes)} classes, {len(ontology.object_properties)} object properties, {len(ontology.data_properties)} data properties")
    
    # Parse SHACL shapes
    shacl_graph = shacl_processor.parse(xml_fragment)
    print(f"   SHACL: Found {len(shacl_graph)} triples")
    
    # Parse DSPy definitions
    dspy_signatures = dspy_processor.parse(xml_fragment)
    dspy_modules = dspy_processor.parse_modules(xml_fragment)
    model_config = dspy_processor.parse_model_configuration(xml_fragment)
    
    print(f"   DSPy: Found {len(dspy_signatures)} signatures, {len(dspy_modules)} modules")
    if model_config:
        print(f"   Model: {model_config.provider}/{model_config.model_name}")
    
    print()
    
    # Step 2: Compile for execution
    print("2. Compiling for execution...")
    
    compiler = DSPyCompiler()
    signature = compiler.compile(
        ontology=ontology,
        shacl_graph=shacl_graph,
        dspy_signatures=dspy_signatures,
        dspy_modules=dspy_modules,
        model_config=model_config,
        signature_name="recommendation_signature"
    )
    
    print(f"   Compiled signature: {signature.name}")
    print(f"   Description: {signature.description}")
    print(f"   Inputs: {len(signature.inputs)}")
    print(f"   Outputs: {len(signature.outputs)}")
    print(f"   Validation rules: {len(signature.validation_rules)}")
    print(f"   Examples: {len(signature.examples)}")
    
    print()
    
    # Step 3: Show the execution-ready signature
    print("3. Execution-ready signature details:")
    print(f"   Name: {signature.name}")
    print(f"   Description: {signature.description}")
    
    if signature.inputs:
        print("   Inputs:")
        for input_name, input_schema in signature.inputs.items():
            print(f"     - {input_name}: {input_schema.get('type', 'unknown')} ({input_schema.get('semantic_type', 'unknown')})")
    
    if signature.outputs:
        print("   Outputs:")
        for output_name, output_schema in signature.outputs.items():
            print(f"     - {output_name}: {output_schema.get('type', 'unknown')} ({output_schema.get('semantic_type', 'unknown')})")
    
    if signature.validation_rules:
        print("   Validation Rules:")
        for rule in signature.validation_rules:
            print(f"     - {rule}")
    
    if signature.model_config:
        print(f"   Model: {signature.model_config.provider}/{signature.model_config.model_name}")
    
    if signature.module_config:
        print(f"   Module: {signature.module_config.module_type} using {signature.module_config.signature_name}")
    
    print()
    
    # Step 4: Link into executable system
    print("4. Linking into executable system...")
    
    linker = SemanticLinker()
    
    # Validate the system
    validation_report = linker.validate_system(signature, ontology)
    print(f"   Validation: {'✅ PASS' if validation_report['valid'] else '❌ FAIL'}")
    print(f"   Signature valid: {validation_report['signature_valid']}")
    print(f"   Inputs valid: {validation_report['inputs_valid']}")
    print(f"   Outputs valid: {validation_report['outputs_valid']}")
    print(f"   Model config valid: {validation_report['model_config_valid']}")
    
    # Link the system
    executable_system = linker.link(signature, ontology, shacl_graph)
    print(f"   Executable system created with {len(executable_system)} components")
    
    if 'semantic_context' in executable_system:
        context = executable_system['semantic_context']
        print(f"   Semantic context: {len(context['classes'])} classes, {len(context['properties'])} properties")
    
    if executable_system.get('shacl_validation'):
        print(f"   SHACL validation: {executable_system['shacl_triples']} triples")
    
    print("\n=== Test Complete ===")
    print("✅ Complete pattern achieved:")
    print("   - Processors: XML → Data objects")
    print("   - Compiler: Data objects → Execution-ready objects")
    print("   - Linker: Execution-ready objects → Executable system")
    print("   - No BPMN dependencies")
    print("   - Ontology-driven execution")


if __name__ == "__main__":
    test_ontology_dspy_integration() 