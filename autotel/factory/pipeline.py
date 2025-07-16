"""Pipeline orchestrator for AutoTel semantic execution pipeline."""

from typing import Dict, Any, Optional
from datetime import datetime

from .processors.owl_processor import OWLProcessor
from .processors.shacl_processor import SHACLProcessor
from .processors.dspy_processor import DSPyProcessor
from .processors.otel_processor import OTELProcessor
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
        self.owl_processor = OWLProcessor()
        self.shacl_processor = SHACLProcessor()
        self.dspy_processor = DSPyProcessor()
        self.otel_processor = OTELProcessor()
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
                
                ontology_def = self.owl_processor.parse_ontology_definition(owl_xml)
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
                
                shacl_result = self.shacl_processor.parse(shacl_xml)
                span.set_attribute("node_shapes_count", len(shacl_result['node_shapes']))
                span.set_attribute("property_shapes_count", len(shacl_result['property_shapes']))
                span.set_attribute("constraints_count", len(shacl_result['constraints']))
                span.set_attribute("success", True)
                
                return shacl_result
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
                
                signatures = self.dspy_processor.parse(dspy_xml)
                modules = self.dspy_processor.parse_modules(dspy_xml)
                model_config = self.dspy_processor.parse_model_configuration(dspy_xml)
                validation_rules = self.dspy_processor.parse_validation_rules(dspy_xml)
                
                span.set_attribute("signatures_count", len(signatures))
                span.set_attribute("modules_count", len(modules))
                span.set_attribute("validation_rules_count", len(validation_rules))
                span.set_attribute("success", True)
                
                return {
                    "signatures": signatures,
                    "modules": modules,
                    "model_configuration": model_config,
                    "validation_rules": validation_rules,
                    "metadata": {
                        "signatures_count": len(signatures),
                        "modules_count": len(modules),
                        "validation_rules_count": len(validation_rules)
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
            dspy_modules = []
            model_config = None
            
            # Process OWL
            if owl_xml and owl_xml.strip():
                try:
                    ontology_def = self.owl_processor.parse_ontology_definition(owl_xml)
                except Exception as e:
                    self.telemetry.record_metric("pipeline_owl_processing_failure", 1)
                    raise
            
            # Process SHACL
            if shacl_xml and shacl_xml.strip():
                try:
                    shacl_result = self.shacl_processor.parse(shacl_xml)
                    shacl_graph = shacl_result
                except Exception as e:
                    self.telemetry.record_metric("pipeline_shacl_processing_failure", 1)
                    raise
            
            # Process DSPy
            if dspy_xml and dspy_xml.strip():
                try:
                    dspy_signatures = self.dspy_processor.parse(dspy_xml)
                    dspy_modules = self.dspy_processor.parse_modules(dspy_xml)
                    model_config = self.dspy_processor.parse_model_configuration(dspy_xml)
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
                    ontology_schema=ontology_schema,
                    validation_rules=validation_rules,
                    dspy_signatures=dspy_signatures,
                    dspy_modules=dspy_modules,
                    model_config=model_config
                )
        
        # Stage 3: Linker
        with self.telemetry.start_span(
            name="pipeline.linker",
            operation_type="LINKER",
            stage="linker"
        ):
            executable_system = None
            if dspy_signature:
                executable_system = self.linker.link(dspy_signature)
        
        # Stage 4: Executor
        with self.telemetry.start_span(
            name="pipeline.executor",
            operation_type="EXECUTOR",
            stage="executor"
        ):
            result = None
            if executable_system:
                result = self.executor.execute(executable_system, inputs)
            else:
                # Return a basic result if no executable system
                result = ExecutionResult(
                    success=False,
                    outputs={},
                    errors=["No executable system generated from inputs"],
                    metadata={"stage": "executor", "reason": "no_executable_system"}
                )
        
        with self.telemetry.start_span(
            name="pipeline.complete",
            operation_type="COMPLETE",
            stage="complete"
        ):
            pass
        return result

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