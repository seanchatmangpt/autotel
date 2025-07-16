# AutoTel Processor ABC (Abstract Base Class) Unification Plan

## 🎯 **Project Overview**

This plan unifies AutoTel's processor architecture using **Abstract Base Classes (ABC)** to establish consistent interfaces, reduce code duplication, and improve maintainability across all processor types (BPMN, DMN, DSPy, SHACL, OWL, Jinja, OTEL).

## 📊 **Current Processor Architecture Analysis**

### **Processor Classes Identified**
```
autotel/factory/processors/
├── BPMNProcessor          # BPMN 2.0 XML parsing (90 lines)
├── DMNProcessor           # Decision Model and Notation (197 lines)
├── DSPyProcessor          # DSPy signature/module parsing (391 lines)
├── SHACLProcessor         # SHACL constraint validation (896 lines)
├── OWLProcessor           # OWL ontology processing (527 lines)
├── JinjaProcessor         # Jinja2 template processing (361 lines)
└── OTELProcessor          # OpenTelemetry integration (587 lines)
```

### **Current Inconsistencies**
1. **Interface Inconsistency**: Different method names and signatures
2. **Telemetry Integration**: Inconsistent telemetry patterns
3. **Error Handling**: Varying error handling approaches
4. **Configuration**: Different initialization patterns
5. **Validation**: Inconsistent input/output validation
6. **Documentation**: Varying docstring formats and completeness

### **Common Patterns Identified**
- All processors have telemetry integration
- All processors parse XML or structured content
- All processors return structured data objects
- All processors have error handling with spans
- All processors support file and string input

## 🎯 **ABC Unification Objectives**

### **Primary Goals**
1. **Consistent Interface**: Standardized method signatures across all processors
2. **Code Reuse**: Shared functionality through base classes
3. **Type Safety**: Proper type hints and validation
4. **Telemetry Standardization**: Unified telemetry patterns
5. **Error Handling**: Consistent error handling and reporting
6. **Configuration**: Standardized configuration management

### **Secondary Goals**
1. **Performance**: Optimized shared operations
2. **Testing**: Simplified testing with common test patterns
3. **Documentation**: Standardized documentation format
4. **Extensibility**: Easy addition of new processor types
5. **Maintainability**: Reduced code duplication and complexity

## 🔧 **ABC Architecture Design**

### **Core Abstract Base Classes**

```python
from abc import ABC, abstractmethod
from typing import Dict, Any, Optional, Union, List
from pathlib import Path
from dataclasses import dataclass

@dataclass
class ProcessorConfig:
    """Standard configuration for all processors."""
    service_name: str
    enable_telemetry: bool = True
    enable_validation: bool = True
    enable_caching: bool = False
    cache_size: int = 1000
    timeout_seconds: Optional[int] = None

@dataclass
class ProcessingResult:
    """Standard result structure for all processors."""
    success: bool
    data: Any
    metadata: Dict[str, Any]
    errors: List[str]
    warnings: List[str]
    processing_time_ms: float

class BaseProcessor(ABC):
    """Abstract base class for all AutoTel processors."""
    
    def __init__(self, config: ProcessorConfig, telemetry_manager: Optional[TelemetryManager] = None):
        self.config = config
        self.telemetry = telemetry_manager or self._create_telemetry_manager()
        self._cache = {} if config.enable_caching else None
    
    @abstractmethod
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process content and return standardized result."""
        pass
    
    @abstractmethod
    def validate_input(self, content: Union[str, Path]) -> bool:
        """Validate input content before processing."""
        pass
    
    @abstractmethod
    def get_supported_formats(self) -> List[str]:
        """Return list of supported input formats."""
        pass
    
    def process_file(self, file_path: Path) -> ProcessingResult:
        """Process file with standard file handling."""
        return self._process_with_telemetry(
            lambda: self._process_file_internal(file_path),
            operation_name="process_file",
            file_path=str(file_path)
        )
    
    def process_string(self, content: str) -> ProcessingResult:
        """Process string content with standard handling."""
        return self._process_with_telemetry(
            lambda: self._process_string_internal(content),
            operation_name="process_string",
            content_length=len(content)
        )

class XMLProcessor(BaseProcessor):
    """Base class for XML-based processors."""
    
    def __init__(self, config: ProcessorConfig, namespaces: Dict[str, str], telemetry_manager: Optional[TelemetryManager] = None):
        super().__init__(config, telemetry_manager)
        self.namespaces = namespaces
        self.xml_parser = self._create_xml_parser()
    
    def validate_input(self, content: Union[str, Path]) -> bool:
        """Validate XML content."""
        try:
            if isinstance(content, Path):
                with open(content, 'r') as f:
                    content_str = f.read()
            else:
                content_str = content
            
            ET.fromstring(content_str)
            return True
        except ET.ParseError:
            return False
    
    def get_supported_formats(self) -> List[str]:
        return ["xml", "xml_string"]

class StructuredDataProcessor(BaseProcessor):
    """Base class for structured data processors (JSON, YAML, etc.)."""
    
    def __init__(self, config: ProcessorConfig, supported_formats: List[str], telemetry_manager: Optional[TelemetryManager] = None):
        super().__init__(config, telemetry_manager)
        self.supported_formats = supported_formats
    
    def get_supported_formats(self) -> List[str]:
        return self.supported_formats
```

