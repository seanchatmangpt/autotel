"""
Advanced DSPy Services for BPMN Integration with SHACL Validation
Allows calling any DSPy signature from BPMN XML using Service Tasks
Supports dynamic signature creation from XML definitions with SHACL validation
"""

import json
import logging
import dspy
from typing import Dict, Any, Optional, Type
from dataclasses import dataclass
from rdflib import Graph, URIRef, Literal, Namespace
from rdflib.namespace import RDF, XSD
import pyshacl

# Configure logging
logger = logging.getLogger(__name__)

# Configure DSPy globally
ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.7)
dspy.configure(lm=ollama_lm)

@dataclass
class AdvancedDSPySignature:
    """Enhanced registry entry for a DSPy signature with SHACL integration"""
    signature_class: Type[dspy.Signature]
    description: str
    input_fields: Dict[str, str]  # field_name -> description
    output_fields: Dict[str, str]  # field_name -> description
    shacl_input_shapes: Dict[str, str] = None  # field_name -> shape_uri
    shacl_output_shapes: Dict[str, str] = None  # output_name -> shape_uri
    validation_enabled: bool = True

class AdvancedDSPyServiceRegistry:
    """Advanced registry for DSPy signatures with SHACL validation"""
    
    def __init__(self):
        self._signatures: Dict[str, AdvancedDSPySignature] = {}
        self._predict_modules: Dict[str, dspy.Predict] = {}
        self._parser_signatures: Dict[str, Type[dspy.Signature]] = {}
        self._shacl_graph = Graph()
    
    def create_dynamic_signature(self, name: str, input_fields: Dict[str, str], 
                                output_fields: Dict[str, str], description: str = "") -> Type[dspy.Signature]:
        """Create a DSPy signature dynamically from field definitions"""
        
        # Create input field attributes
        input_attrs = {}
        for field_name, field_desc in input_fields.items():
            input_attrs[field_name] = dspy.InputField(desc=field_desc)
        
        # Create output field attributes
        output_attrs = {}
        for field_name, field_desc in output_fields.items():
            output_attrs[field_name] = dspy.OutputField(desc=field_desc)
        
        # Create the signature class dynamically
        signature_class = type(
            name,
            (dspy.Signature,),
            {
                "__doc__": description,
                **input_attrs,
                **output_attrs
            }
        )
        
        return signature_class
    
    def register_dynamic_signature(self, name: str, input_fields: Dict[str, str], 
                                  output_fields: Dict[str, str], description: str = "",
                                  shacl_input_shapes: Dict[str, str] = None,
                                  shacl_output_shapes: Dict[str, str] = None):
        """Register a dynamically created DSPy signature with SHACL validation"""
        signature_class = self.create_dynamic_signature(name, input_fields, output_fields, description)
        
        # Create registry entry
        signature_info = AdvancedDSPySignature(
            signature_class=signature_class,
            description=description,
            input_fields=input_fields,
            output_fields=output_fields,
            shacl_input_shapes=shacl_input_shapes,
            shacl_output_shapes=shacl_output_shapes,
            validation_enabled=True
        )
        self._signatures[name] = signature_info
        self._predict_modules[name] = dspy.Predict(signature_class)
    
    def register_parser_signatures(self, parser_signatures: Dict[str, Type[dspy.Signature]], 
                                 shacl_graph: Graph = None):
        """Register dynamic signatures from the BPMN parser"""
        for name, signature_class in parser_signatures.items():
            self._parser_signatures[name] = signature_class
            self._predict_modules[name] = dspy.Predict(signature_class)
        
        if shacl_graph:
            self._shacl_graph = shacl_graph
    
    def add_shacl_shapes(self, shacl_graph: Graph):
        """Add SHACL shapes for validation"""
        self._shacl_graph = shacl_graph
    
    def call_signature(self, signature_name: str, **kwargs) -> Dict[str, Any]:
        """Call a registered DSPy signature with SHACL validation"""
        # Validate inputs if SHACL shapes are available
        if signature_name in self._signatures:
            sig_info = self._signatures[signature_name]
            if sig_info.validation_enabled and sig_info.shacl_input_shapes:
                self._validate_inputs_with_shacl(signature_name, kwargs, sig_info)
        
        try:
            # Call the DSPy signature
            if signature_name in self._parser_signatures:
                result = self._predict_modules[signature_name](**kwargs)
            else:
                result = self._predict_modules[signature_name](**kwargs)
            
            # Extract results
            output_dict = self._extract_signature_output(result)
            
            # Validate outputs if SHACL shapes are available
            if signature_name in self._signatures:
                sig_info = self._signatures[signature_name]
                if sig_info.validation_enabled and sig_info.shacl_output_shapes:
                    self._validate_outputs_with_shacl(signature_name, output_dict, sig_info)
            
            return output_dict
                
        except Exception as e:
            logger.error(f"Error calling DSPy signature '{signature_name}': {str(e)}")
            raise
    
    def _validate_inputs_with_shacl(self, signature_name: str, inputs: Dict[str, Any], 
                                   sig_info: AdvancedDSPySignature):
        """Validate input parameters using SHACL shapes"""
        if not sig_info.shacl_input_shapes or len(self._shacl_graph) == 0:
            return
        
        for field_name, shape_uri in sig_info.shacl_input_shapes.items():
            if field_name in inputs:
                self._validate_data_with_shacl(
                    {field_name: inputs[field_name]}, 
                    shape_uri, 
                    f"DSPy Input [{signature_name}.{field_name}]"
                )
    
    def _validate_outputs_with_shacl(self, signature_name: str, outputs: Dict[str, Any], 
                                    sig_info: AdvancedDSPySignature):
        """Validate output parameters using SHACL shapes"""
        if not sig_info.shacl_output_shapes or len(self._shacl_graph) == 0:
            return
        
        for field_name, shape_uri in sig_info.shacl_output_shapes.items():
            if field_name in outputs:
                self._validate_data_with_shacl(
                    {field_name: outputs[field_name]}, 
                    shape_uri, 
                    f"DSPy Output [{signature_name}.{field_name}]"
                )
    
    def _validate_data_with_shacl(self, data_dict: dict, shape_uri: str, context: str):
        """Validate data using SHACL shapes"""
        logger.info(f"Validating {context} against shape: <{shape_uri}>")
        
        # Convert Python dict to RDF graph
        data_graph = Graph()
        autotel_ns = Namespace("http://autotel.ai/data/")
        instance_uri = autotel_ns.instance
        
        # Add type information
        data_graph.add((instance_uri, RDF.type, URIRef(shape_uri)))
        
        # Convert data to RDF triples
        for key, value in data_dict.items():
            predicate = autotel_ns[key]
            if isinstance(value, str):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.string)))
            elif isinstance(value, int):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.integer)))
            elif isinstance(value, float):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.decimal)))
            elif isinstance(value, bool):
                data_graph.add((instance_uri, predicate, Literal(value, datatype=XSD.boolean)))
            else:
                # Serialize complex objects as JSON strings
                json_value = json.dumps(value) if not isinstance(value, str) else str(value)
                data_graph.add((instance_uri, predicate, Literal(json_value, datatype=XSD.string)))
        
        # Perform SHACL validation
        conforms, results_graph, results_text = pyshacl.validate(
            data_graph,
            shacl_graph=self._shacl_graph,
            inference='rdfs'
        )
        
        if not conforms:
            raise ValueError(f"{context} data failed SHACL validation for shape <{shape_uri}>:\n{results_text}")
        
        logger.info(f"SUCCESS: {context} data conforms to SHACL contract")
    
    def _extract_signature_output(self, result) -> Dict[str, Any]:
        """Extract output from DSPy signature result"""
        if hasattr(result, '_store') and isinstance(result._store, dict) and result._store:
            return dict(result._store)
        else:
            # Fallback: return all public fields
            return {k: v for k, v in vars(result).items() if not k.startswith('_')}
    
    def get_signature_info(self, signature_name: str) -> Optional[AdvancedDSPySignature]:
        """Get information about a registered signature"""
        return self._signatures.get(signature_name)
    
    def list_signatures(self) -> Dict[str, AdvancedDSPySignature]:
        """List all registered signatures"""
        return self._signatures.copy()
    
    def list_parser_signatures(self) -> Dict[str, Type[dspy.Signature]]:
        """List all parser signatures"""
        return self._parser_signatures.copy()
    
    def enable_validation(self, signature_name: str, enabled: bool = True):
        """Enable or disable SHACL validation for a signature"""
        if signature_name in self._signatures:
            self._signatures[signature_name].validation_enabled = enabled
            logger.info(f"SHACL validation {'enabled' if enabled else 'disabled'} for signature: {signature_name}")

# Global advanced registry instance
advanced_dspy_registry = AdvancedDSPyServiceRegistry()

# Advanced service function for BPMN integration
def advanced_dspy_service(operation_name: str, **operation_params) -> Dict[str, Any]:
    """
    Advanced function to call any DSPy signature with SHACL validation.
    This is the main entry point for Service Tasks with advanced features.
    
    Args:
        operation_name: Name of the registered DSPy signature
        **operation_params: Parameters to pass to the signature
    
    Returns:
        Dictionary containing the results
    """
    try:
        # Call the DSPy signature
        result = advanced_dspy_registry.call_signature(operation_name, **operation_params)
        return result
        
    except Exception as e:
        # Return error information
        error_result = {
            "error": True,
            "message": str(e),
            "operation": operation_name
        }
        return error_result

def initialize_advanced_dspy_services():
    """Initialize advanced DSPy services (no hardcoded signatures)"""
    logger.info("Advanced DSPy services initialized - all signatures will be loaded from XML") 