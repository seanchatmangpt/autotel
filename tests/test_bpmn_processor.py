"""
Test suite for BPMN Processor

Tests the unified BPMN processor with telemetry validation and factory boy integration.
"""

import pytest
import tempfile
import os
from pathlib import Path
from unittest.mock import patch, MagicMock

from autotel.processors.bpmn_processor import BPMNProcessor
from autotel.processors.base import ProcessorConfig, ProcessorResult


class TestBPMNProcessor:
    """Test suite for BPMNProcessor class"""
    
    @pytest.fixture
    def processor_config(self):
        """Create a processor configuration for testing"""
        return ProcessorConfig(
            name="bpmn_processor",
            enabled=True,
            timeout=30,
            settings={"validate_schema": True}
        )
    
    @pytest.fixture
    def bpmn_processor(self, processor_config):
        """Create a BPMN processor instance for testing"""
        return BPMNProcessor(processor_config)
    
    @pytest.fixture
    def sample_bpmn_xml(self):
        """Simple, valid BPMN XML for testing"""
        return '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="test_process" name="Test Process" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:task id="Task_1" name="Test Task">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:task>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="EndEvent_1" />
  </bpmn:process>
</bpmn:definitions>'''
    
    def test_processor_initialization(self, processor_config):
        """Test BPMN processor initialization"""
        processor = BPMNProcessor(processor_config)
        
        assert processor.get_name() == "bpmn_processor"
        assert processor.is_enabled() is True
        assert processor.get_setting("validate_schema") is True
        assert processor.get_config() == processor_config
    
    def test_processor_metadata(self, bpmn_processor):
        """Test processor metadata and capabilities"""
        metadata = bpmn_processor.get_metadata()
        
        assert metadata['name'] == "bpmn_processor"
        assert metadata['version'] == "2.1.0"
        assert "parsing" in metadata['capabilities']
        assert "validation" in metadata['capabilities']
        assert "workflow_specification" in metadata['capabilities']
        assert "bpmn" in metadata['supported_formats']
        assert "xml" in metadata['supported_formats']
    
    def test_processor_capabilities(self, bpmn_processor):
        """Test processor capabilities"""
        capabilities = bpmn_processor.get_capabilities()
        supported_formats = bpmn_processor.get_supported_formats()
        
        assert "parsing" in capabilities
        assert "validation" in capabilities
        assert "workflow_specification" in capabilities
        assert "file_processing" in capabilities
        
        assert "bpmn" in supported_formats
        assert "xml" in supported_formats
        assert "bpmn20" in supported_formats
    
    def test_parse_xml_success(self, bpmn_processor, sample_bpmn_xml):
        """Test successful XML parsing"""
        data = {
            'xml': sample_bpmn_xml,
            'process_id': 'test_process'
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is True
        assert result.data is not None
        assert result.metadata['process_id'] == 'test_process'
        assert result.metadata['parser_type'] == "CamundaParser"
        assert 'duration_ms' in result.metadata
    
    def test_parse_file_success(self, bpmn_processor, sample_bpmn_xml):
        """Test successful file parsing"""
        # Create temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
            f.write(sample_bpmn_xml)
            temp_file = f.name
        
        try:
            data = {
                'file_path': temp_file,
                'process_id': 'test_process'
            }
            
            result = bpmn_processor.process(data)
            
            assert result.success is True
            assert result.data is not None
            assert result.metadata['process_id'] == 'test_process'
            assert result.metadata['parser_type'] == "CamundaParser"
            assert 'duration_ms' in result.metadata
            
        finally:
            # Clean up temporary file
            os.unlink(temp_file)
    
    def test_parse_xml_convenience_method(self, bpmn_processor, sample_bpmn_xml):
        """Test parse_xml convenience method"""
        workflow_spec = bpmn_processor.parse_xml(sample_bpmn_xml, 'test_process')
        
        assert workflow_spec is not None
        assert hasattr(workflow_spec, 'name')
    
    def test_parse_file_convenience_method(self, bpmn_processor, sample_bpmn_xml):
        """Test parse_file convenience method"""
        # Create temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
            f.write(sample_bpmn_xml)
            temp_file = f.name
        
        try:
            workflow_spec = bpmn_processor.parse_file(temp_file, 'test_process')
            
            assert workflow_spec is not None
            assert hasattr(workflow_spec, 'name')
            
        finally:
            # Clean up temporary file
            os.unlink(temp_file)
    
    def test_invalid_xml_format(self, bpmn_processor):
        """Test handling of invalid XML format"""
        invalid_xml = "<invalid>xml</invalid>"
        process_id = "test_process"
        
        data = {
            'xml': invalid_xml,
            'process_id': process_id
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is False
        assert "Failed to parse BPMN XML" in result.error
        assert result.metadata['error_type'] == "ValueError"
        assert 'duration_ms' in result.metadata
    
    def test_missing_process_id(self, bpmn_processor, sample_bpmn_xml):
        """Test handling of missing process ID"""
        data = {
            'xml': sample_bpmn_xml
            # Missing process_id
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is False
        assert "process_id is required" in result.error
        assert result.metadata['error_type'] == "ValueError"
    
    def test_nonexistent_process_id(self, bpmn_processor, sample_bpmn_xml):
        """Test handling of non-existent process ID"""
        nonexistent_process_id = "nonexistent_process"
        
        data = {
            'xml': sample_bpmn_xml,
            'process_id': nonexistent_process_id
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is False
        assert f"Process ID '{nonexistent_process_id}' not found" in result.error
        assert result.metadata['error_type'] == "ValueError"
    
    def test_file_not_found(self, bpmn_processor):
        """Test handling of non-existent file"""
        nonexistent_file = "/path/to/nonexistent/file.bpmn"
        process_id = "test_process"
        
        data = {
            'file_path': nonexistent_file,
            'process_id': process_id
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is False
        assert "BPMN file not found" in result.error
        assert result.metadata['error_type'] == "FileNotFoundError"
    
    def test_invalid_input_format(self, bpmn_processor):
        """Test handling of invalid input format"""
        invalid_data = 123  # Invalid type
        
        result = bpmn_processor.process(invalid_data)
        
        assert result.success is False
        assert "Unsupported input format" in result.error
        assert result.metadata['error_type'] == "ValueError"
    
    def test_string_input_without_process_id(self, bpmn_processor, sample_bpmn_xml):
        """Test handling of string input without process ID"""
        result = bpmn_processor.process(sample_bpmn_xml)
        
        assert result.success is False
        assert "String input requires process_id parameter" in result.error
        assert result.metadata['error_type'] == "ValueError"
    
    def test_file_path_without_process_id(self, bpmn_processor, sample_bpmn_xml):
        """Test handling of file path without process ID"""
        # Create temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.bpmn', delete=False) as f:
            f.write(sample_bpmn_xml)
            temp_file = f.name
        
        try:
            result = bpmn_processor.process(temp_file)
            
            assert result.success is False
            assert "File path input requires process_id parameter" in result.error
            assert result.metadata['error_type'] == "ValueError"
            
        finally:
            # Clean up temporary file
            os.unlink(temp_file)
    
    def test_parser_info(self, bpmn_processor):
        """Test get_parser_info method"""
        parser_info = bpmn_processor.get_parser_info()
        
        assert parser_info['parser_type'] == "CamundaParser"
        assert parser_info['parser_class'] == "CamundaParser"
        assert "parsing" in parser_info['capabilities']
        assert "bpmn" in parser_info['supported_formats']
    
    @patch('autotel.helpers.telemetry.span.create_processor_span')
    def test_telemetry_integration(self, mock_create_span, bpmn_processor, sample_bpmn_xml):
        """Test telemetry span creation and attributes"""
        # Mock span
        mock_span = MagicMock()
        mock_create_span.return_value.__enter__.return_value = mock_span
        
        data = {
            'xml': sample_bpmn_xml,
            'process_id': 'test_process'
        }
        
        result = bpmn_processor.process(data)
        
        # Verify span was created
        mock_create_span.assert_called_once_with(
            "process", 
            "bpmn"
        )
        
        # Verify span attributes were set
        assert mock_span.set_attribute.called
        
        # Get the call arguments - we need to check individual calls
        set_attribute_calls = mock_span.set_attribute.call_args_list
        
        if result.success:
            # Check that the expected attributes were set
            attribute_names = [call[0][0] for call in set_attribute_calls]
            assert "bpmn.process_id" in attribute_names
            assert "bpmn.parsing_success" in attribute_names
            assert "bpmn.duration_ms" in attribute_names
        else:
            # Check that the expected attributes were set
            attribute_names = [call[0][0] for call in set_attribute_calls]
            assert "bpmn.parsing_success" in attribute_names
            assert "bpmn.error" in attribute_names
            assert "bpmn.duration_ms" in attribute_names
    
    def test_multiple_processes_in_xml(self, bpmn_processor):
        """Test parsing XML with multiple processes"""
        # Create XML with multiple processes
        multi_process_xml = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="process1" name="Process 1" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_1</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="EndEvent_1" />
  </bpmn:process>
  <bpmn:process id="process2" name="Process 2" isExecutable="true">
    <bpmn:startEvent id="StartEvent_2" name="Start">
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:endEvent id="EndEvent_2" name="End">
      <bpmn:incoming>Flow_2</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_2" sourceRef="StartEvent_2" targetRef="EndEvent_2" />
  </bpmn:process>
</bpmn:definitions>'''
        
        # Test parsing first process
        data1 = {
            'xml': multi_process_xml,
            'process_id': 'process1'
        }
        
        result1 = bpmn_processor.process(data1)
        assert result1.success is True
        # SpiffWorkflow uses 'id' attribute, not 'name'
        assert result1.data.name == "process1"
        
        # Test parsing second process
        data2 = {
            'xml': multi_process_xml,
            'process_id': 'process2'
        }
        
        result2 = bpmn_processor.process(data2)
        assert result2.success is True
        # SpiffWorkflow uses 'id' attribute, not 'name'
        assert result2.data.name == "process2"
    
    def test_complex_bpmn_with_tasks(self, bpmn_processor):
        """Test parsing complex BPMN with multiple tasks"""
        complex_bpmn = '''<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
                  xmlns:bpmndi="http://www.omg.org/spec/BPMN/20100524/DI"
                  xmlns:dc="http://www.omg.org/spec/DD/20100524/DC"
                  xmlns:di="http://www.omg.org/spec/DD/20100524/DI"
                  id="Definitions_1"
                  targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn:process id="complex_process" name="Complex Process" isExecutable="true">
    <bpmn:startEvent id="StartEvent_1" name="Start">
      <bpmn:outgoing>Flow_1</bpmn:outgoing>
    </bpmn:startEvent>
    <bpmn:task id="Task_1" name="First Task">
      <bpmn:incoming>Flow_1</bpmn:incoming>
      <bpmn:outgoing>Flow_2</bpmn:outgoing>
    </bpmn:task>
    <bpmn:task id="Task_2" name="Second Task">
      <bpmn:incoming>Flow_2</bpmn:incoming>
      <bpmn:outgoing>Flow_3</bpmn:outgoing>
    </bpmn:task>
    <bpmn:endEvent id="EndEvent_1" name="End">
      <bpmn:incoming>Flow_3</bpmn:incoming>
    </bpmn:endEvent>
    <bpmn:sequenceFlow id="Flow_1" sourceRef="StartEvent_1" targetRef="Task_1" />
    <bpmn:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="Task_2" />
    <bpmn:sequenceFlow id="Flow_3" sourceRef="Task_2" targetRef="EndEvent_1" />
  </bpmn:process>
</bpmn:definitions>'''
        
        data = {
            'xml': complex_bpmn,
            'process_id': 'complex_process'
        }
        
        result = bpmn_processor.process(data)
        
        assert result.success is True
        # SpiffWorkflow uses 'id' attribute, not 'name'
        assert result.data.name == "complex_process"
        
        # Verify the workflow spec contains the expected tasks
        workflow_spec = result.data
        task_specs = [spec for spec in workflow_spec.task_specs.values() if hasattr(spec, 'name')]
        
        # Should have 2 tasks (excluding start/end events)
        assert len([t for t in task_specs if 'Task' in t.name]) == 2
    
    def test_processor_registration(self, processor_config):
        """Test that processor is automatically registered"""
        from autotel.processors.registry import registry
        
        # Create processor (should auto-register)
        processor = BPMNProcessor(processor_config)
        
        # Check if registered
        registered_processors = registry.list_registered()
        assert "bpmn_processor" in registered_processors
        
        # Verify we can get the processor class
        processor_class = registry.get_class("bpmn_processor")
        assert processor_class == BPMNProcessor 