### **Specialized Processor Classes**

```python
class BPMNProcessor(XMLProcessor):
    """Unified BPMN processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        bpmn_config = config or ProcessorConfig(
            service_name="autotel-bpmn-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        bpmn_namespaces = {
            'bpmn': 'http://www.omg.org/spec/BPMN/20100524/MODEL',
            'bpmndi': 'http://www.omg.org/spec/BPMN/20100524/DI',
            'camunda': 'http://camunda.org/schema/1.0/bpmn'
        }
        
        super().__init__(bpmn_config, bpmn_namespaces, telemetry_manager)
        self.spiff_parser = CamundaParser()
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process BPMN content and return workflow specifications."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal BPMN string processing."""
        start_time = time.time()
        
        try:
            # Parse XML
            bpmn_tree = ET.fromstring(content)
            
            # Add to SpiffWorkflow parser
            self.spiff_parser.add_bpmn_xml(bpmn_tree)
            
            # Extract workflow specs
            workflow_specs = {}
            for spec in self.spiff_parser.get_specs():
                workflow_specs[spec.name] = spec
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=workflow_specs,
                metadata={
                    "workflow_count": len(workflow_specs),
                    "input_size_bytes": len(content.encode('utf-8')),
                    "parser_type": "SpiffWorkflow.CamundaParser"
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class DMNProcessor(XMLProcessor):
    """Unified DMN processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        dmn_config = config or ProcessorConfig(
            service_name="autotel-dmn-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        dmn_namespaces = {
            'dmn': 'http://www.omg.org/spec/DMN/20191111/MODEL/',
            'camunda': 'http://camunda.org/schema/1.0/dmn'
        }
        
        super().__init__(dmn_config, dmn_namespaces, telemetry_manager)
        self.spiff_parser = CamundaParser()
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process DMN content and return decision tables."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal DMN string processing."""
        start_time = time.time()
        
        try:
            # Parse XML
            dmn_tree = ET.fromstring(content)
            
            # Add to SpiffWorkflow parser
            self.spiff_parser.add_dmn_xml(dmn_tree)
            
            # Extract decisions
            decisions = {}
            for dmn_parser in getattr(self.spiff_parser, 'dmn_parsers', {}).values():
                if hasattr(dmn_parser, 'decision') and hasattr(dmn_parser.decision, 'id'):
                    decisions[dmn_parser.decision.id] = dmn_parser.decision
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=decisions,
                metadata={
                    "decision_count": len(decisions),
                    "input_size_bytes": len(content.encode('utf-8')),
                    "parser_type": "SpiffWorkflow.CamundaParser"
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class DSPyProcessor(XMLProcessor):
    """Unified DSPy processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        dspy_config = config or ProcessorConfig(
            service_name="autotel-dspy-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        dspy_namespaces = {
            'dspy': 'http://autotel.ai/dspy',
            'owl': 'http://www.w3.org/2002/07/owl#',
            'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
            'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
            'shacl': 'http://www.w3.org/ns/shacl#'
        }
        
        super().__init__(dspy_config, dspy_namespaces, telemetry_manager)
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process DSPy content and return signature definitions."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal DSPy string processing."""
        start_time = time.time()
        
        try:
            # Parse signatures, modules, and configuration
            signatures = self._parse_signatures(content)
            modules = self._parse_modules(content)
            model_config = self._parse_model_configuration(content)
            validation_rules = self._parse_validation_rules(content)
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data={
                    "signatures": signatures,
                    "modules": modules,
                    "model_configuration": model_config,
                    "validation_rules": validation_rules
                },
                metadata={
                    "signatures_count": len(signatures),
                    "modules_count": len(modules),
                    "validation_rules_count": len(validation_rules),
                    "input_size_bytes": len(content.encode('utf-8'))
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class SHACLProcessor(XMLProcessor):
    """Unified SHACL processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        shacl_config = config or ProcessorConfig(
            service_name="autotel-shacl-processor",
            enable_telemetry=True,
            enable_validation=True,
            enable_caching=True,
            cache_size=1000
        )
        
        shacl_namespaces = {
            'shacl': 'http://www.w3.org/ns/shacl#',
            'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
            'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
            'owl': 'http://www.w3.org/2002/07/owl#'
        }
        
        super().__init__(shacl_config, shacl_namespaces, telemetry_manager)
        self.validators = self._initialize_validators()
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process SHACL content and return validation rules."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal SHACL string processing."""
        start_time = time.time()
        
        try:
            # Parse SHACL constraints
            shacl_result = self._parse_shacl_constraints(content)
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=shacl_result,
                metadata={
                    "node_shapes_count": len(shacl_result.get('node_shapes', [])),
                    "property_shapes_count": len(shacl_result.get('property_shapes', [])),
                    "constraints_count": len(shacl_result.get('constraints', [])),
                    "input_size_bytes": len(content.encode('utf-8'))
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class OWLProcessor(XMLProcessor):
    """Unified OWL processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        owl_config = config or ProcessorConfig(
            service_name="autotel-owl-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        owl_namespaces = {
            'owl': 'http://www.w3.org/2002/07/owl#',
            'rdf': 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
            'rdfs': 'http://www.w3.org/2000/01/rdf-schema#',
            'xml': 'http://www.w3.org/XML/1998/namespace'
        }
        
        super().__init__(owl_config, owl_namespaces, telemetry_manager)
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process OWL content and return ontology definitions."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal OWL string processing."""
        start_time = time.time()
        
        try:
            # Parse ontology definition
            ontology_def = self._parse_ontology_definition(content)
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=ontology_def,
                metadata={
                    "classes_count": len(ontology_def.classes),
                    "object_properties_count": len(ontology_def.object_properties),
                    "data_properties_count": len(ontology_def.data_properties),
                    "individuals_count": len(ontology_def.individuals),
                    "axioms_count": len(ontology_def.axioms),
                    "input_size_bytes": len(content.encode('utf-8'))
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class JinjaProcessor(StructuredDataProcessor):
    """Unified Jinja processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        jinja_config = config or ProcessorConfig(
            service_name="autotel-jinja-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        super().__init__(jinja_config, ["xml", "xml_string"], telemetry_manager)
        self.jinja_env = self._create_jinja_environment()
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process Jinja content and return template definitions."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal Jinja string processing."""
        start_time = time.time()
        
        try:
            # Parse template definitions
            template_def = self._parse_template_definitions(content)
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=template_def,
                metadata={
                    "templates_count": len(template_def.templates),
                    "global_variables_count": len(template_def.global_variables),
                    "input_size_bytes": len(content.encode('utf-8'))
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )

class OTELProcessor(StructuredDataProcessor):
    """Unified OTEL processor with ABC compliance."""
    
    def __init__(self, config: Optional[ProcessorConfig] = None, telemetry_manager: Optional[TelemetryManager] = None):
        otel_config = config or ProcessorConfig(
            service_name="autotel-otel-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        
        super().__init__(otel_config, ["json", "json_string", "otel_export"], telemetry_manager)
    
    def process(self, content: Union[str, Path]) -> ProcessingResult:
        """Process OTEL content and return telemetry analysis."""
        if isinstance(content, Path):
            return self.process_file(content)
        else:
            return self.process_string(content)
    
    def _process_string_internal(self, content: str) -> ProcessingResult:
        """Internal OTEL string processing."""
        start_time = time.time()
        
        try:
            # Parse OTEL data
            otel_data = json.loads(content)
            
            # Analyze spans, metrics, and logs
            analysis_result = self._analyze_otel_data(otel_data)
            
            processing_time = (time.time() - start_time) * 1000
            
            return ProcessingResult(
                success=True,
                data=analysis_result,
                metadata={
                    "spans_count": len(analysis_result.get('spans', [])),
                    "metrics_count": len(analysis_result.get('metrics', [])),
                    "logs_count": len(analysis_result.get('logs', [])),
                    "input_size_bytes": len(content.encode('utf-8'))
                },
                errors=[],
                warnings=[],
                processing_time_ms=processing_time
            )
            
        except Exception as e:
            processing_time = (time.time() - start_time) * 1000
            return ProcessingResult(
                success=False,
                data={},
                metadata={"processing_time_ms": processing_time},
                errors=[str(e)],
                warnings=[],
                processing_time_ms=processing_time
            )
```

