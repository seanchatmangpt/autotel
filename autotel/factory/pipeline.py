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
            ontology_def = self.owl_processor.parse_ontology_definition(owl_xml)
            shacl_graph = self.shacl_processor.parse(shacl_xml)
            dspy_signatures = self.dspy_processor.parse(dspy_xml)
            dspy_modules = self.dspy_processor.parse_modules(dspy_xml)
            model_config = self.dspy_processor.parse_model_configuration(dspy_xml)
        # Stage 2: Compilers
        with self.telemetry.start_span(
            name="pipeline.compilers",
            operation_type="COMPILERS",
            stage="compilers"
        ):
            ontology_schema = self.ontology_compiler.compile(ontology_def)
            validation_rules = self.validation_compiler.compile(shacl_graph)
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
            executable_system = self.linker.link(dspy_signature)
        # Stage 4: Executor
        with self.telemetry.start_span(
            name="pipeline.executor",
            operation_type="EXECUTOR",
            stage="executor"
        ):
            result = self.executor.execute(executable_system, inputs)
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
        with open(owl_file, 'r', encoding='utf-8') as f:
            owl_xml = f.read()
        
        with open(shacl_file, 'r', encoding='utf-8') as f:
            shacl_xml = f.read()
        
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