"""Pipeline orchestrator for AutoTel semantic execution pipeline."""

from typing import Dict, Any, Optional
from datetime import datetime

from autotel.processors.owl_processor import OWLProcessor
from autotel.processors.shacl_processor import SHACLProcessor
from autotel.processors.dspy_processor import DSPyProcessor
from autotel.processors.otel_processor import OTELProcessor
from autotel.processors.base import ProcessorConfig
from .ontology_compiler import OntologyCompiler
from .validation_compiler import ValidationCompiler
from .dspy_compiler import DSPyCompiler
from .linker import SemanticLinker
from .executor import OntologyExecutor, ExecutionResult
from autotel.core.telemetry import TelemetryManager, create_telemetry_manager


class PipelineOrchestrator:
    """Orchestrates the complete AutoTel semantic execution pipeline."""

    def __init__(self, telemetry_manager: TelemetryManager = None):
        """Initialize the pipeline orchestrator."""
        # Initialize processors with unified configuration
        self.owl_processor = OWLProcessor(
            ProcessorConfig(name="owl_processor"), 
            telemetry=telemetry_manager
        )
        self.shacl_processor = SHACLProcessor(ProcessorConfig(name="shacl_processor"))
        self.dspy_processor = DSPyProcessor(ProcessorConfig(name="dspy_processor"))
        self.otel_processor = OTELProcessor(ProcessorConfig(name="otel_processor"))
        self.ontology_compiler = OntologyCompiler()
        self.validation_compiler = ValidationCompiler()
        self.dspy_compiler = DSPyCompiler()
        self.linker = SemanticLinker()
        self.executor = OntologyExecutor()
        self.telemetry = telemetry_manager or create_telemetry_manager()

    def process_owl_content(self, owl_xml: str) -> Dict[str, Any]:
        """Process OWL XML content individually."""
        with self.telemetry.start_span(
            name="pipeline.process_owl",
            operation_type="OWL_PROCESSING",
            stage="owl_processing"
        ) as span:
            try:
                if not owl_xml or owl_xml.strip() == "":
                    span.set_attribute("empty_content", True)
                    return {"error": "Empty OWL content provided"}
                
                # Use unified processor
                result = self.owl_processor.process(owl_xml)
                if not result.success:
                    span.set_attribute("success", False)
                    span.set_attribute("error", result.error)
                    return {"error": result.error}
                
                ontology_def = result.data
                span.set_attribute("classes_count", len(ontology_def.classes))
                span.set_attribute("properties_count", len(ontology_def.object_properties) + len(ontology_def.data_properties))
                span.set_attribute("success", True)
                
                return {
                    "ontology_definition": ontology_def,
                    "metadata": {
                        "classes_count": len(ontology_def.classes),
                        "object_properties_count": len(ontology_def.object_properties),
                        "data_properties_count": len(ontology_def.data_properties),
                        "individuals_count": len(ontology_def.individuals),
                        "axioms_count": len(ontology_def.axioms)
                    }
                }
            except Exception as e:
                span.set_attribute("success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(self.telemetry.trace.Status(self.telemetry.trace.StatusCode.ERROR, str(e)))
                raise

    def process_shacl_content(self, shacl_xml: str) -> Dict[str, Any]:
        """Process SHACL XML content individually."""
        with self.telemetry.start_span(
            name="pipeline.process_shacl",
            operation_type="SHACL_PROCESSING",
            stage="shacl_processing"
        ) as span:
            try:
                if not shacl_xml or shacl_xml.strip() == "":
                    span.set_attribute("empty_content", True)
                    return {"error": "Empty SHACL content provided"}
                
                # Use unified processor
                result = self.shacl_processor.process(shacl_xml)
                if not result.success:
                    span.set_attribute("success", False)
                    span.set_attribute("error", result.error)
                    return {"error": result.error}
                
                shacl_result = result.data
                span.set_attribute("node_shapes_count", len(shacl_result.node_shapes))
                span.set_attribute("property_shapes_count", len(shacl_result.property_shapes))
                span.set_attribute("constraints_count", len(shacl_result.constraints))
                span.set_attribute("success", True)
                
                return {
                    "node_shapes": shacl_result.node_shapes,
                    "property_shapes": shacl_result.property_shapes,
                    "constraints": shacl_result.constraints,
                    "total_shapes": shacl_result.total_shapes,
                    "total_constraints": shacl_result.total_constraints
                }
            except Exception as e:
                span.set_attribute("success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(self.telemetry.trace.Status(self.telemetry.trace.StatusCode.ERROR, str(e)))
                raise

    def process_dspy_content(self, dspy_xml: str) -> Dict[str, Any]:
        """Process DSPy XML content individually."""
        with self.telemetry.start_span(
            name="pipeline.process_dspy",
            operation_type="DSPY_PROCESSING",
            stage="dspy_processing"
        ) as span:
            try:
                if not dspy_xml or dspy_xml.strip() == "":
                    span.set_attribute("empty_content", True)
                    return {"error": "Empty DSPy content provided"}
                
                # Use unified processor
                result = self.dspy_processor.process(dspy_xml)
                if not result.success:
                    span.set_attribute("success", False)
                    span.set_attribute("error", result.error)
                    return {"error": result.error}
                
                dspy_result = result.data
                signatures = dspy_result.signatures
                
                span.set_attribute("signatures_count", len(signatures))
                span.set_attribute("success", True)
                
                return {
                    "signatures": signatures,
                    "metadata": {
                        "signatures_count": len(signatures),
                        "total_signatures": dspy_result.total_signatures
                    }
                }
            except Exception as e:
                span.set_attribute("success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(self.telemetry.trace.Status(self.telemetry.trace.StatusCode.ERROR, str(e)))
                raise

    def execute_pipeline(
        self,
        owl_xml: str,
        shacl_xml: str,
        dspy_xml: str,
        inputs: Dict[str, Any]
    ) -> ExecutionResult:
        """Execute the complete pipeline from XML inputs to execution results."""
        # Stage 1: Processors
        with self.telemetry.start_span(
            name="pipeline.processors",
            operation_type="PROCESSORS",
            stage="processors",
            input_types=str(type(owl_xml))+","+str(type(shacl_xml))+","+str(type(dspy_xml))
        ):
            # Process each input individually with error handling
            ontology_def = None
            shacl_graph = None
            dspy_signatures = []
            
            # Process OWL
            if owl_xml and owl_xml.strip():
                try:
                    result = self.owl_processor.process(owl_xml)
                    if result.success:
                        ontology_def = result.data
                    else:
                        self.telemetry.record_metric("pipeline_owl_processing_failure", 1)
                        raise ValueError(f"OWL processing failed: {result.error}")
                except Exception as e:
                    self.telemetry.record_metric("pipeline_owl_processing_failure", 1)
                    raise
            
            # Process SHACL
            if shacl_xml and shacl_xml.strip():
                try:
                    result = self.shacl_processor.process(shacl_xml)
                    if result.success:
                        shacl_graph = result.data
                    else:
                        self.telemetry.record_metric("pipeline_shacl_processing_failure", 1)
                        raise ValueError(f"SHACL processing failed: {result.error}")
                except Exception as e:
                    self.telemetry.record_metric("pipeline_shacl_processing_failure", 1)
                    raise
            
            # Process DSPy
            if dspy_xml and dspy_xml.strip():
                try:
                    result = self.dspy_processor.process(dspy_xml)
                    if result.success:
                        dspy_signatures = result.data.signatures
                    else:
                        self.telemetry.record_metric("pipeline_dspy_processing_failure", 1)
                        raise ValueError(f"DSPy processing failed: {result.error}")
                except Exception as e:
                    self.telemetry.record_metric("pipeline_dspy_processing_failure", 1)
                    raise
        
        # Stage 2: Compilers
        with self.telemetry.start_span(
            name="pipeline.compilers",
            operation_type="COMPILERS",
            stage="compilers"
        ):
            ontology_schema = None
            validation_rules = []
            dspy_signature = None
            
            if ontology_def:
                ontology_schema = self.ontology_compiler.compile(ontology_def)
            
            if shacl_graph:
                validation_rules = self.validation_compiler.compile(shacl_graph)
            
            if ontology_schema and validation_rules and dspy_signatures:
                dspy_signature = self.dspy_compiler.compile(
                    ontology_schema, validation_rules, dspy_signatures
                )
        
        # Stage 3: Linking
        with self.telemetry.start_span(
            name="pipeline.linking",
            operation_type="LINKING",
            stage="linking"
        ):
            linked_result = self.linker.link(
                ontology_schema, validation_rules, dspy_signature
            )
        
        # Stage 4: Execution
        with self.telemetry.start_span(
            name="pipeline.execution",
            operation_type="EXECUTION",
            stage="execution"
        ):
            execution_result = self.executor.execute(linked_result, inputs)
        
        return execution_result

    def execute_from_files(
        self,
        owl_file: str,
        shacl_file: str,
        dspy_file: str,
        inputs: Dict[str, Any]
    ) -> ExecutionResult:
        """Execute pipeline from file paths."""
        # Read XML files
        owl_xml = ""
        shacl_xml = ""
        dspy_xml = ""
        
        if owl_file:
            with open(owl_file, 'r', encoding='utf-8') as f:
                owl_xml = f.read()
        
        if shacl_file:
            with open(shacl_file, 'r', encoding='utf-8') as f:
                shacl_xml = f.read()
        
        if dspy_file:
            with open(dspy_file, 'r', encoding='utf-8') as f:
                dspy_xml = f.read()
        
        return self.execute_pipeline(owl_xml, shacl_xml, dspy_xml, inputs) 

    def process_otel_data(
        self,
        otel_data: Dict[str, Any],
        analysis_config: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """Process OpenTelemetry data for analysis and insights."""
        with self.telemetry.start_span(
            name="pipeline.otel_processing",
            operation_type="OTEL_PROCESSING",
            stage="otel_processing"
        ):
            # Parse OTEL trace
            otel_trace = self.otel_processor.parse_trace(otel_data)
            
            # Analyze trace
            analysis_result = self.otel_processor.analyze_trace(otel_trace)
            
            # Convert to AutoTel telemetry format
            autotel_telemetry = self.otel_processor.convert_to_autotel_telemetry(otel_trace)
            
            return {
                "trace_id": otel_trace.trace_id,
                "analysis": analysis_result,
                "autotel_telemetry": autotel_telemetry,
                "metadata": {
                    "spans_count": len(otel_trace.spans),
                    "metrics_count": len(otel_trace.metrics),
                    "logs_count": len(otel_trace.logs),
                    "processing_timestamp": datetime.now().isoformat()
                }
            }

    def process_otel_file(
        self,
        otel_file: str,
        analysis_config: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """Process OpenTelemetry data from file."""
        return self.otel_processor.parse_file(otel_file) 