## 🔧 **Shared Infrastructure**

### **Common Utilities**

```python
class ProcessorRegistry:
    """Registry for managing all processor instances."""
    
    def __init__(self):
        self._processors: Dict[str, BaseProcessor] = {}
        self._configs: Dict[str, ProcessorConfig] = {}
    
    def register_processor(self, name: str, processor: BaseProcessor, config: ProcessorConfig):
        """Register a processor with its configuration."""
        self._processors[name] = processor
        self._configs[name] = config
    
    def get_processor(self, name: str) -> Optional[BaseProcessor]:
        """Get a registered processor by name."""
        return self._processors.get(name)
    
    def list_processors(self) -> List[str]:
        """List all registered processor names."""
        return list(self._processors.keys())
    
    def get_processor_config(self, name: str) -> Optional[ProcessorConfig]:
        """Get configuration for a processor."""
        return self._configs.get(name)

class ProcessorFactory:
    """Factory for creating processor instances with consistent configuration."""
    
    def __init__(self, telemetry_manager: Optional[TelemetryManager] = None):
        self.telemetry_manager = telemetry_manager
        self.registry = ProcessorRegistry()
        self._initialize_default_processors()
    
    def _initialize_default_processors(self):
        """Initialize all default processors."""
        # BPMN Processor
        bpmn_config = ProcessorConfig(
            service_name="autotel-bpmn-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        bpmn_processor = BPMNProcessor(bpmn_config, self.telemetry_manager)
        self.registry.register_processor("bpmn", bpmn_processor, bpmn_config)
        
        # DMN Processor
        dmn_config = ProcessorConfig(
            service_name="autotel-dmn-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        dmn_processor = DMNProcessor(dmn_config, self.telemetry_manager)
        self.registry.register_processor("dmn", dmn_processor, dmn_config)
        
        # DSPy Processor
        dspy_config = ProcessorConfig(
            service_name="autotel-dspy-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        dspy_processor = DSPyProcessor(dspy_config, self.telemetry_manager)
        self.registry.register_processor("dspy", dspy_processor, dspy_config)
        
        # SHACL Processor
        shacl_config = ProcessorConfig(
            service_name="autotel-shacl-processor",
            enable_telemetry=True,
            enable_validation=True,
            enable_caching=True
        )
        shacl_processor = SHACLProcessor(shacl_config, self.telemetry_manager)
        self.registry.register_processor("shacl", shacl_processor, shacl_config)
        
        # OWL Processor
        owl_config = ProcessorConfig(
            service_name="autotel-owl-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        owl_processor = OWLProcessor(owl_config, self.telemetry_manager)
        self.registry.register_processor("owl", owl_processor, owl_config)
        
        # Jinja Processor
        jinja_config = ProcessorConfig(
            service_name="autotel-jinja-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        jinja_processor = JinjaProcessor(jinja_config, self.telemetry_manager)
        self.registry.register_processor("jinja", jinja_processor, jinja_config)
        
        # OTEL Processor
        otel_config = ProcessorConfig(
            service_name="autotel-otel-processor",
            enable_telemetry=True,
            enable_validation=True
        )
        otel_processor = OTELProcessor(otel_config, self.telemetry_manager)
        self.registry.register_processor("otel", otel_processor, otel_config)
    
    def create_processor(self, processor_type: str, config: Optional[ProcessorConfig] = None) -> BaseProcessor:
        """Create a processor instance with optional custom configuration."""
        if config:
            # Create new instance with custom config
            processor_classes = {
                "bpmn": BPMNProcessor,
                "dmn": DMNProcessor,
                "dspy": DSPyProcessor,
                "shacl": SHACLProcessor,
                "owl": OWLProcessor,
                "jinja": JinjaProcessor,
                "otel": OTELProcessor
            }
            
            processor_class = processor_classes.get(processor_type)
            if not processor_class:
                raise ValueError(f"Unknown processor type: {processor_type}")
            
            return processor_class(config, self.telemetry_manager)
        else:
            # Return registered instance
            processor = self.registry.get_processor(processor_type)
            if not processor:
                raise ValueError(f"Processor not found: {processor_type}")
            return processor
    
    def process_with_type(self, processor_type: str, content: Union[str, Path]) -> ProcessingResult:
        """Process content using a specific processor type."""
        processor = self.create_processor(processor_type)
        return processor.process(content)
```

