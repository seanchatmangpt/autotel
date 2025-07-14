"""
Test Semantic Linker - AUTOTEL-105

Tests the semantic linker that receives parsed, isolated objects and links them together
by injecting executable capabilities into BPMN task specifications.
"""

import pytest
from unittest.mock import Mock, MagicMock
from autotel.factory.linker import SemanticLinker, SemanticLinkerError
from autotel.factory.processors.dspy_processor import DSPySignatureDefinition
from SpiffWorkflow.specs import WorkflowSpec


class TestSemanticLinker:
    """Test suite for SemanticLinker class."""
    
    def setup_method(self):
        """Set up test fixtures."""
        self.linker = SemanticLinker()
        
        # Create mock BPMN spec with tasks
        self.bpmn_spec = Mock(spec=WorkflowSpec)
        
        # Create mock tasks with proper attributes
        self.mock_tasks = [
            self._create_mock_task("BusinessRuleTask_1", decisionRef="simple_decision"),
            self._create_mock_task("ServiceTask_1", dspy_service="customer_analysis"),
            self._create_mock_task("ServiceTask_2", dspy_service="fraud_detection"),
            self._create_mock_task("StartEvent_1"),  # No extensions
            self._create_mock_task("EndEvent_1")     # No extensions
        ]
        
        # Set up the get_tasks method to return our mock tasks
        self.bpmn_spec.get_tasks.return_value = self.mock_tasks
        
        # Create mock DMN decisions
        self.dmn_decisions = {
            "simple_decision": Mock(),
            "another_decision": Mock()
        }
        
        # Create mock DSPy signatures
        self.dspy_signatures = [
            DSPySignatureDefinition(
                name="customer_analysis",
                description="Analyze customer data",
                inputs={"customer_data": {"description": "Customer info", "optional": "false"}},
                outputs={"analysis_result": "Analysis results"}
            ),
            DSPySignatureDefinition(
                name="fraud_detection", 
                description="Detect fraud",
                inputs={"transaction_data": {"description": "Transaction info", "optional": "false"}},
                outputs={"fraud_score": "Fraud score"}
            )
        ]
    
    def _create_mock_task(self, name, decisionRef=None, dspy_service=None):
        """Create a mock task with specified extensions."""
        task = Mock()
        task.name = name
        
        if decisionRef:
            task.decisionRef = decisionRef
        
        if dspy_service:
            task.extensions = {
                'dspy_service': {
                    'signature': dspy_service,
                    'service': dspy_service,
                    'params': {'param1': 'value1'}
                }
            }
        else:
            task.extensions = {}
        
        return task
    
    def test_linker_success_path(self):
        """Test successful linking of all components."""
        result = self.linker.link(
            bpmn_spec=self.bpmn_spec,
            dmn_decisions=self.dmn_decisions,
            dspy_signatures=self.dspy_signatures
        )
        
        # Verify BPMN spec is returned
        assert result == self.bpmn_spec
        
        # Verify DMN decisions were injected
        business_rule_task = self.bpmn_spec.get_tasks.return_value[0]
        assert hasattr(business_rule_task, 'executable_dmn')
        assert business_rule_task.executable_dmn == self.dmn_decisions["simple_decision"]
        
        # Verify DSPy signatures were injected
        service_task1 = self.bpmn_spec.get_tasks.return_value[1]
        assert hasattr(service_task1, 'executable_dspy_signature')
        assert service_task1.executable_dspy_signature.name == "customer_analysis"
        
        service_task2 = self.bpmn_spec.get_tasks.return_value[2]
        assert hasattr(service_task2, 'executable_dspy_signature')
        assert service_task2.executable_dspy_signature.name == "fraud_detection"
        
        # Verify tasks without extensions are not modified
        start_task = self.bpmn_spec.get_tasks.return_value[3]
        assert not hasattr(start_task, 'executable_dmn')
        assert not hasattr(start_task, 'executable_dspy_signature')
    
    def test_linker_semantic_attack_missing_dmn(self):
        """Test semantic attack with missing DMN decision reference."""
        # Create task with nonexistent DMN reference
        task_with_missing_dmn = self._create_mock_task("BusinessRuleTask_2", decisionRef="nonexistent_decision")
        self.bpmn_spec.get_tasks.return_value.append(task_with_missing_dmn)
        
        with pytest.raises(SemanticLinkerError, match="Referenced DMN decision 'nonexistent_decision' not found"):
            self.linker.link(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )
    
    def test_linker_semantic_attack_missing_dspy(self):
        """Test semantic attack with missing DSPy signature reference."""
        # Create task with nonexistent DSPy reference
        task_with_missing_dspy = self._create_mock_task("ServiceTask_3", dspy_service="nonexistent_signature")
        self.bpmn_spec.get_tasks.return_value.append(task_with_missing_dspy)
        
        with pytest.raises(SemanticLinkerError, match="Referenced DSPy signature 'nonexistent_signature' not found"):
            self.linker.link(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )
    
    def test_linker_semantic_attack_malicious_dmn_ref(self):
        """Test semantic attack with malicious DMN decision reference."""
        # Create task with malicious DMN reference
        malicious_task = self._create_mock_task("BusinessRuleTask_3", decisionRef="../../../etc/passwd")
        self.bpmn_spec.get_tasks.return_value.append(malicious_task)
        
        with pytest.raises(SemanticLinkerError, match="Referenced DMN decision '../../../etc/passwd' not found"):
            self.linker.link(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )
    
    def test_linker_semantic_attack_malicious_dspy_ref(self):
        """Test semantic attack with malicious DSPy signature reference."""
        # Create task with malicious DSPy reference
        malicious_task = self._create_mock_task("ServiceTask_4", dspy_service="<script>alert('xss')</script>")
        self.bpmn_spec.get_tasks.return_value.append(malicious_task)
        
        with pytest.raises(SemanticLinkerError, match="Referenced DSPy signature '<script>alert\\('xss'\\)</script>' not found"):
            self.linker.link(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )
    
    def test_validate_linkage_success(self):
        """Test successful validation of linkage."""
        validation_report = self.linker.validate_linkage(
            bpmn_spec=self.bpmn_spec,
            dmn_decisions=self.dmn_decisions,
            dspy_signatures=self.dspy_signatures
        )
        
        assert validation_report['valid'] == True
        assert validation_report['missing_dmn_refs'] == []
        assert validation_report['missing_dspy_refs'] == []
        assert validation_report['total_tasks'] == 5
        assert validation_report['available_dmn_decisions'] == ["simple_decision", "another_decision"]
        assert validation_report['available_dspy_signatures'] == ["customer_analysis", "fraud_detection"]
    
    def test_validate_linkage_missing_refs(self):
        """Test validation with missing references."""
        # Add task with missing references
        task_with_missing_refs = self._create_mock_task("ProblemTask", decisionRef="missing_dmn", dspy_service="missing_dspy")
        self.bpmn_spec.get_tasks.return_value.append(task_with_missing_refs)
        
        with pytest.raises(SemanticLinkerError, match="Validation failed: Missing DMN refs: \\['missing_dmn'\\], Missing DSPy refs: \\['missing_dspy'\\]"):
            self.linker.validate_linkage(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )
    
    def test_linker_handles_empty_signatures(self):
        """Test linker handles empty DSPy signatures list."""
        result = self.linker.link(
            bpmn_spec=self.bpmn_spec,
            dmn_decisions=self.dmn_decisions,
            dspy_signatures=[]
        )
        
        assert result == self.bpmn_spec
        
        # Verify DMN decisions still work
        business_rule_task = self.bpmn_spec.get_tasks.return_value[0]
        assert hasattr(business_rule_task, 'executable_dmn')
        
        # Verify DSPy tasks don't get signatures injected
        service_task1 = self.bpmn_spec.get_tasks.return_value[1]
        assert not hasattr(service_task1, 'executable_dspy_signature')
    
    def test_linker_handles_empty_decisions(self):
        """Test linker handles empty DMN decisions dict."""
        result = self.linker.link(
            bpmn_spec=self.bpmn_spec,
            dmn_decisions={},
            dspy_signatures=self.dspy_signatures
        )
        
        assert result == self.bpmn_spec
        
        # Verify DSPy signatures still work
        service_task1 = self.bpmn_spec.get_tasks.return_value[1]
        assert hasattr(service_task1, 'executable_dspy_signature')
        
        # Verify DMN tasks don't get decisions injected
        business_rule_task = self.bpmn_spec.get_tasks.return_value[0]
        assert not hasattr(business_rule_task, 'executable_dmn')
    
    def test_linker_error_handling(self):
        """Test linker properly handles and re-raises errors."""
        # Mock an exception during linking
        self.bpmn_spec.get_tasks.side_effect = Exception("Test exception")
        
        with pytest.raises(SemanticLinkerError, match="Failed to link semantic objects: Test exception"):
            self.linker.link(
                bpmn_spec=self.bpmn_spec,
                dmn_decisions=self.dmn_decisions,
                dspy_signatures=self.dspy_signatures
            )


if __name__ == "__main__":
    pytest.main([__file__]) 