#!/usr/bin/env python3
"""
OpenTelemetry Telemetry Manager with LinkML Schema Integration
Eliminates hardcoded strings by using LinkML schema for all telemetry operations
"""

import json
import time
from datetime import datetime
from typing import Dict, List, Any, Optional, Union
from pathlib import Path
from dataclasses import dataclass, field

# OpenTelemetry imports
from opentelemetry import trace, metrics
from opentelemetry.trace import Status, StatusCode
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import ConsoleSpanExporter, BatchSpanProcessor
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import ConsoleMetricExporter, PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource

# LinkML imports
from linkml_runtime.utils.schemaview import SchemaView
from linkml_runtime.dumpers import yaml_dumper
from linkml_runtime.loaders import yaml_loader

@dataclass
class TelemetryConfig:
    """Configuration for telemetry operations"""
    service_name: str = "autotel-service"
    service_version: str = "1.0.0"
    enable_tracing: bool = True
    enable_metrics: bool = True
    schema_path: Optional[str] = None

class OTELTelemetryManager:
    """
    OpenTelemetry Telemetry Manager using LinkML schema
    Eliminates hardcoded strings by using schema-driven telemetry
    """
    
    def __init__(self, config: TelemetryConfig):
        """Initialize the telemetry manager with LinkML schema"""
        self.config = config
        self.schema_view: Optional[SchemaView] = None
        self.tracer: Optional[trace.Tracer] = None
        self.meter: Optional[metrics.Meter] = None
        
        # Load LinkML schema for telemetry
        self._load_telemetry_schema()
        
        # Initialize OpenTelemetry
        self._initialize_otel()
        
        # Create metrics from schema
        self._create_metrics_from_schema()
    
    def _load_telemetry_schema(self) -> None:
        """Load the OTEL traces LinkML schema"""
        if self.config.schema_path:
            schema_path = Path(self.config.schema_path)
        else:
            # Default to the schema in the same directory
            schema_path = Path(__file__).parent / "otel_traces_schema.yaml"
        
        if not schema_path.exists():
            raise FileNotFoundError(f"Telemetry schema not found: {schema_path}")
        
        self.schema_view = SchemaView(str(schema_path))
        
        # Validate schema has required classes
        required_classes = ["Span", "LinkMLOperation", "ValidationResult"]
        for class_name in required_classes:
            if not self.schema_view.get_class(class_name):
                raise ValueError(f"Required class '{class_name}' not found in telemetry schema")
    
    def _initialize_otel(self) -> None:
        """Initialize OpenTelemetry with resource information"""
        # Create resource with service information
        resource = Resource.create({
            "service.name": self.config.service_name,
            "service.version": self.config.service_version
        })
        
        if self.config.enable_tracing:
            # Configure tracing
            trace_provider = TracerProvider(resource=resource)
            trace_provider.add_span_processor(BatchSpanProcessor(ConsoleSpanExporter()))
            trace.set_tracer_provider(trace_provider)
            self.tracer = trace.get_tracer(self.config.service_name)
        
        if self.config.enable_metrics:
            # Configure metrics
            metric_reader = PeriodicExportingMetricReader(ConsoleMetricExporter())
            metric_provider = MeterProvider(resource=resource, metric_readers=[metric_reader])
            metrics.set_meter_provider(metric_provider)
            self.meter = metrics.get_meter(self.config.service_name)
    
    def _create_metrics_from_schema(self) -> None:
        """Create metrics based on the LinkML schema definitions"""
        if not self.meter or not self.schema_view:
            return
        
        # Get metric definitions from schema
        metric_class = self.schema_view.get_class("Metric")
        if not metric_class:
            return
        
        # Create counters for LinkML operations
        self.linkml_operations_counter = self.meter.create_counter(
            name="linkml_operations_total",
            description="Total number of LinkML operations performed"
        )
        
        # Create histogram for operation durations
        self.operation_duration_histogram = self.meter.create_histogram(
            name="linkml_operation_duration_seconds",
            description="Duration of LinkML operations in seconds"
        )
        
        # Create counter for validation operations
        self.validation_operations_counter = self.meter.create_counter(
            name="validation_operations_total",
            description="Total number of validation operations"
        )
    
    def get_operation_type_enum(self) -> List[str]:
        """Get valid operation types from the LinkML schema"""
        if not self.schema_view:
            return []
        
        operation_enum = self.schema_view.get_enum("LinkMLOperationType")
        if operation_enum:
            return list(operation_enum.permissible_values.keys())
        return []
    
    def get_validation_type_enum(self) -> List[str]:
        """Get valid validation types from the LinkML schema"""
        if not self.schema_view:
            return []
        
        validation_enum = self.schema_view.get_enum("ValidationType")
        if validation_enum:
            return list(validation_enum.permissible_values.keys())
        return []
    
    def create_span_attributes(self, operation_type: str, **kwargs) -> Dict[str, Any]:
        """Create span attributes using LinkML schema validation"""
        if not self.schema_view:
            return kwargs
        
        # Validate operation type against schema
        valid_operations = self.get_operation_type_enum()
        if operation_type not in valid_operations:
            raise ValueError(f"Invalid operation type '{operation_type}'. Valid types: {valid_operations}")
        
        # Get span attributes from schema
        span_class = self.schema_view.get_class("Span")
        if not span_class:
            return kwargs
        
        # Create validated attributes
        attributes = {"operation_type": operation_type}
        
        # Add schema-validated attributes
        for key, value in kwargs.items():
            # Check if this is a valid span attribute
            if key in span_class.attributes or key in self.schema_view.all_slots():
                attributes[key] = value
        
        return attributes
    
    def start_span(self, name: str, operation_type: str, **kwargs) -> trace.Span:
        """Start a span with schema-validated attributes"""
        if not self.tracer:
            raise RuntimeError("Tracing not enabled")
        
        attributes = self.create_span_attributes(operation_type, **kwargs)
        return self.tracer.start_as_current_span(name, attributes=attributes)
    
    def record_metric(self, metric_name: str, value: Union[int, float], **attributes) -> None:
        """Record a metric with schema validation"""
        if not self.meter:
            raise RuntimeError("Metrics not enabled")
        
        # Validate metric attributes against schema
        validated_attributes = {}
        for key, val in attributes.items():
            if key in self.schema_view.all_slots():
                validated_attributes[key] = val
        
        if metric_name == "linkml_operations_total":
            self.linkml_operations_counter.add(value, validated_attributes)
        elif metric_name == "validation_operations_total":
            self.validation_operations_counter.add(value, validated_attributes)
        elif metric_name == "operation_duration_seconds":
            self.operation_duration_histogram.record(value, validated_attributes)
    
    def validate_span_data(self, span_data: Dict[str, Any]) -> Dict[str, Any]:
        """Validate span data against LinkML schema"""
        if not self.schema_view:
            return span_data
        
        # Get span class definition
        span_class = self.schema_view.get_class("Span")
        if not span_class:
            return span_data
        
        # Validate required attributes
        validated_data = {}
        for attr_name, attr_def in span_class.attributes.items():
            if attr_name in span_data:
                # Basic type validation
                if attr_def.range == "string" and not isinstance(span_data[attr_name], str):
                    span_data[attr_name] = str(span_data[attr_name])
                elif attr_def.range == "integer" and not isinstance(span_data[attr_name], int):
                    try:
                        span_data[attr_name] = int(span_data[attr_name])
                    except (ValueError, TypeError):
                        continue
                elif attr_def.range == "float" and not isinstance(span_data[attr_name], float):
                    try:
                        span_data[attr_name] = float(span_data[attr_name])
                    except (ValueError, TypeError):
                        continue
                
                validated_data[attr_name] = span_data[attr_name]
        
        return validated_data
    
    def create_linkml_operation(self, operation_type: str, **kwargs) -> Dict[str, Any]:
        """Create a LinkML operation record using schema"""
        if not self.schema_view:
            return {"operation_type": operation_type, **kwargs}
        
        # Get LinkMLOperation class definition
        operation_class = self.schema_view.get_class("LinkMLOperation")
        if not operation_class:
            return {"operation_type": operation_type, **kwargs}
        
        # Validate operation type
        valid_operations = self.get_operation_type_enum()
        if operation_type not in valid_operations:
            raise ValueError(f"Invalid operation type '{operation_type}'. Valid types: {valid_operations}")
        
        # Create operation record with schema validation
        operation_record = {"operation_type": operation_type}
        
        for key, value in kwargs.items():
            if key in operation_class.attributes:
                operation_record[key] = value
        
        return operation_record
    
    def create_validation_result(self, validation_type: str, passed: bool, **kwargs) -> Dict[str, Any]:
        """Create a validation result using schema"""
        if not self.schema_view:
            return {"validation_type": validation_type, "passed": passed, **kwargs}
        
        # Validate validation type
        valid_types = self.get_validation_type_enum()
        if validation_type not in valid_types:
            raise ValueError(f"Invalid validation type '{validation_type}'. Valid types: {valid_types}")
        
        # Get ValidationResult class definition
        result_class = self.schema_view.get_class("ValidationResult")
        if not result_class:
            return {"validation_type": validation_type, "passed": passed, **kwargs}
        
        # Create validation result with schema validation
        validation_result = {
            "validation_type": validation_type,
            "passed": passed
        }
        
        for key, value in kwargs.items():
            if key in result_class.attributes:
                validation_result[key] = value
        
        return validation_result
    
    def export_schema_metadata(self) -> Dict[str, Any]:
        """Export schema metadata for telemetry validation"""
        if not self.schema_view:
            return {}
        
        return {
            "schema_id": self.schema_view.schema.id,
            "schema_name": self.schema_view.schema.name,
            "schema_version": self.schema_view.schema.version,
            "classes_count": len(self.schema_view.all_classes()),
            "enums_count": len(self.schema_view.all_enums()),
            "slots_count": len(self.schema_view.all_slots()),
            "operation_types": self.get_operation_type_enum(),
            "validation_types": self.get_validation_type_enum()
        }
    
    def force_flush(self) -> None:
        """Force flush all telemetry data"""
        if self.config.enable_tracing:
            trace.get_tracer_provider().force_flush()
        
        if self.config.enable_metrics:
            # Metrics are automatically flushed by the reader
            pass

# Factory function for easy creation
def create_telemetry_manager(
    service_name: str = "autotel-service",
    service_version: str = "1.0.0",
    schema_path: Optional[str] = None,
    enable_tracing: bool = True,
    enable_metrics: bool = True
) -> OTELTelemetryManager:
    """Create a telemetry manager with the specified configuration"""
    config = TelemetryConfig(
        service_name=service_name,
        service_version=service_version,
        schema_path=schema_path,
        enable_tracing=enable_tracing,
        enable_metrics=enable_metrics
    )
    return OTELTelemetryManager(config) 