### **Enhanced Pipeline Integration**

```python
class UnifiedPipelineOrchestrator:
    """Enhanced pipeline orchestrator using unified processors."""
    
    def __init__(self, telemetry_manager: Optional[TelemetryManager] = None):
        self.telemetry_manager = telemetry_manager or create_telemetry_manager()
        self.processor_factory = ProcessorFactory(self.telemetry_manager)
        self.compilers = self._initialize_compilers()
        self.linker = SemanticLinker()
        self.executor = OntologyExecutor()
    
    def _initialize_compilers(self):
        """Initialize all compilers."""
        return {
            "ontology": OntologyCompiler(),
            "validation": ValidationCompiler(),
            "dspy": DSPyCompiler()
        }
    
    def process_content(self, content_type: str, content: Union[str, Path]) -> ProcessingResult:
        """Process content using the appropriate processor."""
        return self.processor_factory.process_with_type(content_type, content)
    
    def execute_pipeline(
        self,
        owl_content: Optional[Union[str, Path]] = None,
        shacl_content: Optional[Union[str, Path]] = None,
        dspy_content: Optional[Union[str, Path]] = None,
        inputs: Optional[Dict[str, Any]] = None
    ) -> ExecutionResult:
        """Execute the complete pipeline with unified processors."""
        
        with self.telemetry_manager.start_span(
            name="unified_pipeline.execute",
            operation_type="PIPELINE_EXECUTION",
            stage="pipeline_execution"
        ) as span:
            
            # Stage 1: Processors
            processor_results = {}
            
            if owl_content:
                owl_result = self.process_content("owl", owl_content)
                processor_results["owl"] = owl_result
                span.set_attribute("owl_success", owl_result.success)
                span.set_attribute("owl_classes_count", owl_result.metadata.get("classes_count", 0))
            
            if shacl_content:
                shacl_result = self.process_content("shacl", shacl_content)
                processor_results["shacl"] = shacl_result
                span.set_attribute("shacl_success", shacl_result.success)
                span.set_attribute("shacl_constraints_count", shacl_result.metadata.get("constraints_count", 0))
            
            if dspy_content:
                dspy_result = self.process_content("dspy", dspy_content)
                processor_results["dspy"] = dspy_result
                span.set_attribute("dspy_success", dspy_result.success)
                span.set_attribute("dspy_signatures_count", dspy_result.metadata.get("signatures_count", 0))
            
            # Stage 2: Compilers
            compiler_results = {}
            
            if "owl" in processor_results and processor_results["owl"].success:
                ontology_schema = self.compilers["ontology"].compile(processor_results["owl"].data)
                compiler_results["ontology"] = ontology_schema
            
            if "shacl" in processor_results and processor_results["shacl"].success:
                validation_rules = self.compilers["validation"].compile(processor_results["shacl"].data)
                compiler_results["validation"] = validation_rules
            
            if "dspy" in processor_results and processor_results["dspy"].success:
                dspy_signature = self.compilers["dspy"].compile(
                    compiler_results.get("ontology"),
                    compiler_results.get("validation"),
                    processor_results["dspy"].data
                )
                compiler_results["dspy"] = dspy_signature
            
            # Stage 3: Linker
            if all(key in compiler_results for key in ["ontology", "validation", "dspy"]):
                linked_system = self.linker.link(
                    compiler_results["ontology"],
                    compiler_results["validation"],
                    compiler_results["dspy"]
                )
                
                # Stage 4: Executor
                execution_result = self.executor.execute(linked_system, inputs or {})
                
                span.set_attribute("pipeline_success", True)
                span.set_attribute("execution_success", execution_result.success)
                
                return execution_result
            else:
                span.set_attribute("pipeline_success", False)
                span.set_attribute("missing_components", str(list(set(["ontology", "validation", "dspy"]) - set(compiler_results.keys()))))
                
                return ExecutionResult(
                    success=False,
                    data={},
                    errors=["Missing required components for pipeline execution"],
                    metadata={"processor_results": processor_results, "compiler_results": compiler_results}
                )
```

