#!/usr/bin/env python3
"""
Example usage of the AOT compiler pipeline
Demonstrates the complete "Reasoner IS the Build System" workflow
"""

import os
import json
import tempfile
from pathlib import Path
from aot_compiler_production import AOTCompilerProduction

def create_example_ontology():
    """Create a simple example ontology in TTL format"""
    ttl_content = """
@prefix : <http://example.org/cns#> .
@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix owl: <http://www.w3.org/2002/07/owl#> .
@prefix xsd: <http://www.w3.org/2001/XMLSchema#> .
@prefix eh: <http://cns.io/eightfold#> .

# Eightfold Path stages
eh:RightUnderstanding a owl:Class ;
    rdfs:label "Right Understanding" ;
    rdfs:comment "The first stage of the Eightfold Path - correct comprehension" .

eh:RightThought a owl:Class ;
    rdfs:label "Right Thought" ;
    rdfs:comment "The second stage - correct intention and planning" .

# CNS Core Classes
:CognitiveAgent a owl:Class ;
    rdfs:label "Cognitive Agent" ;
    rdfs:comment "An intelligent agent in the CNS network" ;
    eh:stage eh:RightUnderstanding .

:KnowledgeBase a owl:Class ;
    rdfs:label "Knowledge Base" ;
    rdfs:comment "Repository of structured knowledge" ;
    eh:stage eh:RightThought .

:ReasoningEngine a owl:Class ;
    rdfs:label "Reasoning Engine" ;
    rdfs:comment "Component that performs inference" ;
    eh:stage eh:RightAction .

# Properties
:hasName a owl:DatatypeProperty ;
    rdfs:label "has name" ;
    rdfs:domain :CognitiveAgent ;
    rdfs:range xsd:string ;
    rdfs:comment "The name of an agent" .

:hasKnowledge a owl:ObjectProperty ;
    rdfs:label "has knowledge" ;
    rdfs:domain :CognitiveAgent ;
    rdfs:range :KnowledgeBase ;
    rdfs:comment "Links an agent to its knowledge base" .

:performsReasoning a owl:ObjectProperty ;
    rdfs:label "performs reasoning" ;
    rdfs:domain :CognitiveAgent ;
    rdfs:range :ReasoningEngine ;
    rdfs:comment "Links an agent to its reasoning engine" .

# Functional property constraint
:hasName a owl:FunctionalProperty .

# Subclass relationships
:SpecializedAgent rdfs:subClassOf :CognitiveAgent ;
    rdfs:label "Specialized Agent" ;
    rdfs:comment "An agent with specialized capabilities" .
"""
    return ttl_content

def create_example_constraints():
    """Create example SHACL constraints"""
    return {
        'shapes': [
            {
                'target': 'http://example.org/cns#CognitiveAgent',
                'constraints': {
                    'properties': [
                        {
                            'path': 'http://example.org/cns#hasName',
                            'minCount': 1,
                            'maxCount': 1,
                            'datatype': 'http://www.w3.org/2001/XMLSchema#string',
                            'pattern': '^[A-Za-z][A-Za-z0-9_]*$'
                        },
                        {
                            'path': 'http://example.org/cns#hasKnowledge',
                            'minCount': 0,
                            'maxCount': 1
                        }
                    ],
                    'node': {
                        'nodeKind': 'IRI'
                    }
                }
            }
        ],
        'rules': [
            {
                'property': 'http://example.org/cns#hasName',
                'domain': 'http://example.org/cns#CognitiveAgent',
                'range': 'http://www.w3.org/2001/XMLSchema#string'
            }
        ]
    }

