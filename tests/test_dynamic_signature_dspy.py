import dspy
import xml.etree.ElementTree as ET
from typing import Dict, Any, Type, Union
import pyshacl
from rdflib import Graph, Namespace, Literal, URIRef
from rdflib.namespace import RDF, XSD, SH, OWL, RDFS
from rdflib.plugins.sparql import prepareQuery

# Import the proper OWL processor
from autotel.factory.processors.owl_processor import OWLProcessor

# XML data with SHACL constraints, OWL ontology, and model configuration
xml_data = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  xmlns:owl="http://www.w3.org/2002/07/owl#"
                  xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
                  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                  targetNamespace="http://autotel.ai/workflows">

  <dspy:model name="qwen3:latest" provider="ollama"/>

  <!-- OWL Ontology Definition -->
  <owl:Ontology about="http://autotel.ai/dspy/ontology">
    <rdf:RDF xmlns="http://autotel.ai/dspy#"
             xmlns:owl="http://www.w3.org/2002/07/owl#"
             xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
             xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#"
             xmlns:xsd="http://www.w3.org/2001/XMLSchema#">
      
      <owl:Ontology rdf:about="http://autotel.ai/dspy/ontology">
        <rdfs:label>AutoTel DSPy Ontology</rdfs:label>
        <rdfs:comment>Ontology for DSPy signature validation and reasoning</rdfs:comment>
      </owl:Ontology>

      <!-- OWL Classes -->
      <owl:Class rdf:about="http://autotel.ai/dspy#Recommendation">
        <rdfs:label>Recommendation</rdfs:label>
        <rdfs:comment>A structured recommendation with reasoning</rdfs:comment>
      </owl:Class>

      <owl:Class rdf:about="http://autotel.ai/dspy#Reasoning">
        <rdfs:label>Reasoning</rdfs:label>
        <rdfs:comment>Logical reasoning or explanation</rdfs:comment>
      </owl:Class>

      <owl:Class rdf:about="http://autotel.ai/dspy#UserInput">
        <rdfs:label>UserInput</rdfs:label>
        <rdfs:comment>User-provided input for processing</rdfs:comment>
      </owl:Class>

      <!-- OWL Object Properties -->
      <owl:ObjectProperty rdf:about="http://autotel.ai/dspy#hasReasoning">
        <rdfs:domain rdf:resource="http://autotel.ai/dspy#Recommendation"/>
        <rdfs:range rdf:resource="http://autotel.ai/dspy#Reasoning"/>
        <rdfs:label>has reasoning</rdfs:label>
      </owl:ObjectProperty>

      <owl:ObjectProperty rdf:about="http://autotel.ai/dspy#derivedFrom">
        <rdfs:domain rdf:resource="http://autotel.ai/dspy#Recommendation"/>
        <rdfs:range rdf:resource="http://autotel.ai/dspy#UserInput"/>
        <rdfs:label>derived from</rdfs:label>
      </owl:ObjectProperty>

      <!-- OWL Data Properties -->
      <owl:DatatypeProperty rdf:about="http://autotel.ai/dspy#hasContent">
        <rdfs:domain rdf:resource="http://autotel.ai/dspy#Recommendation"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <rdfs:label>has content</rdfs:label>
      </owl:DatatypeProperty>

      <owl:DatatypeProperty rdf:about="http://autotel.ai/dspy#hasExplanation">
        <rdfs:domain rdf:resource="http://autotel.ai/dspy#Reasoning"/>
        <rdfs:range rdf:resource="http://www.w3.org/2001/XMLSchema#string"/>
        <rdfs:label>has explanation</rdfs:label>
      </owl:DatatypeProperty>
      
    </rdf:RDF>
  </owl:Ontology>

  <dspy:signatures>
    <dspy:signature name="ExplainChoice" description="Explain a simple AI choice">
      <dspy:input name="user_input" description="User's input" owl:class="http://autotel.ai/dspy#UserInput">
        <shacl:PropertyShape>
          <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
          <shacl:minLength>1</shacl:minLength>
          <shacl:maxLength>1000</shacl:maxLength>
        </shacl:PropertyShape>
      </dspy:input>
      <dspy:output name="recommendation" description="AI recommendation" owl:class="http://autotel.ai/dspy#Recommendation">
        <shacl:PropertyShape>
          <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
          <shacl:minLength>10</shacl:minLength>
        </shacl:PropertyShape>
      </dspy:output>
      <dspy:output name="why" description="Reasoning for the recommendation" owl:class="http://autotel.ai/dspy#Reasoning">
        <shacl:PropertyShape>
          <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
          <shacl:minLength>20</shacl:minLength>
        </shacl:PropertyShape>
      </dspy:output>
    </dspy:signature>
  </dspy:signatures>

  <bpmn:process id="simple_dspy_process" isExecutable="true">
    <bpmn:startEvent id="start"/>
    <bpmn:sequenceFlow id="flow1" sourceRef="start" targetRef="ai_task"/>
    <bpmn:serviceTask id="ai_task" name="AI Decision">
      <bpmn:extensionElements>
        <dspy:chain-of-thought signature="ExplainChoice" resultVariable="recommendation">
          <dspy:param name="user_input" value="#{user_input}"/>
        </dspy:chain-of-thought>
      </bpmn:extensionElements>
    </bpmn:serviceTask>
    <bpmn:sequenceFlow id="flow2" sourceRef="ai_task" targetRef="end"/>
    <bpmn:endEvent id="end"/>
  </bpmn:process>

