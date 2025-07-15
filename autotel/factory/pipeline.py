"""
AutoTel Factory Pipeline - Best Practice Architecture

Implements proper separation of concerns:
- Parsers: Parse and validate files
- Linker: Wire together parsed objects
- Orchestrator: Execute workflows with telemetry
"""

from typing import List, Dict, Any, Optional
from pathlib import Path
import logging

from .processors.bpmn_processor import BPMNProcessor
from .processors.dmn_processor import DMNProcessor
from .processors.dspy_processor import DSPyProcessor
from .linker import SemanticLinker
from ..core.orchestrator import Orchestrator
from ..core.telemetry import TelemetryManager

logger = logging.getLogger(__name__)

class AutoTelPipeline:
    """
    Best practice pipeline for AutoTel workflow processing
    
    Architecture:
    1. Parse all files (BPMN, DMN, DSPy) into isolated objects
    2. Link parsed objects into executable workflow
    3. Execute workflow with full telemetry
    """
    
    def __init__(self, 
                 telemetry_manager: Optional[TelemetryManager] = None,
                 enable_persistence: bool = True):
        """
        Initialize the AutoTel pipeline
        
        Args:
            telemetry_manager: Telemetry manager instance (optional)
            enable_persistence: Enable process state persistence
        """
        self.telemetry_manager = telemetry_manager or TelemetryManager()
        self.enable_persistence = enable_persistence
        
        # Create shared parser for BPMN and DMN integration
        from SpiffWorkflow.camunda.parser import CamundaParser
        self.shared_parser = CamundaParser()
        
        # Initialize processors with shared parser
        self.bpmn_processor = BPMNProcessor(parser=self.shared_parser)
        self.dmn_processor = DMNProcessor()
        self.dspy_processor = DSPyProcessor()
        
        # Initialize linker
        self.linker = SemanticLinker()
        
        # Parsed objects (will be populated by parse_files)
        self.bpmn_specs: Dict[str, Any] = {}
        self.dmn_decisions: Dict[str, Any] = {}
        self.dspy_signatures: List[Any] = []
        self.dspy_modules: List[Any] = []
        self.dspy_config: Optional[Any] = None
    
    def parse_files(self, 
                   bpmn_files: List[str],
                   dmn_files: Optional[List[str]] = None,
                   dspy_files: Optional[List[str]] = None) -> None:
        """
        Parse all workflow files into isolated objects
        
        Args:
            bpmn_files: List of BPMN XML file paths
            dmn_files: List of DMN XML file paths (optional)
            dspy_files: List of DSPy XML file paths (optional)
        """
        with self.telemetry_manager.start_span("parse_files", "schema_loading") as span:
            try:
                # Parse DMN files first (dependencies for BPMN)
                if dmn_files:
                    for dmn_file in dmn_files:
                        self._parse_dmn_file(dmn_file)
                        span.add_event("parsed_dmn_file", {"file": dmn_file})
                
                # Parse BPMN files
                for bpmn_file in bpmn_files:
                    self._parse_bpmn_file(bpmn_file)
                    span.add_event("parsed_bpmn_file", {"file": bpmn_file})
                
                # Parse DSPy files (if separate from BPMN)
                if dspy_files:
                    for dspy_file in dspy_files:
                        self._parse_dspy_file(dspy_file)
                        span.add_event("parsed_dspy_file", {"file": dspy_file})
                
                logger.info(f"Parsed {len(dmn_files or [])} DMN files, {len(bpmn_files)} BPMN files")
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to parse files: {e}")
                raise
    
    def _parse_dmn_file(self, dmn_file: str) -> None:
        """Parse a single DMN file and load into shared parser"""
        try:
            # Parse DMN file for our own tracking
            decisions = self.dmn_processor.parse_file(dmn_file)
            for decision in decisions:
                self.dmn_decisions[decision.id] = decision
            
            # Load DMN file into shared parser for BPMN integration
            with open(dmn_file, 'r', encoding='utf-8') as f:
                dmn_xml = f.read()
            import lxml.etree as etree
            dmn_tree = etree.fromstring(dmn_xml.encode('utf-8'))
            self.shared_parser.add_dmn_xml(dmn_tree)
            
        except Exception as e:
            logger.error(f"Failed to parse DMN file {dmn_file}: {e}")
            raise
    
    def _parse_bpmn_file(self, bpmn_file: str) -> None:
        """Parse a single BPMN file, extracting all process specs."""
        try:
            # Read BPMN XML
            with open(bpmn_file, 'r', encoding='utf-8') as f:
                bpmn_xml = f.read()
            # Parse XML to find all process IDs
            import lxml.etree as etree
            tree = etree.fromstring(bpmn_xml.encode('utf-8'))
            nsmap = tree.nsmap.copy()
            bpmn_ns = nsmap.get('bpmn', 'http://www.omg.org/spec/BPMN/20100524/MODEL')
            process_elems = tree.findall(f'.//{{{bpmn_ns}}}process')
            for proc in process_elems:
                process_id = proc.get('id')
                if not process_id:
                    continue
                # Parse the process spec
                spec = self.bpmn_processor.parse_file(bpmn_file, process_id)
                self.bpmn_specs[process_id] = spec
            # Extract DSPy signatures and modules from BPMN
            signatures = self.dspy_processor.find_signatures_in_bpmn(bpmn_xml)
            self.dspy_signatures.extend(signatures)
            modules = self.dspy_processor.parse_modules_in_bpmn(bpmn_xml)
            self.dspy_modules.extend(modules)
            config = self.dspy_processor.parse_model_configuration(bpmn_xml)
            if config and not self.dspy_config:
                self.dspy_config = config
            
            # Inject DSPy modules into BPMN tasks
            self._inject_dspy_modules_into_bpmn(bpmn_xml, modules)
        except Exception as e:
            logger.error(f"Failed to parse BPMN file {bpmn_file}: {e}")
            raise
    
    def _parse_dspy_file(self, dspy_file: str) -> None:
        """Parse a single DSPy file"""
        try:
            signatures = self.dspy_processor.parse_file(dspy_file)
            self.dspy_signatures.extend(signatures)
        except Exception as e:
            logger.error(f"Failed to parse DSPy file {dspy_file}: {e}")
            raise
    
    def _inject_dspy_modules_into_bpmn(self, bpmn_xml: str, dspy_modules: List[Any]) -> None:
        """Inject DSPy modules into BPMN task specifications"""
        try:
            # Parse BPMN XML to find service tasks
            import lxml.etree as etree
            root = etree.fromstring(bpmn_xml.encode('utf-8'))
            
            # Find all service tasks
            bpmn_ns = 'http://www.omg.org/spec/BPMN/20100524/MODEL'
            dspy_ns = 'http://autotel.ai/dspy'
            service_tasks = root.xpath('.//bpmn:serviceTask', namespaces={'bpmn': bpmn_ns})
            
            # Create a mapping of task IDs to DSPy modules
            task_to_module = {}
            for task in service_tasks:
                task_id = task.get('id')
                if task_id:
                    # Look for DSPy modules in this task's extension elements
                    extension_elements = task.xpath('.//bpmn:extensionElements', namespaces={'bpmn': bpmn_ns})
                    
                    for ext_elem in extension_elements:
                        # Check for dspy:predict
                        predict_elements = ext_elem.xpath('.//dspy:predict', namespaces={'dspy': dspy_ns})
                        for pred_elem in predict_elements:
                            signature_name = pred_elem.get('signature')
                            result_variable = pred_elem.get('result')
                            if signature_name and result_variable:
                                # Find the corresponding DSPy module
                                for module in dspy_modules:
                                    if (module.signature_name == signature_name and 
                                        module.result_variable == result_variable):
                                        task_to_module[task_id] = module
                                        break
                        
                        # Check for dspy:chain-of-thought
                        cot_elements = ext_elem.xpath('.//dspy:chain-of-thought', namespaces={'dspy': dspy_ns})
                        for cot_elem in cot_elements:
                            signature_name = cot_elem.get('signature')
                            result_variable = cot_elem.get('result')
                            if signature_name and result_variable:
                                # Find the corresponding DSPy module
                                for module in dspy_modules:
                                    if (module.signature_name == signature_name and 
                                        module.result_variable == result_variable):
                                        task_to_module[task_id] = module
                                        break
            
            # Inject DSPy modules into BPMN task specifications
            for process_id, bpmn_spec in self.bpmn_specs.items():
                for task_spec in bpmn_spec.task_specs.values():
                    if hasattr(task_spec, 'id') and task_spec.id in task_to_module:
                        module = task_to_module[task_spec.id]
                        
                        # Create DSPy service extension
                        dspy_service = {
                            'signature': module.signature_name,
                            'module_type': module.module_type,
                            'result_variable': module.result_variable,
                            'parameters': module.parameters or {}
                        }
                        
                        # Initialize extensions if not present
                        if not hasattr(task_spec, 'extensions'):
                            task_spec.extensions = {}
                        
                        # Inject DSPy service
                        task_spec.extensions['dspy_service'] = dspy_service
                        
        except Exception as e:
            logger.error(f"Failed to inject DSPy modules into BPMN: {e}")
            # Don't raise - this is not critical for basic functionality
    
    def link_workflow(self) -> Any:
        """
        Link parsed objects into an executable workflow
        
        Returns:
            Linked workflow specification
        """
        with self.telemetry_manager.start_span("link_workflow", "schema_loading") as span:
            try:
                # Link BPMN specs with DMN decisions and DSPy signatures
                linked_specs = {}
                
                for process_id, bpmn_spec in self.bpmn_specs.items():
                    linked_spec = self.linker.link(
                        bpmn_spec=bpmn_spec,
                        dmn_decisions=self.dmn_decisions,
                        dspy_signatures=self.dspy_signatures
                    )
                    linked_specs[process_id] = linked_spec
                
                span.add_event("linked_workflow", {
                    "process_count": len(linked_specs),
                    "dmn_count": len(self.dmn_decisions),
                    "dspy_signature_count": len(self.dspy_signatures)
                })
                
                logger.info(f"Linked {len(linked_specs)} process specifications")
                return linked_specs
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to link workflow: {e}")
                raise
    
    def create_orchestrator(self, 
                          linked_specs: Dict[str, Any],
                          dspy_config: Optional[Any] = None) -> Orchestrator:
        """
        Create an orchestrator with linked process definitions
        
        Args:
            linked_specs: Linked process definitions/specs
            dspy_config: DSPy model configuration
            
        Returns:
            Orchestrator: Configured orchestrator instance
        """
        return Orchestrator(
            process_definitions=linked_specs,
            telemetry_manager=self.telemetry_manager,
            enable_persistence=self.enable_persistence,
            dspy_config=dspy_config
        )
    
    def execute_workflow(self, 
                        bpmn_files: List[str],
                        dmn_files: Optional[List[str]] = None,
                        dspy_files: Optional[List[str]] = None,
                        process_id: str = None,
                        variables: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """
        Execute a complete workflow end-to-end
        
        Args:
            bpmn_files: List of BPMN XML file paths
            dmn_files: List of DMN XML file paths (optional)
            dspy_files: List of DSPy XML file paths (optional)
            process_id: ID of the process to execute (uses first if not specified)
            variables: Initial process variables
            
        Returns:
            Dict[str, Any]: Execution results and telemetry
        """
        with self.telemetry_manager.start_span("execute_workflow", "schema_loading") as span:
            try:
                # Parse all files
                self.parse_files(bpmn_files, dmn_files, dspy_files)
                
                # Link workflow
                linked_specs = self.link_workflow()
                
                # Create orchestrator
                orchestrator = self.create_orchestrator(linked_specs)
                
                # Determine process to execute
                if not process_id:
                    process_id = list(linked_specs.keys())[0]
                
                # Start process
                instance = orchestrator.start_process(process_id, variables)
                
                # Execute process
                result = orchestrator.execute_process(instance.instance_id)
                
                # Export telemetry
                telemetry = orchestrator.export_telemetry()
                
                span.add_event("workflow_executed", {
                    "process_id": process_id,
                    "instance_id": instance.instance_id,
                    "status": result.status.value
                })
                
                return {
                    "process_id": process_id,
                    "instance_id": instance.instance_id,
                    "status": result.status.value,
                    "variables": result.variables,
                    "execution_path": result.execution_path,
                    "telemetry": telemetry
                }
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to execute workflow: {e}")
                raise
    
    def validate_files(self, 
                      bpmn_files: List[str],
                      dmn_files: Optional[List[str]] = None,
                      dspy_files: Optional[List[str]] = None) -> Dict[str, Any]:
        """
        Validate all workflow files without execution
        
        Args:
            bpmn_files: List of BPMN XML file paths
            dmn_files: List of DMN XML file paths (optional)
            dspy_files: List of DSPy XML file paths (optional)
            
        Returns:
            Dict[str, Any]: Validation results
        """
        with self.telemetry_manager.start_span("validate_files", "constraint_validation") as span:
            try:
                # Parse all files
                self.parse_files(bpmn_files, dmn_files, dspy_files)
                
                # Link workflow (this will validate references)
                linked_specs = self.link_workflow()
                
                # Validate linker results for each BPMN spec
                validation_reports = []
                for bpmn_spec in self.bpmn_specs.values():
                    validation_report = self.linker.validate_linkage(
                        bpmn_spec=bpmn_spec,
                        dmn_decisions=self.dmn_decisions,
                        dspy_signatures=self.dspy_signatures
                    )
                    validation_reports.append(validation_report)
                
                # Combine validation reports
                combined_report = {
                    'valid': all(r.get('valid', False) for r in validation_reports),
                    'total_tasks': sum(r.get('total_tasks', 0) for r in validation_reports),
                    'missing_dmn_refs': [],
                    'missing_dspy_refs': [],
                    'available_dmn_decisions': list(self.dmn_decisions.keys()),
                    'available_dspy_signatures': [s.name for s in self.dspy_signatures]
                }
                
                # Collect all missing references
                for report in validation_reports:
                    combined_report['missing_dmn_refs'].extend(report.get('missing_dmn_refs', []))
                    combined_report['missing_dspy_refs'].extend(report.get('missing_dspy_refs', []))
                
                span.add_event("validation_completed", {
                    "valid": combined_report.get('valid', False),
                    "process_count": len(linked_specs),
                    "dmn_count": len(self.dmn_decisions),
                    "dspy_signature_count": len(self.dspy_signatures)
                })
                
                return {
                    "valid": combined_report.get('valid', False),
                    "process_specs": len(linked_specs),
                    "dmn_decisions": len(self.dmn_decisions),
                    "dspy_signatures": len(self.dspy_signatures),
                    "dspy_modules": len(self.dspy_modules),
                    "dspy_config": self.dspy_config is not None,
                    "validation_report": combined_report
                }
                
            except Exception as e:
                span.record_exception(e)
                logger.error(f"Failed to validate files: {e}")
                raise 