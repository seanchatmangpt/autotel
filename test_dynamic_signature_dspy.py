import dspy
import xml.etree.ElementTree as ET
from typing import Dict, Any, Type, Union

dspy.configure(lm=dspy.LM("ollama/qwen3:latest"))

# XML data with SHACL constraints
xml_data = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:dspy="http://autotel.ai/dspy"
                  xmlns:shacl="http://www.w3.org/ns/shacl#"
                  targetNamespace="http://autotel.ai/workflows">

  <dspy:signatures>
    <dspy:signature name="ExplainChoice" description="Explain a simple AI choice">
      <dspy:input name="user_input" description="User's input">
        <shacl:PropertyShape>
          <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
          <shacl:minLength>1</shacl:minLength>
          <shacl:maxLength>1000</shacl:maxLength>
        </shacl:PropertyShape>
      </dspy:input>
      <dspy:output name="recommendation" description="AI recommendation">
        <shacl:PropertyShape>
          <shacl:datatype>http://www.w3.org/2001/XMLSchema#string</shacl:datatype>
          <shacl:minLength>10</shacl:minLength>
        </shacl:PropertyShape>
      </dspy:output>
      <dspy:output name="why" description="Reasoning for the recommendation">
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

# Parse XML and extract signature information
root = ET.fromstring(xml_data)
dspy_ns = 'http://autotel.ai/dspy'
shacl_ns = 'http://www.w3.org/ns/shacl#'

# Find the signature
signature_elem = root.find(f'.//{{{dspy_ns}}}signature')
signature_name = signature_elem.get('name')
signature_description = signature_elem.get('description', '')

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

print(f"Parsed from XML:")
print(f"  Signature: {signature_name}")
print(f"  Description: {signature_description}")
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
        "__doc__": signature_description,
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
for output_name in outputs:
    value = getattr(result, output_name, None)
    print(f"{output_name}: {value}") 