"""
Advanced DSPy Services for BPMN Integration with SHACL Validation
Supports the Four Pillars architecture: BPMN + DMN + DSPy + SHACL
"""

import json
import dspy
from typing import Dict, Any, Optional, Type, List
from dataclasses import dataclass
from pathlib import Path
import logging
from rdflib import Graph, URIRef, Literal, Namespace
from rdflib.namespace import RDF, XSD
import pyshacl

# Configure logging
logger = logging.getLogger(__name__)

# Configure DSPy globally
try:
    ollama_lm = dspy.LM('ollama/qwen3:latest', temperature=0.7)
    dspy.configure(lm=ollama_lm)
except Exception as e:
    logger.warning(f"Could not configure DSPy with Ollama: {e}")
    # Fallback to mock LM for testing
    dspy.configure(lm=dspy.MockLM())

@dataclass
class AdvancedDSPySignature:
    """Enhanced registry entry for a DSPy signature with SHACL integration"""
    signature_class: Type[dspy.Signature]
    description: str
    input_fields: Dict[str, str]  # field_name -> description
    output_fields: Dict[str, str]  # field_name -> description
    shacl_input_shapes: Dict[str, str] = None  # field_name -> shape_uri
    shacl_output_shapes: Dict[str, str] = None  # field_name -> shape_uri
    validation_enabled: bool = True

class AdvancedDSPyServiceRegistry:
    """Advanced registry for DSPy signatures with SHACL validation"""
    
    def __init__(self):
        self._signatures: Dict[str, AdvancedDSPySignature] = {}
        self._predict_modules: Dict[str, dspy.Predict] = {}
        self._parser_signatures: Dict[str, Type[dspy.Signature]] = {}
        self._shacl_graph = Graph()
        self._validation_cache = {}
    
    def register_signature(self, name: str, signature_class: Type[dspy.Signature], 
                          description: str = "", shacl_shapes: Dict[str, Dict[str, str]] = None):
        """Register a DSPy signature with optional SHACL shapes"""
        # Extract field information
        input_fields = {}
        output_fields = {}
        
        # Get all attributes of the class
        for attr_name in dir(signature_class):
            if not attr_name.startswith('_'):
                attr_value = getattr(signature_class, attr_name)
                if hasattr(attr_value, '__class__'):
                    tname = attr_value.__class__.__name__
                    if tname == 'InputField':
                        input_fields[attr_name] = getattr(attr_value, 'desc', attr_name)
                    elif tname == 'OutputField':
                        output_fields[attr_name] = getattr(attr_value, 'desc', attr_name)
        
        # Extract SHACL shapes if provided
        shacl_input_shapes = shacl_shapes.get('inputs', {}) if shacl_shapes else None
        shacl_output_shapes = shacl_shapes.get('outputs', {}) if shacl_shapes else None
        
        # Create registry entry
        signature_info = AdvancedDSPySignature(
            signature_class=signature_class,
            description=description,
            input_fields=input_fields,
            output_fields=output_fields,
            shacl_input_shapes=shacl_input_shapes,
            shacl_output_shapes=shacl_output_shapes
        )
        self._signatures[name] = signature_info
        self._predict_modules[name] = dspy.Predict(signature_class)
        
        logger.info(f"Registered DSPy signature: {name}")
        logger.info(f"  Inputs: {list(input_fields.keys())}")
        logger.info(f"  Outputs: {list(output_fields.keys())}")
        if shacl_input_shapes:
            logger.info(f"  Input SHACL shapes: {shacl_input_shapes}")
        if shacl_output_shapes:
            logger.info(f"  Output SHACL shapes: {shacl_output_shapes}")
    
    def register_parser_signatures(self, parser_signatures: Dict[str, Type[dspy.Signature]], 
                                 shacl_graph: Graph = None):
        """Register dynamic signatures from the BPMN parser with SHACL support"""
        if shacl_graph:
            self._shacl_graph = shacl_graph
        
        for name, signature_class in parser_signatures.items():
            self._parser_signatures[name] = signature_class
            self._predict_modules[name] = dspy.Predict(signature_class)
            logger.info(f"Registered parser signature: {name}")
    
    def add_shacl_shapes(self, shacl_graph: Graph):
        """Add SHACL shapes to the registry"""
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

# Advanced DSPy signatures with SHACL integration
class CustomerRiskAnalysis(dspy.Signature):
    """AI-powered customer risk assessment with SHACL validation"""
    customer_profile = dspy.InputField(desc="Customer demographic and financial data")
    transaction_history = dspy.InputField(desc="Historical transaction patterns")
    risk_score = dspy.OutputField(desc="Numerical risk assessment (0-100)")
    risk_factors = dspy.OutputField(desc="Key risk factors identified")

