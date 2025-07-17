#!/usr/bin/env python3
"""
AutoTel Unified Processor Generator

Uses the Jinja processor to generate unified processors from templates.
Demonstrates eating our own dog food by using our processor architecture.
"""

import json
from pathlib import Path
from typing import Dict, Any

from autotel.factory.processors.jinja_processor import JinjaProcessor
from autotel.processors import ProcessorConfig


def load_processor_specs() -> Dict[str, Dict[str, Any]]:
    """Load processor specifications for generation."""
    return {
        "BPMNProcessor": {
            "processor_name": "BPMNProcessor",
            "processor_description": "Parses BPMN 2.0 XML into SpiffWorkflow WorkflowSpec objects",
            "processor_capabilities": ["bpmn_parse", "workflow_spec", "xml_processing"],
            "supported_formats": ["xml", "bpmn"],
            "dependencies": ["SpiffWorkflow", "lxml"],
            "imports": [
                "from typing import Optional",
                "from SpiffWorkflow.camunda.parser import CamundaParser",
                "from SpiffWorkflow.bpmn.parser.ValidationException import ValidationException",
                "from SpiffWorkflow.specs import WorkflowSpec",
                "import lxml.etree as etree"
            ],
            "config_settings": {
                "parser_timeout": 30,
                "validate_schema": True,
                "ignore_custom_dsls": True
            },
            "process_impl_logic": "Parse BPMN XML and return WorkflowSpec",
            "error_handling": "Handle XML parsing, validation, and missing process errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "process_id"]
        },
        "DMNProcessor": {
            "processor_name": "DMNProcessor",
            "processor_description": "Parses DMN 1.3 XML into SpiffWorkflow decision objects",
            "processor_capabilities": ["dmn_parse", "decision_extraction", "xml_processing"],
            "supported_formats": ["xml", "dmn"],
            "dependencies": ["SpiffWorkflow", "lxml"],
            "imports": [
                "from typing import Dict, Any",
                "from SpiffWorkflow.camunda.parser import CamundaParser",
                "import lxml.etree as etree"
            ],
            "config_settings": {
                "parser_timeout": 30,
                "validate_schema": True,
                "extract_decisions": True
            },
            "process_impl_logic": "Parse DMN XML and extract decision objects",
            "error_handling": "Handle XML parsing and decision extraction errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "decisions_found"]
        },
        "DSPyProcessor": {
            "processor_name": "DSPyProcessor",
            "processor_description": "Parses DSPy XML into signature, module, and model definitions",
            "processor_capabilities": ["dspy_parse", "signature_extraction", "module_extraction", "validation_rule_extraction"],
            "supported_formats": ["xml", "dspy"],
            "dependencies": [],
            "imports": [
                "import xml.etree.ElementTree as ET",
                "from typing import Dict, List, Any, Optional"
            ],
            "config_settings": {
                "parse_signatures": True,
                "parse_modules": True,
                "parse_model_config": True,
                "extract_validation_rules": True
            },
            "process_impl_logic": "Parse DSPy XML and extract definitions",
            "error_handling": "Handle XML parsing and definition extraction errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "signatures_found", "modules_found"]
        },
        "SHACLProcessor": {
            "processor_name": "SHACLProcessor",
            "processor_description": "Parses SHACL XML into constraint graphs and validation rules",
            "processor_capabilities": ["shacl_parse", "constraint_validation", "pluggable_validation"],
            "supported_formats": ["xml", "shacl", "rdf"],
            "dependencies": ["rdflib"],
            "imports": [
                "import xml.etree.ElementTree as ET",
                "from typing import Dict, Any, List, Tuple, Optional, Callable",
                "from rdflib import Graph, Namespace, URIRef, Literal",
                "from rdflib.namespace import RDF, XSD",
                "import time",
                "import re",
                "from functools import lru_cache"
            ],
            "config_settings": {
                "enable_caching": True,
                "pluggable_validators": True,
                "performance_metrics": True
            },
            "process_impl_logic": "Parse SHACL XML and generate validation rules",
            "error_handling": "Handle XML parsing, SHACL validation, and constraint errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "constraints_found", "validation_rules_generated"]
        },
        "OWLProcessor": {
            "processor_name": "OWLProcessor",
            "processor_description": "Parses OWL/RDF XML into ontology definitions",
            "processor_capabilities": ["owl_parse", "ontology_extraction"],
            "supported_formats": ["xml", "owl", "rdf"],
            "dependencies": [],
            "imports": [
                "import xml.etree.ElementTree as ET",
                "from typing import Dict, Any, List, Optional",
                "import re"
            ],
            "config_settings": {
                "extract_classes": True,
                "extract_properties": True,
                "extract_individuals": True,
                "extract_axioms": True
            },
            "process_impl_logic": "Parse OWL XML and extract ontology components",
            "error_handling": "Handle XML parsing and ontology extraction errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "classes_found", "properties_found"]
        },
        "JinjaProcessor": {
            "processor_name": "JinjaProcessor",
            "processor_description": "Parses Jinja2 templates from XML and renders with variables",
            "processor_capabilities": ["jinja_parse", "template_render", "template_validation"],
            "supported_formats": ["xml", "jinja"],
            "dependencies": ["jinja2"],
            "imports": [
                "import json",
                "import time",
                "from dataclasses import dataclass, field",
                "from pathlib import Path",
                "from typing import Any, Dict, List, Optional, Union",
                "from xml.etree import ElementTree as ET",
                "from jinja2 import Environment, Template, TemplateError, UndefinedError"
            ],
            "config_settings": {
                "trim_blocks": True,
                "lstrip_blocks": True,
                "custom_filters": True
            },
            "process_impl_logic": "Parse Jinja templates and render with variables",
            "error_handling": "Handle XML parsing, Jinja errors, and validation",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "templates_found", "rendering_success"]
        },
        "OTELProcessor": {
            "processor_name": "OTELProcessor",
            "processor_description": "Parses OpenTelemetry data (spans, metrics, logs, traces) from JSON",
            "processor_capabilities": ["otel_parse", "telemetry_analysis", "telemetry_conversion"],
            "supported_formats": ["json", "otel"],
            "dependencies": [],
            "imports": [
                "import json",
                "import time",
                "import logging",
                "from datetime import datetime",
                "from typing import Dict, List, Any, Optional, Union",
                "from dataclasses import dataclass, field",
                "from pathlib import Path"
            ],
            "config_settings": {
                "parse_spans": True,
                "parse_metrics": True,
                "parse_logs": True,
                "analyze_traces": True
            },
            "process_impl_logic": "Parse OTEL data and perform analysis",
            "error_handling": "Handle JSON parsing, telemetry, and analysis errors",
            "telemetry_attributes": ["input_size_bytes", "parse_success", "spans_found", "metrics_found"]
        }
    }