## 🧪 **Testing Strategy**

### **Common Test Patterns**

```python
import pytest
from pathlib import Path
from unittest.mock import Mock, patch

class BaseProcessorTest:
    """Base test class for all processors."""
    
    @pytest.fixture
    def processor(self):
        """Create processor instance for testing."""
        raise NotImplementedError("Subclasses must implement processor fixture")
    
    @pytest.fixture
    def sample_content(self):
        """Provide sample content for testing."""
        raise NotImplementedError("Subclasses must implement sample_content fixture")
    
    def test_processor_initialization(self, processor):
        """Test processor initialization."""
        assert processor is not None
        assert processor.config is not None
        assert processor.telemetry is not None
    
    def test_validate_input_valid(self, processor, sample_content):
        """Test input validation with valid content."""
        assert processor.validate_input(sample_content) is True
    
    def test_validate_input_invalid(self, processor):
        """Test input validation with invalid content."""
        invalid_content = "invalid content"
        assert processor.validate_input(invalid_content) is False
    
    def test_process_success(self, processor, sample_content):
        """Test successful processing."""
        result = processor.process(sample_content)
        
        assert isinstance(result, ProcessingResult)
        assert result.success is True
        assert result.data is not None
        assert result.errors == []
        assert result.processing_time_ms > 0
    
    def test_process_failure(self, processor):
        """Test processing failure."""
        invalid_content = "invalid content"
        result = processor.process(invalid_content)
        
        assert isinstance(result, ProcessingResult)
        assert result.success is False
        assert len(result.errors) > 0
    
    def test_get_supported_formats(self, processor):
        """Test supported formats listing."""
        formats = processor.get_supported_formats()
        assert isinstance(formats, list)
        assert len(formats) > 0

class BPMNProcessorTest(BaseProcessorTest):
    """Test class for BPMN processor."""
    
    @pytest.fixture
    def processor(self):
        return BPMNProcessor()
    
    @pytest.fixture
    def sample_content(self):
        return """
        <?xml version="1.0" encoding="UTF-8"?>
        <bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL">
            <bpmn:process id="Process_1" name="Sample Process">
                <bpmn:startEvent id="StartEvent_1" name="Start"/>
                <bpmn:endEvent id="EndEvent_1" name="End"/>
            </bpmn:process>
        </bpmn:definitions>
        """
    
    def test_process_bpmn_workflow(self, processor, sample_content):
        """Test BPMN workflow processing."""
        result = processor.process(sample_content)
        
        assert result.success is True
        assert "Process_1" in result.data
        assert result.metadata["workflow_count"] == 1

class DMNProcessorTest(BaseProcessorTest):
    """Test class for DMN processor."""
    
    @pytest.fixture
    def processor(self):
        return DMNProcessor()
    
    @pytest.fixture
    def sample_content(self):
        return """
        <?xml version="1.0" encoding="UTF-8"?>
        <dmn:definitions xmlns:dmn="http://www.omg.org/spec/DMN/20191111/MODEL/">
            <dmn:decision id="Decision_1" name="Sample Decision">
                <dmn:decisionTable id="DecisionTable_1">
                    <dmn:input id="Input_1" label="Input"/>
                    <dmn:output id="Output_1" label="Output"/>
                </dmn:decisionTable>
            </dmn:decision>
        </dmn:definitions>
        """
    
    def test_process_dmn_decision(self, processor, sample_content):
        """Test DMN decision processing."""
        result = processor.process(sample_content)
        
        assert result.success is True
        assert "Decision_1" in result.data
        assert result.metadata["decision_count"] == 1
```