def run_example():
    """Run the complete AOT pipeline example"""
    print("🚀 CNS AOT Compiler Pipeline Example")
    print("=" * 50)
    
    # Create temporary directories
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        input_dir = temp_path / "input"
        output_dir = temp_path / "output"
        
        input_dir.mkdir()
        output_dir.mkdir()
        
        print(f"📁 Working directory: {temp_dir}")
        
        # Create example ontology file
        ontology_file = input_dir / "cns_example.ttl"
        ontology_file.write_text(create_example_ontology())
        print(f"📄 Created ontology: {ontology_file}")
        
        # Create configuration
        config = {
            'owl': {
                'strict_mode': True,
                'inference_enabled': True,
                'reasoning_depth': 3,
                'eightfold_integration': True
            },
            'shacl': {
                'validation_level': 'strict',
                'constraint_optimization': True,
                'generate_validation_code': True
            },
            'reasoning': {
                'max_cycles': 5,
                'parallel_reasoning': False,  # Simpler for example
                'proof_generation': True,
                'cognitive_model': '8H'
            },
            'cjinja': {
                'optimization_level': 2,
                'target_architecture': '8T',
                'memory_model': '8M',
                'telemetry_enabled': True,
                'generate_tests': True
            },
            'pipeline': {
                'parallel_compilation': False,  # Simpler for example
                'cache_enabled': True,
                'deterministic_output': True
            }
        }
        
        print("⚙️  Configuration loaded")
        
        # Initialize compiler
        print("🔧 Initializing AOT Compiler...")
        compiler = AOTCompilerProduction(config)
        
        # Compile the project
        print("🔄 Compiling project...")
        try:
            summary = compiler.compile_project(input_dir, output_dir)
            
            print("✅ Compilation successful!")
            print(f"📊 Summary:")
            print(f"   - Session ID: {summary['session_id']}")
            print(f"   - Files processed: {summary['total_files']}")
            print(f"   - Successful: {summary['successful']}")
            print(f"   - Failed: {summary['failed']}")
            print(f"   - Total time: {summary['total_time']:.2f}s")
            
            print(f"📈 Metrics:")
            for key, value in summary['metrics'].items():
                print(f"   - {key}: {value}")
            
            # Generate build script
            print("🔨 Generating build script...")
            build_script = compiler.generate_build_script(output_dir)
            print(f"📜 Build script: {build_script}")
            
            # Export compilation graph
            graph_file = output_dir / "compilation_graph.json"
            compiler.export_compilation_graph(graph_file)
            print(f"📊 Compilation graph: {graph_file}")
            
            # Show generated files
            print(f"📁 Generated files:")
            for file_path in output_dir.rglob("*"):
                if file_path.is_file():
                    size = file_path.stat().st_size
                    print(f"   - {file_path.name} ({size} bytes)")
            
            # Show a sample of generated C code
            c_files = list(output_dir.glob("*.c"))
            if c_files:
                print(f"📝 Sample generated C code ({c_files[0].name}):")
                print("-" * 40)
                content = c_files[0].read_text()
                lines = content.split('\n')
                for i, line in enumerate(lines[:20]):  # Show first 20 lines
                    print(f"{i+1:3}: {line}")
                if len(lines) > 20:
                    print(f"... ({len(lines) - 20} more lines)")
                print("-" * 40)
            
            # Show compilation summary
            summary_files = list(output_dir.glob("compilation_summary_*.json"))
            if summary_files:
                print(f"📋 Compilation Summary:")
                with open(summary_files[0]) as f:
                    summary_data = json.load(f)
                    print(json.dumps(summary_data, indent=2)[:500] + "...")
            
            # Test the build script (if safe)
            if build_script.exists():
                print(f"🧪 Testing build script...")
                print(f"   Build script is ready at: {build_script}")
                print(f"   To test: cd {output_dir} && ./build.sh")
            
            print("🎉 AOT Pipeline Example Complete!")
            print(f"🔍 Explore the generated files in: {output_dir}")
            
        except Exception as e:
            print(f"❌ Compilation failed: {e}")
            import traceback
            traceback.print_exc()