def generate_unified_processors():
    """Generate unified processors using Jinja processor."""
    print("🚀 AutoTel Unified Processor Generator")
    print("=" * 50)
    
    # Initialize Jinja processor
    jinja_processor = JinjaProcessor()
    
    # Load processor specifications
    processor_specs = load_processor_specs()
    
    # Load the template
    template_path = Path("templates/processor_generator.xml")
    if not template_path.exists():
        print(f"❌ Template not found: {template_path}")
        return
    
    with open(template_path, 'r', encoding='utf-8') as f:
        template_content = f.read()
    
    # Create output directory
    output_dir = Path("autotel/processors/unified")
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Generate each processor
    for processor_name, spec in processor_specs.items():
        print(f"\n📝 Generating {processor_name}...")
        
        # Prepare variables for template rendering
        variables = {
            **spec,
            "class_name": processor_name,
            "file_name": f"{processor_name.lower()}.py"
        }
        
        try:
            # Process the template
            result = jinja_processor.process_templates(template_content, variables)
            
            if result.success:
                # Get the rendered content from the first template
                if result.rendering_results:
                    rendered_content = result.rendering_results[0].rendered_content
                    
                    # Write to file
                    output_file = output_dir / f"{processor_name.lower()}.py"
                    with open(output_file, 'w', encoding='utf-8') as f:
                        f.write(rendered_content)
                    
                    print(f"✅ Generated: {output_file}")
                    print(f"   - Capabilities: {', '.join(spec['processor_capabilities'])}")
                    print(f"   - Formats: {', '.join(spec['supported_formats'])}")
                else:
                    print(f"❌ No rendering results for {processor_name}")
            else:
                print(f"❌ Failed to generate {processor_name}:")
                for error in result.errors:
                    print(f"   - {error}")
                    
        except Exception as e:
            print(f"❌ Error generating {processor_name}: {e}")
    
    # Generate __init__.py for the unified processors
    generate_init_file(output_dir, processor_specs.keys())
    
    print(f"\n🎉 Processor generation complete!")
    print(f"📁 Output directory: {output_dir}")
    print(f"📊 Generated {len(processor_specs)} processors")


def generate_init_file(output_dir: Path, processor_names):
    """Generate __init__.py file for unified processors."""
    init_content = '''"""
AutoTel Unified Processors

Generated processors using unified ABC/metaclass architecture.
All processors inherit from BaseProcessor with consistent interfaces.
"""

'''
    
    # Add imports
    for processor_name in processor_names:
        init_content += f"from .{processor_name.lower()} import {processor_name}\n"
    
    init_content += "\n__all__ = [\n"
    for processor_name in processor_names:
        init_content += f'    "{processor_name}",\n'
    init_content += "]\n"
    
    # Write __init__.py
    init_file = output_dir / "__init__.py"
    with open(init_file, 'w', encoding='utf-8') as f:
        f.write(init_content)
    
    print(f"✅ Generated: {init_file}")


if __name__ == "__main__":
    generate_unified_processors() 