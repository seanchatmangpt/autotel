import json
from pathlib import Path
from typing import Any, Dict, List, Optional
from autotel.processors.base import BaseProcessor, ProcessorConfig, ProcessorResult
from autotel.processors.meta import processor_metadata
from autotel.helpers.telemetry.span import create_processor_span, record_span_success, record_span_error

@processor_metadata(
    name="otel_processor",
    version="1.0.0",
    capabilities=["otel_parse", "telemetry_extraction"],
    supported_formats=["json", "otel"],
    author="AutoTel Team"
)
class OTELProcessor(BaseProcessor):
    """
    Unified OTEL processor for AutoTel.
    Parses OpenTelemetry JSON trace files and extracts spans, metrics, logs (happy path).
    """
    def _process_impl(self, data: Any) -> ProcessorResult:
        """
        Parse OpenTelemetry JSON file or string and extract spans, metrics, logs (happy path).
        Args:
            data: Path to JSON file or JSON string
        Returns:
            ProcessorResult with dict containing spans, metrics, logs, and metadata
        """
        with create_processor_span("parse", "otel") as span:
            try:
                json_data = self._get_json_data(data)
                # Happy path: expect top-level keys: spans, metrics, logs
                spans = json_data.get("spans", [])
                metrics = json_data.get("metrics", [])
                logs = json_data.get("logs", [])
                metadata = {
                    "spans_count": len(spans),
                    "metrics_count": len(metrics),
                    "logs_count": len(logs)
                }
                record_span_success(span, metadata)
                return ProcessorResult.success_result(
                    data={
                        "spans": spans,
                        "metrics": metrics,
                        "logs": logs
                    },
                    metadata=metadata
                )
            except Exception as e:
                record_span_error(span, e, {"input_type": type(data).__name__})
                return ProcessorResult.error_result(str(e), {"error_type": type(e).__name__})

    def _get_json_data(self, data: Any) -> Dict[str, Any]:
        """Get JSON data from file path or string."""
        if isinstance(data, Path) or (isinstance(data, str) and len(data) < 256 and Path(data).exists()):
            path = Path(data) if isinstance(data, str) else data
            return json.loads(path.read_text(encoding="utf-8"))
        elif isinstance(data, str):
            return json.loads(data)
        else:
            raise ValueError("Input must be a JSON string or file path") 
 