## 📊 **Implementation Roadmap**

### **Phase 1: Foundation (Week 1-2)**
- [ ] Create abstract base classes (`BaseProcessor`, `XMLProcessor`, `StructuredDataProcessor`)
- [ ] Implement `ProcessorConfig` and `ProcessingResult` dataclasses
- [ ] Create `ProcessorRegistry` and `ProcessorFactory`
- [ ] Implement shared utilities and common functionality
- [ ] Create comprehensive test framework

### **Phase 2: Processor Migration (Week 3-4)**
- [ ] Migrate `BPMNProcessor` to ABC compliance
- [ ] Migrate `DMNProcessor` to ABC compliance
- [ ] Migrate `DSPyProcessor` to ABC compliance
- [ ] Migrate `SHACLProcessor` to ABC compliance
- [ ] Migrate `OWLProcessor` to ABC compliance
- [ ] Migrate `JinjaProcessor` to ABC compliance
- [ ] Migrate `OTELProcessor` to ABC compliance

### **Phase 3: Pipeline Integration (Week 5)**
- [ ] Update `PipelineOrchestrator` to use unified processors
- [ ] Implement `UnifiedPipelineOrchestrator`
- [ ] Update CLI commands to use factory pattern
- [ ] Update all tests to use new interfaces

### **Phase 4: Optimization & Validation (Week 6)**
- [ ] Performance optimization and caching
- [ ] Comprehensive integration testing
- [ ] Documentation updates
- [ ] Code review and cleanup

