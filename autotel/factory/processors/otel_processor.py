"""OpenTelemetry processor for AutoTel semantic execution pipeline."""

import json
import time
import logging
from datetime import datetime
from typing import Dict, List, Any, Optional, Union
from dataclasses import dataclass, field
from pathlib import Path

from ...core.telemetry import create_telemetry_manager
from ...schemas.executor_types import TelemetryData
from opentelemetry import trace
from opentelemetry.trace import Status, StatusCode


@dataclass
class OTELSpan:
    """OpenTelemetry span data structure."""
    name: str
    trace_id: str
    span_id: str
    parent_id: Optional[str]
    start_time: datetime
    end_time: datetime
    status: str
    attributes: Dict[str, Any]
    events: List[Dict[str, Any]]
    links: List[Dict[str, Any]]
    kind: str
    resource: Dict[str, Any]


@dataclass
class OTELMetric:
    """OpenTelemetry metric data structure."""
    name: str
    value: Union[int, float]
    unit: str
    description: str
    attributes: Dict[str, Any]
    timestamp: datetime


@dataclass
class OTELLog:
    """OpenTelemetry log data structure."""
    timestamp: datetime
    severity: str
    message: str
    attributes: Dict[str, Any]
    resource: Dict[str, Any]
    trace_id: Optional[str]
    span_id: Optional[str]


@dataclass
class OTELTrace:
    """Complete OpenTelemetry trace data."""
    trace_id: str
    spans: List[OTELSpan]
    metrics: List[OTELMetric]
    logs: List[OTELLog]
    metadata: Dict[str, Any]