</bpmn:definitions>'''

# SHACL to Python type mapping
SHACL_TYPE_MAP = {
    "http://www.w3.org/2001/XMLSchema#string": str,
    "http://www.w3.org/2001/XMLSchema#integer": int,
    "http://www.w3.org/2001/XMLSchema#float": float,
    "http://www.w3.org/2001/XMLSchema#boolean": bool,
    "http://www.w3.org/2001/XMLSchema#date": str,  # Could be datetime.date
    "http://www.w3.org/2001/XMLSchema#dateTime": str,  # Could be datetime.datetime
}

def extract_shacl_constraints(property_elem) -> Dict[str, Any]:
    """Extract SHACL constraints from a property element"""
    constraints = {}
    
    # Find PropertyShape
    prop_shape = property_elem.find('.//{http://www.w3.org/ns/shacl#}PropertyShape')
    if prop_shape is None:
        return constraints
    
    # Extract datatype
    datatype_elem = prop_shape.find('.//{http://www.w3.org/ns/shacl#}datatype')
    if datatype_elem is not None:
        constraints['datatype'] = datatype_elem.text
    
    # Extract minLength
    min_length_elem = prop_shape.find('.//{http://www.w3.org/ns/shacl#}minLength')
    if min_length_elem is not None:
        constraints['minLength'] = int(min_length_elem.text)
    
    # Extract maxLength
    max_length_elem = prop_shape.find('.//{http://www.w3.org/ns/shacl#}maxLength')
    if max_length_elem is not None:
        constraints['maxLength'] = int(max_length_elem.text)
    
    return constraints

def shacl_to_python_type(constraints: Dict[str, Any]) -> Type:
    """Convert SHACL constraints to Python type"""
    datatype = constraints.get('datatype')
    if datatype in SHACL_TYPE_MAP:
        return SHACL_TYPE_MAP[datatype]
    return str  # Default to string

def format_shacl_constraints_for_llm(constraints: Dict[str, Any]) -> str:
    """Format SHACL constraints in a way the LLM can understand"""
    if not constraints:
        return "no constraints"
    
    parts = []
    
    # Datatype
    datatype = constraints.get('datatype', '')
    if datatype:
        # Extract just the type name from the full URI
        type_name = datatype.split('#')[-1] if '#' in datatype else datatype
        parts.append(f"type: {type_name}")
    
    # Length constraints
    if 'minLength' in constraints:
        parts.append(f"minimum length: {constraints['minLength']} characters")
    if 'maxLength' in constraints:
        parts.append(f"maximum length: {constraints['maxLength']} characters")
    
    return ", ".join(parts)

def create_enhanced_description(base_description: str, inputs: list, outputs: list, 
                              input_constraints: Dict, output_constraints: Dict) -> str:
    """Create an enhanced description with SHACL constraints for the LLM"""
    enhanced = base_description + "\n\n"
    
    if inputs:
        enhanced += "Inputs:\n"
        for input_name in inputs:
            constraints = input_constraints.get(input_name, {})
            constraint_desc = format_shacl_constraints_for_llm(constraints)
            enhanced += f"- {input_name}: {constraint_desc}\n"
    
    if outputs:
        enhanced += "\nOutputs (must meet these constraints):\n"
        for output_name in outputs:
            constraints = output_constraints.get(output_name, {})
            constraint_desc = format_shacl_constraints_for_llm(constraints)
            enhanced += f"- {output_name}: {constraint_desc}\n"
    
    enhanced += "\nEnsure all outputs meet their specified constraints."
    return enhanced

def create_shacl_graph_from_constraints(output_constraints: Dict[str, Dict]) -> Graph:
    """Create a SHACL graph from constraints for validation"""
    g = Graph()
    DSPY = Namespace("http://autotel.ai/dspy#")
    g.bind("dspy", DSPY)
    g.bind("sh", SH)
    g.bind("xsd", XSD)
    
    # Create a shape for each output
    for output_name, constraints in output_constraints.items():
        shape_uri = DSPY[f"{output_name}Shape"]
        property_uri = DSPY[output_name]
        
        # Create PropertyShape
        g.add((shape_uri, RDF.type, SH.PropertyShape))
        g.add((shape_uri, SH.path, property_uri))
        
        # Add datatype constraint
        if 'datatype' in constraints:
            datatype_uri = URIRef(constraints['datatype'])
            g.add((shape_uri, SH.datatype, datatype_uri))
        
        # Add minLength constraint
        if 'minLength' in constraints:
            g.add((shape_uri, SH.minLength, Literal(constraints['minLength'], datatype=XSD.integer)))
        
        # Add maxLength constraint
        if 'maxLength' in constraints:
            g.add((shape_uri, SH.maxLength, Literal(constraints['maxLength'], datatype=XSD.integer)))
    
    return g

def validate_outputs_with_shacl(outputs: Dict[str, str], output_constraints: Dict[str, Dict]) -> Dict[str, Any]:
    """Validate outputs against SHACL constraints using pyshacl"""
    if not output_constraints:
        return {"valid": True, "results": {}}
    
    # Create SHACL shapes graph
    shapes_graph = create_shacl_graph_from_constraints(output_constraints)
    
    # Create data graph with outputs
    data_graph = Graph()
    DSPY = Namespace("http://autotel.ai/dspy#")
    data_graph.bind("dspy", DSPY)
    
    # Add outputs as data
    for output_name, value in outputs.items():
        if value:  # Only validate non-empty values
            property_uri = DSPY[output_name]
            data_graph.add((DSPY.Result, property_uri, Literal(value)))
    
    # Validate using pyshacl
    try:
        conforms, results_graph, results_text = pyshacl.validate(
            data_graph, 
            shacl_graph=shapes_graph,
            ont_graph=None,
            inference='none',
            abort_on_error=False,
            allow_warnings=True,
            meta_shacl=False,
            advanced=False,
            js=False,
            debug=False
        )
        
        # Parse validation results
        validation_results = {}
        for s, p, o in results_graph:
            if p == SH.result:
                # Extract validation details
                validation_results[str(s)] = {
                    "conforms": conforms,
                    "result_text": results_text
                }
        
        return {
            "valid": conforms,
            "results": validation_results,
            "results_text": results_text
        }
        
    except Exception as e:
        return {
            "valid": False,
            "error": str(e),
            "results": {}
        }

def parse_model_configuration(root) -> Dict[str, str]:
    """Parse model configuration from XML"""
    dspy_ns = 'http://autotel.ai/dspy'
    model_elem = root.find(f'.//{{{dspy_ns}}}model')
    
    if model_elem is not None:
        return {
            'name': model_elem.get('name', ''),
            'provider': model_elem.get('provider', ''),
        }
    return {}

def parse_owl_ontology_from_xml(xml_content: str) -> Graph:
    """Parse OWL ontology from XML using the proper OWL processor"""
    owl_processor = OWLProcessor()
    return owl_processor.parse(xml_content)

def extract_owl_class_annotations(root) -> Dict[str, str]:
    """Extract OWL class annotations from signature outputs only (for validation)"""
    owl_ns = 'http://www.w3.org/2002/07/owl#'
    dspy_ns = 'http://autotel.ai/dspy'
    
    annotations = {}
    
    # Find signature
    signature_elem = root.find(f'.//{{{dspy_ns}}}signature')
    if signature_elem is None:
        return annotations
    
    # Extract output OWL classes only (inputs don't need validation)
    for output_elem in signature_elem.findall(f'.//{{{dspy_ns}}}output'):
        output_name = output_elem.get('name')
        owl_class = output_elem.get(f'{{{owl_ns}}}class')
        if owl_class:
            annotations[output_name] = owl_class
    
    return annotations

def create_owl_data_graph(outputs: Dict[str, str], owl_classes: Dict[str, str]) -> Graph:
    """Create OWL data graph from outputs and their class annotations"""
    g = Graph()
    DSPY = Namespace("http://autotel.ai/dspy#")
    g.bind("dspy", DSPY)
    g.bind("owl", OWL)
    g.bind("rdfs", RDFS)
    
    # Create instances for each output
    for output_name, value in outputs.items():
        if value and output_name in owl_classes:
            # Create instance URI
            instance_uri = DSPY[f"{output_name}Instance"]
            class_uri = URIRef(owl_classes[output_name])
            
            # Add instance to class
            g.add((instance_uri, RDF.type, class_uri))
            
            # Add content property if it's a Recommendation
            if "Recommendation" in owl_classes[output_name]:
                g.add((instance_uri, DSPY.hasContent, Literal(value)))
            elif "Reasoning" in owl_classes[output_name]:
                g.add((instance_uri, DSPY.hasExplanation, Literal(value)))
    
    return g

def extract_class_info_from_ontology(ontology_graph: Graph, class_uri: str) -> Dict[str, Any]:
    """Extract class information from OWL ontology"""
    class_info = {
        "uri": class_uri,
        "name": class_uri.split('#')[-1] if '#' in class_uri else class_uri,
        "label": "",
        "comment": "",
        "properties": [],
        "semantic_type": "general"
    }
    
    uri_ref = URIRef(class_uri)
    
    # Extract label
    for label in ontology_graph.objects(uri_ref, RDFS.label):
        class_info["label"] = str(label)
    
    # Extract comment
    for comment in ontology_graph.objects(uri_ref, RDFS.comment):
        class_info["comment"] = str(comment)
    
    # Extract properties (both object and datatype properties)
    for prop_uri in ontology_graph.subjects(RDFS.domain, uri_ref):
        prop_name = str(prop_uri).split('#')[-1] if '#' in str(prop_uri) else str(prop_uri)
        class_info["properties"].append(prop_name)
    
    # Determine semantic type based on class name and properties
    class_name = class_info["name"].lower()
    if "recommendation" in class_name or "suggestion" in class_name:
        class_info["semantic_type"] = "recommendation"
    elif "reasoning" in class_name or "explanation" in class_name or "why" in class_name:
        class_info["semantic_type"] = "reasoning"
    elif "input" in class_name or "user" in class_name:
        class_info["semantic_type"] = "user_input"
    elif "decision" in class_name or "choice" in class_name:
        class_info["semantic_type"] = "decision"
    elif "analysis" in class_name or "evaluation" in class_name:
        class_info["semantic_type"] = "analysis"
    else:
        class_info["semantic_type"] = "general"
    
    return class_info

def validate_with_owl_simple(ontology_graph: Graph, owl_classes: Dict[str, str], outputs: Dict[str, str]) -> Dict[str, Any]:
    """80/20 OWL validation - focus on the most valuable 20% of OWL features"""
    if not owl_classes:
        return {"valid": True, "results": {}, "semantic_guidance": ""}
    
    validation_results = {}
    semantic_guidance_parts = []
    
    # Dynamic class-based validation and semantic guidance
    for output_name, owl_class in owl_classes.items():
        value = outputs.get(output_name, "")
        
        # Extract class information from ontology
        class_info = extract_class_info_from_ontology(ontology_graph, owl_class)
        
        validation_results[output_name] = {
            "class": owl_class,
            "class_name": class_info["name"],
            "class_label": class_info["label"],
            "class_comment": class_info["comment"],
            "semantic_type": class_info["semantic_type"],
            "has_value": bool(value),
            "value_length": len(value) if value else 0,
            "properties": class_info["properties"]
        }
        
        # Generate semantic guidance based on extracted class information
        guidance_parts = []
        
        if class_info["label"]:
            guidance_parts.append(f"{output_name} should be a {class_info['label'].lower()}")
        else:
            guidance_parts.append(f"{output_name} is of type {class_info['name']}")
        
        if class_info["comment"]:
            guidance_parts.append(f"({class_info['comment']})")
        
        if class_info["properties"]:
            guidance_parts.append(f"with properties: {', '.join(class_info['properties'])}")
        
        # Add semantic type specific guidance
        if class_info["semantic_type"] == "recommendation":
            if not value:
                validation_results[output_name]["error"] = f"{class_info['name']} must have content"
            guidance_parts.append("should provide a structured recommendation or suggestion")
            
        elif class_info["semantic_type"] == "reasoning":
            if not value:
                validation_results[output_name]["error"] = f"{class_info['name']} must have explanation"
            guidance_parts.append("should provide logical reasoning or explanation")
            
        elif class_info["semantic_type"] == "user_input":
            guidance_parts.append("represents user-provided input")
            
        elif class_info["semantic_type"] == "decision":
            if not value:
                validation_results[output_name]["error"] = f"{class_info['name']} must have a decision"
            guidance_parts.append("should provide a clear decision or choice")
            
        elif class_info["semantic_type"] == "analysis":
            if not value:
                validation_results[output_name]["error"] = f"{class_info['name']} must have analysis"
            guidance_parts.append("should provide detailed analysis or evaluation")
        
        semantic_guidance_parts.append(" ".join(guidance_parts))
    
    # Overall validation - simple and pragmatic
    all_valid = all(
        result.get("has_value", False) and 
        not result.get("error", False)
        for result in validation_results.values()
    )
    
    semantic_guidance = ". ".join(semantic_guidance_parts)
    
    return {
        "valid": all_valid,
        "results": validation_results,
        "semantic_guidance": semantic_guidance
    }

# Parse XML and extract signature information
root = ET.fromstring(xml_data)
dspy_ns = 'http://autotel.ai/dspy'
shacl_ns = 'http://www.w3.org/ns/shacl#'
owl_ns = 'http://www.w3.org/2002/07/owl#'

# Parse OWL ontology using proper OWL processor
print("Parsing OWL ontology...")
ontology_graph = parse_owl_ontology_from_xml(xml_data)
print(f"Parsed {len(ontology_graph)} ontology triples")

# Extract OWL class annotations
owl_classes = extract_owl_class_annotations(root)
print(f"OWL class annotations: {owl_classes}")

# Parse model configuration
model_config = parse_model_configuration(root)
print(f"Model configuration: {model_config}")

# Configure DSPy with model from XML
if model_config:
    model_name = model_config['name']
    provider = model_config['provider']
    
    if provider == 'ollama':
        dspy.configure(lm=dspy.LM(f"ollama/{model_name}"))
        print(f"Configured DSPy with Ollama model: {model_name}")
    elif provider == 'openai':
        dspy.configure(lm=dspy.LM(model_name, api_key="your-api-key"))
        print(f"Configured DSPy with OpenAI model: {model_name}")
    else:
        dspy.configure(lm=dspy.LM(model_name))
        print(f"Configured DSPy with model: {model_name}")
else:
    # Fallback to default
    dspy.configure(lm=dspy.LM("ollama/qwen3:latest"))
    print("No model configuration found, using default: ollama/qwen3:latest")

print()

# Find the signature
signature_elem = root.find(f'.//{{{dspy_ns}}}signature')
signature_name = signature_elem.get('name')
base_description = signature_elem.get('description', '')

# Extract inputs and outputs with SHACL constraints
inputs = []
outputs = []
input_constraints = {}
output_constraints = {}

for input_elem in signature_elem.findall(f'.//{{{dspy_ns}}}input'):
    input_name = input_elem.get('name')
    inputs.append(input_name)
    input_constraints[input_name] = extract_shacl_constraints(input_elem)

for output_elem in signature_elem.findall(f'.//{{{dspy_ns}}}output'):
    output_name = output_elem.get('name')
    outputs.append(output_name)
    output_constraints[output_name] = extract_shacl_constraints(output_elem)

# Create enhanced description with SHACL constraints
enhanced_description = create_enhanced_description(
    base_description, inputs, outputs, input_constraints, output_constraints
)

print(f"Parsed from XML:")
print(f"  Signature: {signature_name}")
print(f"  Base Description: {base_description}")
print(f"  Enhanced Description:\n{enhanced_description}")
print(f"  Inputs: {inputs}")
print(f"  Outputs: {outputs}")
print(f"  Input constraints: {input_constraints}")
print(f"  Output constraints: {output_constraints}")
print()

# Dynamically create signature class from XML data with SHACL typing
annotations = {}
fields = {}

for input_name in inputs:
    constraints = input_constraints[input_name]
    python_type = shacl_to_python_type(constraints)
    annotations[input_name] = python_type
    fields[input_name] = dspy.InputField()
    print(f"  Input '{input_name}': {python_type.__name__} (from SHACL: {constraints.get('datatype', 'unknown')})")

for output_name in outputs:
    constraints = output_constraints[output_name]
    python_type = shacl_to_python_type(constraints)
    annotations[output_name] = python_type
    fields[output_name] = dspy.OutputField()
    print(f"  Output '{output_name}': {python_type.__name__} (from SHACL: {constraints.get('datatype', 'unknown')})")

print()

DynamicSignature = type(
    signature_name,
    (dspy.Signature,),
    {
        "__doc__": enhanced_description,  # Use enhanced description with SHACL constraints
        "__annotations__": annotations,
        **fields
    }
)

# Find the module type from XML
module_elem = root.find(f'.//{{{dspy_ns}}}chain-of-thought')
module_type = "chain-of-thought" if module_elem is not None else "predict"

# Dynamically create the module
if module_type == "chain-of-thought":
    module = dspy.ChainOfThought(DynamicSignature)
elif module_type == "predict":
    module = dspy.Predict(DynamicSignature)
else:
    module = dspy.Predict(DynamicSignature)

print(f"Created {module_type} module with dynamic signature")
print(f"Signature class: {DynamicSignature}")
print(f"Signature annotations: {DynamicSignature.__annotations__}")
print()

# Call the module with input
result = module(user_input="I want a recommendation for a healthy lunch.")

# Print the result and its fields
print("Result:", result)
output_values = {}
for output_name in outputs:
    value = getattr(result, output_name, None)
    output_values[output_name] = value
    print(f"{output_name}: {value}")

# Validate outputs against SHACL constraints using pyshacl
print("\n" + "="*50)
print("SHACL Validation Results:")
validation_result = validate_outputs_with_shacl(output_values, output_constraints)

if validation_result["valid"]:
    print("✅ All outputs conform to SHACL constraints")
else:
    print("❌ Some outputs violate SHACL constraints")
    if "results_text" in validation_result:
        print("Validation details:")
        print(validation_result["results_text"])

# Validate outputs against OWL ontology (80/20 approach)
print("\n" + "="*50)
print("OWL Ontology Validation Results (80/20):")

owl_validation_result = validate_with_owl_simple(ontology_graph, owl_classes, output_values)

if owl_validation_result["valid"]:
    print("✅ All outputs conform to OWL ontology")
else:
    print("❌ Some outputs violate OWL ontology constraints")

# Print detailed OWL validation results
for output_name, result in owl_validation_result["results"].items():
    print(f"\n{output_name}:")
    print(f"  Class: {result.get('class', 'N/A')}")
    print(f"  Class name: {result.get('class_name', 'N/A')}")
    if result.get('class_label'):
        print(f"  Class label: {result.get('class_label')}")
    if result.get('class_comment'):
        print(f"  Class comment: {result.get('class_comment')}")
    print(f"  Semantic type: {result.get('semantic_type', 'N/A')}")
    if result.get('properties'):
        print(f"  Properties: {', '.join(result.get('properties', []))}")
    print(f"  Has value: {'✅' if result.get('has_value') else '❌'}")
    print(f"  Value length: {result.get('value_length', 0)}")
    if 'error' in result:
        print(f"  Error: {result['error']}")

# Print overall semantic guidance
if owl_validation_result.get('semantic_guidance'):
    print(f"\nSemantic Guidance: {owl_validation_result['semantic_guidance']}")

print("="*50) 