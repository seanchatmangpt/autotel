"""
Test telemetry cleanup to prevent provider override warnings.

This test verifies that the global provider management prevents
multiple TracerProvider and MeterProvider instances from being created.
"""

import pytest
import logging
from autotel.core.telemetry import (
    create_telemetry_manager, 
    get_telemetry_manager_or_noop,
    TelemetryManager,
    NoOpTelemetryManager
)
from autotel.factory import PipelineOrchestrator


class TestTelemetryCleanup:
    """Test that telemetry cleanup prevents provider override warnings."""
    
    def test_multiple_telemetry_managers_no_warnings(self, caplog):
        """Test that creating multiple telemetry managers doesn't cause warnings."""
        # Set log level to capture warnings
        caplog.set_level(logging.WARNING)
        
        # Create multiple telemetry managers
        manager1 = create_telemetry_manager("service1")
        manager2 = create_telemetry_manager("service2")
        manager3 = create_telemetry_manager("service3")
        
        # Check that no provider override warnings were logged
        warning_messages = [record.message for record in caplog.records if record.levelno == logging.WARNING]
        provider_warnings = [msg for msg in warning_messages if "Overriding of current" in msg]
        
        assert len(provider_warnings) == 0, f"Found provider override warnings: {provider_warnings}"
        
        # Verify all managers are properly initialized
        assert isinstance(manager1, TelemetryManager)
        assert isinstance(manager2, TelemetryManager)
        assert isinstance(manager3, TelemetryManager)
        
        # Verify they all have tracers
        assert manager1.tracer is not None
        assert manager2.tracer is not None
        assert manager3.tracer is not None
    
    def test_pipeline_orchestrator_telemetry_cleanup(self, caplog):
        """Test that PipelineOrchestrator doesn't cause telemetry warnings."""
        # Set log level to capture warnings
        caplog.set_level(logging.WARNING)
        
        # Create multiple orchestrators
        orchestrator1 = PipelineOrchestrator()
        orchestrator2 = PipelineOrchestrator()
        orchestrator3 = PipelineOrchestrator()
        
        # Check that no provider override warnings were logged
        warning_messages = [record.message for record in caplog.records if record.levelno == logging.WARNING]
        provider_warnings = [msg for msg in warning_messages if "Overriding of current" in msg]
        
        assert len(provider_warnings) == 0, f"Found provider override warnings: {provider_warnings}"
        
        # Verify orchestrators are properly initialized
        assert orchestrator1.telemetry is not None
        assert orchestrator2.telemetry is not None
        assert orchestrator3.telemetry is not None
    
    def test_noop_telemetry_manager_no_warnings(self, caplog):
        """Test that NoOpTelemetryManager doesn't cause warnings."""
        # Set log level to capture warnings
        caplog.set_level(logging.WARNING)
        
        # Create no-op managers
        noop1 = get_telemetry_manager_or_noop("noop1", force_noop=True)
        noop2 = get_telemetry_manager_or_noop("noop2", force_noop=True)
        noop3 = get_telemetry_manager_or_noop("noop3", force_noop=True)
        
        # Check that no provider override warnings were logged
        warning_messages = [record.message for record in caplog.records if record.levelno == logging.WARNING]
        provider_warnings = [msg for msg in warning_messages if "Overriding of current" in msg]
        
        assert len(provider_warnings) == 0, f"Found provider override warnings: {provider_warnings}"
        
        # Verify all are no-op managers
        assert isinstance(noop1, NoOpTelemetryManager)
        assert isinstance(noop2, NoOpTelemetryManager)
        assert isinstance(noop3, NoOpTelemetryManager)
    
    def test_telemetry_manager_functionality(self):
        """Test that telemetry managers still work correctly after cleanup."""
        # Create telemetry manager
        manager = create_telemetry_manager("test-service")
        
        # Test span creation
        with manager.start_span("test_span", "test_operation") as span:
            span.set_attribute("test_key", "test_value")
            assert span is not None
        
        # Test metric recording
        manager.record_metric("test_metric", 1, test_attr="test_value")
        
        # Verify manager is configured
        assert manager.is_configured()
        
        # Get stats
        stats = manager.get_stats()
        assert "schema_connected" in stats
        assert "tracer_enabled" in stats
        assert "meter_enabled" in stats
    
    def test_global_provider_singleton(self):
        """Test that global providers are truly singleton."""
        from autotel.core.telemetry import _get_or_create_tracer_provider, _get_or_create_meter_provider
        
        # Get providers multiple times
        provider1 = _get_or_create_tracer_provider()
        provider2 = _get_or_create_tracer_provider()
        provider3 = _get_or_create_tracer_provider()
        
        meter1 = _get_or_create_meter_provider()
        meter2 = _get_or_create_meter_provider()
        meter3 = _get_or_create_meter_provider()
        
        # Verify they are the same instances
        assert provider1 is provider2
        assert provider2 is provider3
        assert meter1 is meter2
        assert meter2 is meter3
    
    def test_telemetry_manager_with_external_providers(self):
        """Test telemetry manager with external providers."""
        from opentelemetry.sdk.trace import TracerProvider
        from opentelemetry.sdk.trace.export import ConsoleSpanExporter
        
        # Create external providers
        external_tracer_provider = TracerProvider()
        external_span_exporter = ConsoleSpanExporter()
        
        # Create telemetry manager with external providers
        manager = create_telemetry_manager(
            "external-service",
            tracer_provider=external_tracer_provider,
            span_exporter=external_span_exporter
        )
        
        # Verify manager is properly configured
        assert isinstance(manager, TelemetryManager)
        assert manager.tracer is not None
        
        # Test span creation
        with manager.start_span("external_span", "external_operation") as span:
            span.set_attribute("external_key", "external_value")
            assert span is not None


if __name__ == "__main__":
    pytest.main([__file__]) 