class OTELProcessor:
    """Processes OpenTelemetry data for AutoTel semantic execution pipeline."""

    def __init__(self, telemetry_manager=None):
        """Initialize OTEL processor with telemetry integration."""
        self.telemetry = telemetry_manager or create_telemetry_manager(
            service_name="autotel-otel-processor",
            require_linkml_validation=False
        )
        self.logger = logging.getLogger(__name__)

    def parse_spans(self, spans_data: List[Dict[str, Any]]) -> List[OTELSpan]:
        """Parse OpenTelemetry spans from JSON data."""
        with self.telemetry.start_span("otel_parse_spans", "telemetry_processing") as span:
            try:
                spans = []
                span.set_attribute("input_spans_count", len(spans_data))
                
                for span_data in spans_data:
                    otel_span = self._parse_single_span(span_data)
                    if otel_span:
                        spans.append(otel_span)
                
                span.set_attribute("parsed_spans_count", len(spans))
                span.set_attribute("parse_success", True)
                
                # Record metric
                self.telemetry.record_metric("otel_spans_parsed", len(spans))
                
                return spans
                
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_spans_parse_failure", 1)
                raise

    def parse_metrics(self, metrics_data: List[Dict[str, Any]]) -> List[OTELMetric]:
        """Parse OpenTelemetry metrics from JSON data."""
        with self.telemetry.start_span("otel_parse_metrics", "telemetry_processing") as span:
            try:
                metrics = []
                span.set_attribute("input_metrics_count", len(metrics_data))
                
                for metric_data in metrics_data:
                    otel_metric = self._parse_single_metric(metric_data)
                    if otel_metric:
                        metrics.append(otel_metric)
                
                span.set_attribute("parsed_metrics_count", len(metrics))
                span.set_attribute("parse_success", True)
                
                # Record metric
                self.telemetry.record_metric("otel_metrics_parsed", len(metrics))
                
                return metrics
                
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_metrics_parse_failure", 1)
                raise

    def parse_logs(self, logs_data: List[Dict[str, Any]]) -> List[OTELLog]:
        """Parse OpenTelemetry logs from JSON data."""
        with self.telemetry.start_span("otel_parse_logs", "telemetry_processing") as span:
            try:
                logs = []
                span.set_attribute("input_logs_count", len(logs_data))
                
                for log_data in logs_data:
                    otel_log = self._parse_single_log(log_data)
                    if otel_log:
                        logs.append(otel_log)
                
                span.set_attribute("parsed_logs_count", len(logs))
                span.set_attribute("parse_success", True)
                
                # Record metric
                self.telemetry.record_metric("otel_logs_parsed", len(logs))
                
                return logs
                
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_logs_parse_failure", 1)
                raise

    def parse_trace(self, trace_data: Dict[str, Any]) -> OTELTrace:
        """Parse complete OpenTelemetry trace data."""
        with self.telemetry.start_span("otel_parse_trace", "telemetry_processing") as span:
            try:
                trace_id = trace_data.get("trace_id", "unknown")
                span.set_attribute("trace_id", trace_id)
                
                # Parse spans
                spans = self.parse_spans(trace_data.get("spans", []))
                
                # Parse metrics
                metrics = self.parse_metrics(trace_data.get("metrics", []))
                
                # Parse logs
                logs = self.parse_logs(trace_data.get("logs", []))
                
                # Create trace object
                otel_trace = OTELTrace(
                    trace_id=trace_id,
                    spans=spans,
                    metrics=metrics,
                    logs=logs,
                    metadata=trace_data.get("metadata", {})
                )
                
                span.set_attribute("parse_success", True)
                span.set_attribute("spans_count", len(spans))
                span.set_attribute("metrics_count", len(metrics))
                span.set_attribute("logs_count", len(logs))
                
                # Record metric
                self.telemetry.record_metric("otel_traces_parsed", 1)
                
                return otel_trace
                
            except Exception as e:
                span.set_attribute("parse_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_traces_parse_failure", 1)
                raise

    def parse_file(self, file_path: str) -> OTELTrace:
        """Parse OpenTelemetry data from file."""
        with self.telemetry.start_span("otel_parse_file", "telemetry_processing") as span:
            try:
                span.set_attribute("file_path", file_path)
                
                with open(file_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                
                span.set_attribute("file_size_bytes", len(json.dumps(data)))
                
                return self.parse_trace(data)
                
            except FileNotFoundError:
                span.set_attribute("error_type", "FileNotFoundError")
                span.set_attribute("error_message", f"File not found: {file_path}")
                span.set_status(trace.Status(trace.StatusCode.ERROR, f"File not found: {file_path}"))
                
                # Record metric
                self.telemetry.record_metric("otel_file_not_found", 1)
                raise
                
            except Exception as e:
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record metric
                self.telemetry.record_metric("otel_file_parse_failure", 1)
                raise

    def analyze_trace(self, trace: OTELTrace) -> Dict[str, Any]:
        """Analyze OpenTelemetry trace for insights."""
        with self.telemetry.start_span("otel_analyze_trace", "telemetry_analysis") as span:
            try:
                analysis = {
                    "trace_id": trace.trace_id,
                    "span_analysis": self._analyze_spans(trace.spans),
                    "metric_analysis": self._analyze_metrics(trace.metrics),
                    "log_analysis": self._analyze_logs(trace.logs),
                    "performance_analysis": self._analyze_performance(trace),
                    "error_analysis": self._analyze_errors(trace),
                    "dependency_analysis": self._analyze_dependencies(trace)
                }
                
                span.set_attribute("analysis_success", True)
                span.set_attribute("spans_analyzed", len(trace.spans))
                span.set_attribute("metrics_analyzed", len(trace.metrics))
                span.set_attribute("logs_analyzed", len(trace.logs))
                
                # Record metric
                self.telemetry.record_metric("otel_traces_analyzed", 1)
                
                return analysis
                
            except Exception as e:
                span.set_attribute("analysis_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_trace_analysis_failure", 1)
                raise

    def convert_to_autotel_telemetry(self, trace: OTELTrace) -> TelemetryData:
        """Convert OTEL trace to AutoTel TelemetryData format."""
        with self.telemetry.start_span("otel_convert_telemetry", "telemetry_conversion") as span:
            try:
                # Convert spans
                autotel_spans = []
                for otel_span in trace.spans:
                    autotel_span = {
                        "name": otel_span.name,
                        "trace_id": otel_span.trace_id,
                        "span_id": otel_span.span_id,
                        "parent_id": otel_span.parent_id,
                        "start_time": otel_span.start_time.isoformat(),
                        "end_time": otel_span.end_time.isoformat(),
                        "status": otel_span.status,
                        "attributes": otel_span.attributes,
                        "events": otel_span.events,
                        "kind": otel_span.kind,
                        "resource": otel_span.resource
                    }
                    autotel_spans.append(autotel_span)
                
                # Convert metrics
                autotel_metrics = {}
                for otel_metric in trace.metrics:
                    metric_key = f"{otel_metric.name}_{otel_metric.unit}"
                    autotel_metrics[metric_key] = {
                        "value": otel_metric.value,
                        "unit": otel_metric.unit,
                        "description": otel_metric.description,
                        "attributes": otel_metric.attributes,
                        "timestamp": otel_metric.timestamp.isoformat()
                    }
                
                # Convert logs
                autotel_logs = []
                for otel_log in trace.logs:
                    autotel_log = {
                        "timestamp": otel_log.timestamp.isoformat(),
                        "severity": otel_log.severity,
                        "message": otel_log.message,
                        "attributes": otel_log.attributes,
                        "resource": otel_log.resource,
                        "trace_id": otel_log.trace_id,
                        "span_id": otel_log.span_id
                    }
                    autotel_logs.append(autotel_log)
                
                # Create events (extract from spans)
                autotel_events = []
                for otel_span in trace.spans:
                    for event in otel_span.events:
                        autotel_event = {
                            "name": event.get("name", "unknown"),
                            "timestamp": event.get("timestamp", ""),
                            "attributes": event.get("attributes", {}),
                            "span_id": otel_span.span_id,
                            "trace_id": otel_span.trace_id
                        }
                        autotel_events.append(autotel_event)
                
                telemetry_data = TelemetryData(
                    spans=autotel_spans,
                    events=autotel_events,
                    metrics=autotel_metrics,
                    logs=autotel_logs
                )
                
                span.set_attribute("conversion_success", True)
                span.set_attribute("spans_converted", len(autotel_spans))
                span.set_attribute("metrics_converted", len(autotel_metrics))
                span.set_attribute("logs_converted", len(autotel_logs))
                span.set_attribute("events_converted", len(autotel_events))
                
                # Record metric
                self.telemetry.record_metric("otel_telemetry_conversions", 1)
                
                return telemetry_data
                
            except Exception as e:
                span.set_attribute("conversion_success", False)
                span.set_attribute("error_type", type(e).__name__)
                span.set_attribute("error_message", str(e))
                span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
                
                # Record failure metric
                self.telemetry.record_metric("otel_telemetry_conversion_failure", 1)
                raise

    def _parse_single_span(self, span_data: Dict[str, Any]) -> Optional[OTELSpan]:
        """Parse a single OpenTelemetry span."""
        try:
            # Parse timestamps
            start_time = datetime.fromisoformat(span_data.get("start_time", "").replace("Z", "+00:00"))
            end_time = datetime.fromisoformat(span_data.get("end_time", "").replace("Z", "+00:00"))
            
            return OTELSpan(
                name=span_data.get("name", "unknown"),
                trace_id=span_data.get("context", {}).get("trace_id", "unknown"),
                span_id=span_data.get("context", {}).get("span_id", "unknown"),
                parent_id=span_data.get("context", {}).get("parent_id"),
                start_time=start_time,
                end_time=end_time,
                status=span_data.get("status", {}).get("status_code", "UNSET"),
                attributes=span_data.get("attributes", {}),
                events=span_data.get("events", []),
                links=span_data.get("links", []),
                kind=span_data.get("kind", "INTERNAL"),
                resource=span_data.get("resource", {})
            )
        except Exception as e:
            self.logger.warning(f"Failed to parse span: {e}")
            return None

    def _parse_single_metric(self, metric_data: Dict[str, Any]) -> Optional[OTELMetric]:
        """Parse a single OpenTelemetry metric."""
        try:
            timestamp = datetime.fromisoformat(metric_data.get("timestamp", "").replace("Z", "+00:00"))
            
            return OTELMetric(
                name=metric_data.get("name", "unknown"),
                value=metric_data.get("value", 0),
                unit=metric_data.get("unit", ""),
                description=metric_data.get("description", ""),
                attributes=metric_data.get("attributes", {}),
                timestamp=timestamp
            )
        except Exception as e:
            self.logger.warning(f"Failed to parse metric: {e}")
            return None

    def _parse_single_log(self, log_data: Dict[str, Any]) -> Optional[OTELLog]:
        """Parse a single OpenTelemetry log."""
        try:
            timestamp = datetime.fromisoformat(log_data.get("timestamp", "").replace("Z", "+00:00"))
            
            return OTELLog(
                timestamp=timestamp,
                severity=log_data.get("severity", "INFO"),
                message=log_data.get("message", ""),
                attributes=log_data.get("attributes", {}),
                resource=log_data.get("resource", {}),
                trace_id=log_data.get("trace_id"),
                span_id=log_data.get("span_id")
            )
        except Exception as e:
            self.logger.warning(f"Failed to parse log: {e}")
            return None

    def _analyze_spans(self, spans: List[OTELSpan]) -> Dict[str, Any]:
        """Analyze spans for patterns and insights."""
        if not spans:
            return {"total_spans": 0}
        
        # Calculate durations
        durations = []
        for span in spans:
            duration = (span.end_time - span.start_time).total_seconds() * 1000  # Convert to ms
            durations.append(duration)
        
        # Analyze status distribution
        status_counts = {}
        for span in spans:
            status = span.status
            status_counts[status] = status_counts.get(status, 0) + 1
        
        # Analyze span names
        name_counts = {}
        for span in spans:
            name = span.name
            name_counts[name] = name_counts.get(name, 0) + 1
        
        return {
            "total_spans": len(spans),
            "avg_duration_ms": sum(durations) / len(durations) if durations else 0,
            "min_duration_ms": min(durations) if durations else 0,
            "max_duration_ms": max(durations) if durations else 0,
            "status_distribution": status_counts,
            "name_distribution": name_counts,
            "error_rate": status_counts.get("ERROR", 0) / len(spans) if spans else 0
        }

    def _analyze_metrics(self, metrics: List[OTELMetric]) -> Dict[str, Any]:
        """Analyze metrics for patterns and insights."""
        if not metrics:
            return {"total_metrics": 0}
        
        # Group metrics by name
        metric_groups = {}
        for metric in metrics:
            name = metric.name
            if name not in metric_groups:
                metric_groups[name] = []
            metric_groups[name].append(metric.value)
        
        # Calculate statistics for each metric
        metric_stats = {}
        for name, values in metric_groups.items():
            metric_stats[name] = {
                "count": len(values),
                "sum": sum(values),
                "avg": sum(values) / len(values),
                "min": min(values),
                "max": max(values)
            }
        
        return {
            "total_metrics": len(metrics),
            "unique_metric_names": len(metric_groups),
            "metric_statistics": metric_stats
        }

    def _analyze_logs(self, logs: List[OTELLog]) -> Dict[str, Any]:
        """Analyze logs for patterns and insights."""
        if not logs:
            return {"total_logs": 0}
        
        # Analyze severity distribution
        severity_counts = {}
        for log in logs:
            severity = log.severity
            severity_counts[severity] = severity_counts.get(severity, 0) + 1
        
        # Analyze log patterns
        error_logs = [log for log in logs if log.severity in ["ERROR", "FATAL"]]
        warning_logs = [log for log in logs if log.severity == "WARN"]
        
        return {
            "total_logs": len(logs),
            "severity_distribution": severity_counts,
            "error_count": len(error_logs),
            "warning_count": len(warning_logs),
            "error_rate": len(error_logs) / len(logs) if logs else 0
        }

    def _analyze_performance(self, trace: OTELTrace) -> Dict[str, Any]:
        """Analyze performance characteristics of the trace."""
        if not trace.spans:
            return {"total_duration_ms": 0}
        
        # Calculate total trace duration
        start_times = [span.start_time for span in trace.spans]
        end_times = [span.end_time for span in trace.spans]
        
        if start_times and end_times:
            trace_start = min(start_times)
            trace_end = max(end_times)
            total_duration = (trace_end - trace_start).total_seconds() * 1000
        else:
            total_duration = 0
        
        # Calculate span durations
        span_durations = []
        for span in trace.spans:
            duration = (span.end_time - span.start_time).total_seconds() * 1000
            span_durations.append(duration)
        
        return {
            "total_duration_ms": total_duration,
            "avg_span_duration_ms": sum(span_durations) / len(span_durations) if span_durations else 0,
            "min_span_duration_ms": min(span_durations) if span_durations else 0,
            "max_span_duration_ms": max(span_durations) if span_durations else 0,
            "span_count": len(trace.spans)
        }

    def _analyze_errors(self, trace: OTELTrace) -> Dict[str, Any]:
        """Analyze errors in the trace."""
        error_spans = [span for span in trace.spans if span.status == "ERROR"]
        error_logs = [log for log in trace.logs if log.severity in ["ERROR", "FATAL"]]
        
        # Extract error messages from logs
        error_messages = [log.message for log in error_logs]
        
        # Extract error attributes from spans
        error_attributes = []
        for span in error_spans:
            for event in span.events:
                if event.get("name") == "exception":
                    error_attributes.append(event.get("attributes", {}))
        
        return {
            "error_span_count": len(error_spans),
            "error_log_count": len(error_logs),
            "error_messages": error_messages,
            "error_attributes": error_attributes,
            "error_rate": len(error_spans) / len(trace.spans) if trace.spans else 0
        }

    def _analyze_dependencies(self, trace: OTELTrace) -> Dict[str, Any]:
        """Analyze service dependencies in the trace."""
        # Extract service names from resources
        services = set()
        for span in trace.spans:
            service_name = span.resource.get("attributes", {}).get("service.name", "unknown")
            services.add(service_name)
        
        # Analyze parent-child relationships
        parent_child_map = {}
        for span in trace.spans:
            if span.parent_id:
                if span.parent_id not in parent_child_map:
                    parent_child_map[span.parent_id] = []
                parent_child_map[span.parent_id].append(span.span_id)
        
        return {
            "unique_services": list(services),
            "service_count": len(services),
            "parent_child_relationships": len(parent_child_map),
            "max_depth": max(len(children) for children in parent_child_map.values()) if parent_child_map else 0
        } 