def demonstrate_components():
    """Demonstrate individual pipeline components"""
    print("\n🧩 Component Demonstration")
    print("=" * 30)
    
    # Demonstrate OWL Compiler
    print("1. OWL Compiler:")
    try:
        from owl_aot_compiler import OWLCompiler
        
        # Create a minimal TTL content in memory
        with tempfile.NamedTemporaryFile(mode='w', suffix='.ttl', delete=False) as f:
            f.write(create_example_ontology())
            ttl_file = Path(f.name)
        
        owl_compiler = OWLCompiler({'inference_enabled': True})
        result = owl_compiler.compile(ttl_file)
        
        print(f"   ✅ Parsed {result['statistics']['total_classes']} classes")
        print(f"   ✅ Parsed {result['statistics']['total_properties']} properties")
        print(f"   ✅ Eightfold coverage: {result['statistics']['eightfold_coverage']:.1f}%")
        
        # Cleanup
        os.unlink(ttl_file)
        
    except Exception as e:
        print(f"   ❌ OWL Compiler error: {e}")
    
    # Demonstrate SHACL Validator
    print("2. SHACL Validator:")
    try:
        from shacl_validator import SHACLValidator
        
        # Create test ontology and constraints
        test_ontology = {
            'classes': [{
                'uri': 'http://example.org/cns#CognitiveAgent',
                'label': 'CognitiveAgent',
                'properties': [
                    {'uri': 'hasName', 'label': 'hasName'},
                    {'uri': 'hasKnowledge', 'label': 'hasKnowledge'}
                ]
            }]
        }
        
        constraints = create_example_constraints()
        
        validator = SHACLValidator({'generate_validation_code': True})
        report = validator.validate(test_ontology, constraints)
        
        print(f"   ✅ Validation: {'PASSED' if report['valid'] else 'FAILED'}")
        print(f"   ✅ Optimized {report['constraints']['optimized']} constraints")
        
        # Generate validation code
        validation_code = validator.generate_validation_code()
        if validation_code:
            print(f"   ✅ Generated {len(validation_code)} bytes of validation code")
        
    except Exception as e:
        print(f"   ❌ SHACL Validator error: {e}")
    
    # Demonstrate Reasoning Engine
    print("3. Reasoning Engine:")
    try:
        from reasoning_engine import ReasoningEngine
        
        test_ontology = {
            'classes': [{
                'uri': 'http://example.org/cns#CognitiveAgent',
                'label': 'CognitiveAgent',
                'eightfold_mapping': {'stage': 'right_understanding'}
            }],
            'properties': [{
                'uri': 'http://example.org/cns#hasName',
                'label': 'hasName',
                'type': 'DatatypeProperty'
            }],
            'rules': [{
                'id': 'rule_1',
                'type': 'inference',
                'antecedent': [{'subject': '?x', 'predicate': 'rdf:type', 'object': 'CognitiveAgent'}],
                'consequent': {'subject': '?x', 'predicate': 'hasName', 'object': '?name'},
                'eightfold_stage': 'right_understanding'
            }]
        }
        
        engine = ReasoningEngine({'max_cycles': 3, 'parallel_reasoning': False})
        results = engine.reason(test_ontology, {'shapes': []})
        
        print(f"   ✅ Executed {len(results['cycles'])} reasoning cycles")
        print(f"   ✅ Generated {results['total_facts_derived']} derived facts")
        print(f"   ✅ Convergence: {'YES' if results['convergence_achieved'] else 'NO'}")
        
    except Exception as e:
        print(f"   ❌ Reasoning Engine error: {e}")
    
    # Demonstrate C Code Generator
    print("4. C Code Generator:")
    try:
        from cjinja_aot_compiler import CJinjaCompiler
        
        test_context = {
            'ontology': {
                'classes': [{
                    'uri': 'http://example.org/cns#CognitiveAgent',
                    'label': 'CognitiveAgent',
                    'properties': [
                        {'label': 'name', 'range': ['http://www.w3.org/2001/XMLSchema#string']},
                        {'label': 'id', 'range': ['http://www.w3.org/2001/XMLSchema#integer']}
                    ]
                }]
            },
            'constraints': {},
            'reasoning': [],
            'optimizations': {},
            'telemetry': {'points': []}
        }
        
        generator = CJinjaCompiler({'telemetry_enabled': True})
        code = generator.generate(test_context)
        
        print(f"   ✅ Generated {len(code)} bytes of C code")
        print(f"   ✅ Created {len(generator.code_blocks)} code blocks")
        
    except Exception as e:
        print(f"   ❌ C Code Generator error: {e}")

if __name__ == "__main__":
    print("🔬 CNS AOT Compiler Pipeline Demo")
    print("The Reasoner IS the Build System")
    print("=" * 50)
    
    # Run the main example
    run_example()
    
    # Demonstrate individual components
    demonstrate_components()
    
    print("\n🎯 Example complete!")
    print("💡 Try running the individual components with --test flag")
    print("📚 See README.md for detailed documentation")