class DocumentGeneration(dspy.Signature):
    """Generate documents from templates with validation"""
    template_type = dspy.InputField(desc="Type of document to generate")
    data_context = dspy.InputField(desc="Context data for document generation")
    generated_document = dspy.OutputField(desc="Generated document content")

class DataQualityAssessment(dspy.Signature):
    """Assess data quality using AI and SHACL validation"""
    data_sample = dspy.InputField(desc="Sample of data to assess")
    quality_metrics = dspy.OutputField(desc="Quality assessment metrics")
    recommendations = dspy.OutputField(desc="Recommendations for improvement")

class ProcessOptimization(dspy.Signature):
    """Optimize business processes using AI analysis"""
    process_data = dspy.InputField(desc="Process execution data")
    optimization_suggestions = dspy.OutputField(desc="Process optimization suggestions")
    expected_improvement = dspy.OutputField(desc="Expected improvement percentage")

# Register advanced signatures with SHACL shapes
advanced_dspy_registry.register_signature(
    "customer_risk_analysis",
    CustomerRiskAnalysis,
    "AI-powered customer risk assessment with SHACL validation",
    {
        'inputs': {
            'customer_profile': 'http://autotel.ai/shapes#CustomerShape',
            'transaction_history': 'http://autotel.ai/shapes#TransactionHistoryShape'
        },
        'outputs': {
            'risk_score': 'http://autotel.ai/shapes#RiskScoreShape',
            'risk_factors': 'http://autotel.ai/shapes#RiskFactorsShape'
        }
    }
)

advanced_dspy_registry.register_signature(
    "document_generation",
    DocumentGeneration,
    "Generate documents from templates with validation",
    {
        'inputs': {
            'template_type': 'http://autotel.ai/shapes#TemplateTypeShape',
            'data_context': 'http://autotel.ai/shapes#DocumentContextShape'
        },
        'outputs': {
            'generated_document': 'http://autotel.ai/shapes#DocumentShape'
        }
    }
)

advanced_dspy_registry.register_signature(
    "data_quality_assessment",
    DataQualityAssessment,
    "Assess data quality using AI and SHACL validation",
    {
        'inputs': {
            'data_sample': 'http://autotel.ai/shapes#DataSampleShape'
        },
        'outputs': {
            'quality_metrics': 'http://autotel.ai/shapes#QualityMetricsShape',
            'recommendations': 'http://autotel.ai/shapes#RecommendationsShape'
        }
    }
)

advanced_dspy_registry.register_signature(
    "process_optimization",
    ProcessOptimization,
    "Optimize business processes using AI analysis",
    {
        'inputs': {
            'process_data': 'http://autotel.ai/shapes#ProcessDataShape'
        },
        'outputs': {
            'optimization_suggestions': 'http://autotel.ai/shapes#OptimizationSuggestionsShape',
            'expected_improvement': 'http://autotel.ai/shapes#ImprovementShape'
        }
    }
)

# Advanced service functions
def advanced_dspy_service(operation_name: str, **operation_params) -> Dict[str, Any]:
    """
    Advanced function to call any DSPy signature with SHACL validation.
    This is the main entry point for Service Tasks with validation.
    
    Args:
        operation_name: Name of the registered DSPy signature
        **operation_params: Parameters to pass to the signature
    
    Returns:
        Dictionary containing the results
    """
    try:
        # Call the DSPy signature with validation
        result = advanced_dspy_registry.call_signature(operation_name, **operation_params)
        return result
        
    except Exception as e:
        logger.error(f"Error in advanced DSPy service '{operation_name}': {str(e)}")
        # Return error information
        return {
            "error": True,
            "message": str(e),
            "operation": operation_name
        }

def call_customer_risk_analysis(customer_profile: Dict[str, Any], 
                              transaction_history: Dict[str, Any]) -> Dict[str, Any]:
    """Convenience function for customer risk analysis"""
    return advanced_dspy_service("customer_risk_analysis", 
                               customer_profile=customer_profile,
                               transaction_history=transaction_history)

def call_document_generation(template_type: str, data_context: Dict[str, Any]) -> Dict[str, Any]:
    """Convenience function for document generation"""
    return advanced_dspy_service("document_generation", 
                               template_type=template_type,
                               data_context=data_context)

def call_data_quality_assessment(data_sample: Dict[str, Any]) -> Dict[str, Any]:
    """Convenience function for data quality assessment"""
    return advanced_dspy_service("data_quality_assessment", 
                               data_sample=data_sample)

def call_process_optimization(process_data: Dict[str, Any]) -> Dict[str, Any]:
    """Convenience function for process optimization"""
    return advanced_dspy_service("process_optimization", 
                               process_data=process_data)

# Export for workflow integration
advanced_dspy_service = advanced_dspy_service 