## 🎯 **Success Metrics**

### **Code Quality Metrics**
- **Reduction in Code Duplication**: Target 60% reduction in processor code
- **Consistent Interface**: 100% method signature consistency across processors
- **Type Safety**: 100% type hint coverage
- **Test Coverage**: >90% test coverage for all processors

### **Performance Metrics**
- **Processing Speed**: Maintain or improve current processing times
- **Memory Usage**: Reduce memory footprint by 20%
- **Telemetry Overhead**: <5% performance impact from telemetry

### **Maintainability Metrics**
- **Documentation Coverage**: 100% public method documentation
- **Error Handling**: Consistent error handling across all processors
- **Configuration**: Standardized configuration management
- **Extensibility**: Easy addition of new processor types

## 🔧 **Migration Strategy**

### **Backward Compatibility**
- Maintain existing method signatures during transition
- Provide deprecation warnings for old interfaces
- Gradual migration with feature flags
- Comprehensive testing to ensure no regressions

### **Rollout Plan**
1. **Development Phase**: Implement ABC classes alongside existing processors
2. **Testing Phase**: Comprehensive testing with both old and new interfaces
3. **Migration Phase**: Gradual migration of existing code to new interfaces
4. **Cleanup Phase**: Remove old interfaces and clean up deprecated code

### **Risk Mitigation**
- **Feature Flags**: Enable/disable new interfaces during rollout
- **A/B Testing**: Compare performance between old and new implementations
- **Rollback Plan**: Quick rollback capability if issues arise
- **Monitoring**: Enhanced monitoring during migration period

## 📋 **Conclusion**

The Processor ABC Unification Plan provides a comprehensive approach to standardizing AutoTel's processor architecture. By implementing abstract base classes, shared utilities, and consistent interfaces, we can significantly improve code quality, maintainability, and extensibility while maintaining performance and functionality.

The phased implementation approach ensures minimal disruption to existing functionality while providing a clear path to a more robust and maintainable processor architecture. 