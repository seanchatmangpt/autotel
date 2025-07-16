# AutoTel OpenTelemetry Processor Documentation

## Overview

The AutoTel OpenTelemetry (OTEL) Processor is a comprehensive component designed to process, analyze, and convert OpenTelemetry data within the AutoTel semantic execution pipeline. It provides full integration with AutoTel's telemetry system and offers rich analysis capabilities for distributed tracing data.

## Architecture

### Core Components

```
OTEL Processor Architecture
├── OTELProcessor (Main Class)
│   ├── parse_spans() - Parse OTEL spans
│   ├── parse_metrics() - Parse OTEL metrics  
│   ├── parse_logs() - Parse OTEL logs
│   ├── parse_trace() - Parse complete traces
│   ├── analyze_trace() - Comprehensive analysis
│   └── convert_to_autotel_telemetry() - Format conversion
├── Data Structures
│   ├── OTELSpan - Span representation
│   ├── OTELMetric - Metric representation
│   ├── OTELLog - Log representation
│   └── OTELTrace - Complete trace
└── Analysis Modules
    ├── Span Analysis - Duration, status, patterns
    ├── Metric Analysis - Statistics, trends
    ├── Log Analysis - Severity, patterns
    ├── Performance Analysis - Timing, bottlenecks
    ├── Error Analysis - Error rates, patterns
    └── Dependency Analysis - Service relationships
```

### Integration Points

- **Factory Pipeline**: Integrated as `otel_processor` component
- **CLI Commands**: Full command-line interface support
- **Telemetry System**: Native OpenTelemetry integration
- **Data Conversion**: AutoTel telemetry format compatibility

## Data Structures

### OTELSpan
```python
@dataclass
class OTELSpan:
    name: str                    # Span name
    trace_id: str               # Trace identifier
    span_id: str                # Span identifier
    parent_id: Optional[str]    # Parent span ID
    start_time: datetime        # Start timestamp
    end_time: datetime          # End timestamp
    status: str                 # Span status (OK, ERROR, etc.)
    attributes: Dict[str, Any]  # Span attributes
    events: List[Dict[str, Any]] # Span events
    links: List[Dict[str, Any]]  # Span links
    kind: str                   # Span kind (SERVER, CLIENT, etc.)
    resource: Dict[str, Any]    # Resource information
```

### OTELMetric
```python
@dataclass
class OTELMetric:
    name: str                   # Metric name
    value: Union[int, float]    # Metric value
    unit: str                   # Unit of measurement
    description: str            # Metric description
    attributes: Dict[str, Any]  # Metric attributes
    timestamp: datetime         # Metric timestamp
```

### OTELLog
```python
@dataclass
class OTELLog:
    timestamp: datetime         # Log timestamp
    severity: str               # Log severity (INFO, ERROR, etc.)
    message: str                # Log message
    attributes: Dict[str, Any]  # Log attributes
    resource: Dict[str, Any]    # Resource information
    trace_id: Optional[str]     # Associated trace ID
    span_id: Optional[str]      # Associated span ID
```

## Usage Examples

### Basic Processing

```python
from autotel.factory.processors.otel_processor import OTELProcessor

# Create processor
processor = OTELProcessor()

# Sample OTEL data
otel_data = {
    "trace_id": "0x1234567890abcdef",
    "spans": [
        {
            "name": "http_request",
            "context": {
                "trace_id": "0x1234567890abcdef",
                "span_id": "0xabcdef1234567890",
                "parent_id": None
            },
            "start_time": "2025-07-16T15:30:00.000Z",
            "end_time": "2025-07-16T15:30:01.500Z",
            "status": {"status_code": "OK"},
            "attributes": {"http.method": "GET"},
            "events": [],
            "links": [],
            "kind": "SERVER",
            "resource": {"attributes": {"service.name": "user-service"}}
        }
    ],
    "metrics": [],
    "logs": []
}

# Process trace
trace = processor.parse_trace(otel_data)
analysis = processor.analyze_trace(trace)
autotel_telemetry = processor.convert_to_autotel_telemetry(trace)
```

### File Processing

```python
# Process from file
trace = processor.parse_file("otel_trace.json")
analysis = processor.analyze_trace(trace)
```

### Individual Component Processing

```python
# Parse individual components
spans = processor.parse_spans(otel_data["spans"])
metrics = processor.parse_metrics(otel_data["metrics"])
logs = processor.parse_logs(otel_data["logs"])
```

## CLI Usage

### Basic Commands

```bash
# Process OTEL data from file
python -m autotel.cli otel --process --file otel_data.json

# Process inline OTEL data
python -m autotel.cli otel --process --data '{"trace_id": "test", "spans": []}'

# Show span analysis
python -m autotel.cli otel --process --file otel_data.json --spans

# Show performance analysis
python -m autotel.cli otel --process --file otel_data.json --performance

# Show all analyses
python -m autotel.cli otel --process --file otel_data.json --spans --metrics --logs --performance --errors --dependencies
```

### CLI Options

| Option | Description | Example |
|--------|-------------|---------|
| `--process, -p` | Process OpenTelemetry data | `--process` |
| `--file, -f` | OTEL data file (JSON) | `--file trace.json` |
| `--data, -d` | OTEL data as JSON string | `--data '{"trace_id": "test"}'` |
| `--analyze, -a` | Analyze OTEL trace data | `--analyze` |
| `--convert, -c` | Convert to AutoTel format | `--convert` |
| `--output, -o` | Output file for results | `--output results.json` |
| `--spans` | Show span analysis | `--spans` |
| `--metrics` | Show metric analysis | `--metrics` |
| `--logs` | Show log analysis | `--logs` |
| `--performance` | Show performance analysis | `--performance` |
| `--errors` | Show error analysis | `--errors` |
| `--dependencies` | Show dependency analysis | `--dependencies` |
| `--quiet, -q` | Suppress verbose output | `--quiet` |

## Analysis Capabilities

### Span Analysis

The processor provides comprehensive span analysis including:

- **Duration Statistics**: Average, minimum, maximum span durations
- **Status Distribution**: Count of spans by status (OK, ERROR, etc.)
- **Name Distribution**: Frequency of span names
- **Error Rate**: Percentage of spans with error status

```python
span_analysis = {
    "total_spans": 2,
    "avg_duration_ms": 1050.0,
    "min_duration_ms": 600.0,
    "max_duration_ms": 1500.0,
    "status_distribution": {"OK": 2},
    "name_distribution": {"http_request": 1, "database_query": 1},
    "error_rate": 0.0
}
```

### Performance Analysis

Performance analysis focuses on timing characteristics:

- **Total Duration**: Complete trace duration
- **Span Duration Statistics**: Average, min, max span durations
- **Span Count**: Total number of spans in trace

```python
performance_analysis = {
    "total_duration_ms": 1500.0,
    "avg_span_duration_ms": 1050.0,
    "min_span_duration_ms": 600.0,
    "max_span_duration_ms": 1500.0,
    "span_count": 2
}
```

### Error Analysis

Error analysis identifies and categorizes errors:

- **Error Span Count**: Number of spans with error status
- **Error Log Count**: Number of error-level logs
- **Error Messages**: Extracted error messages from logs
- **Error Rate**: Overall error rate percentage

```python
error_analysis = {
    "error_span_count": 0,
    "error_log_count": 0,
    "error_messages": [],
    "error_rate": 0.0
}
```

### Dependency Analysis

Dependency analysis examines service relationships:

- **Unique Services**: List of services involved
- **Service Count**: Number of unique services
- **Parent-Child Relationships**: Span hierarchy analysis
- **Max Depth**: Maximum depth of span hierarchy

```python
dependency_analysis = {
    "unique_services": ["user-service"],
    "service_count": 1,
    "parent_child_relationships": 1,
    "max_depth": 1
}
```

## Pipeline Integration

### Factory Pipeline Integration

The OTEL processor is fully integrated into the AutoTel factory pipeline:

```python
from autotel.factory.pipeline import PipelineOrchestrator

# Create pipeline with OTEL processor
pipeline = PipelineOrchestrator()

# Process OTEL data through pipeline
result = pipeline.process_otel_data(otel_data)

# Access results
trace_id = result["trace_id"]
analysis = result["analysis"]
autotel_telemetry = result["autotel_telemetry"]
metadata = result["metadata"]
```

### Pipeline Methods

#### `process_otel_data(otel_data, analysis_config=None)`

Processes OTEL data and returns comprehensive analysis:

- **Parameters**:
  - `otel_data`: Dictionary containing OTEL trace data
  - `analysis_config`: Optional configuration for analysis
- **Returns**: Dictionary with trace_id, analysis, autotel_telemetry, and metadata

#### `process_otel_file(otel_file, analysis_config=None)`

Processes OTEL data from file:

- **Parameters**:
  - `otel_file`: Path to OTEL data file
  - `analysis_config`: Optional configuration for analysis
- **Returns**: Parsed OTEL trace object

## Telemetry Integration

### Native OpenTelemetry Support

The OTEL processor includes full OpenTelemetry integration:

- **Span Creation**: Each operation creates detailed spans
- **Metric Recording**: Performance metrics are automatically recorded
- **Error Tracking**: Errors are captured and reported
- **Context Propagation**: Trace context is maintained throughout processing

### Telemetry Metrics

The processor records various metrics:

- `otel_spans_parsed`: Number of spans successfully parsed
- `otel_metrics_parsed`: Number of metrics successfully parsed
- `otel_logs_parsed`: Number of logs successfully parsed
- `otel_traces_parsed`: Number of traces successfully parsed
- `otel_traces_analyzed`: Number of traces analyzed
- `otel_telemetry_conversions`: Number of telemetry conversions

### Error Handling

The processor includes comprehensive error handling:

- **Parse Failures**: Individual span/metric/log parsing failures are logged
- **Analysis Failures**: Analysis errors are captured in telemetry
- **Conversion Failures**: Conversion errors are tracked
- **Graceful Degradation**: Processing continues even with partial failures

## Configuration

### OTELProcessingConfig

```python
@dataclass
class OTELProcessingConfig:
    enable_span_analysis: bool = True
    enable_metric_analysis: bool = True
    enable_log_analysis: bool = True
    enable_performance_analysis: bool = True
    enable_error_analysis: bool = True
    enable_dependency_analysis: bool = True
    include_attributes: bool = True
    include_events: bool = True
    include_links: bool = True
    max_spans_per_trace: int = 10000
    max_metrics_per_trace: int = 1000
    max_logs_per_trace: int = 10000
    error_threshold: float = 0.1
    performance_threshold_ms: float = 1000.0
```

## Error Handling

### Common Error Scenarios

1. **Invalid JSON Data**
   ```python
   try:
       trace = processor.parse_trace(invalid_data)
   except json.JSONDecodeError as e:
       # Handle JSON parsing errors
   ```

2. **Missing Required Fields**
   ```python
   # Processor handles missing fields gracefully
   # Returns None for unparseable components
   ```

3. **File Not Found**
   ```python
   try:
       trace = processor.parse_file("nonexistent.json")
   except FileNotFoundError as e:
       # Handle file not found errors
   ```

### Error Recovery

The processor implements robust error recovery:

- **Individual Component Parsing**: Failed spans/metrics/logs are skipped
- **Partial Analysis**: Analysis continues with available data
- **Telemetry Recording**: All errors are recorded for monitoring
- **Graceful Degradation**: System continues operating with reduced functionality

## Performance Considerations

### Optimization Features

1. **Lazy Parsing**: Components are parsed only when needed
2. **Memory Efficiency**: Large traces are processed incrementally
3. **Caching**: Parsed data is cached for repeated access
4. **Batch Processing**: Multiple traces can be processed efficiently

### Performance Limits

- **Max Spans per Trace**: 10,000 (configurable)
- **Max Metrics per Trace**: 1,000 (configurable)
- **Max Logs per Trace**: 10,000 (configurable)
- **Memory Usage**: Scales linearly with trace size

## Testing

### Unit Tests

```python
def test_otel_processor():
    processor = OTELProcessor()
    
    # Test parsing
    trace = processor.parse_trace(sample_data)
    assert trace.trace_id == "0x1234567890abcdef"
    assert len(trace.spans) == 2
    
    # Test analysis
    analysis = processor.analyze_trace(trace)
    assert analysis["span_analysis"]["total_spans"] == 2
    
    # Test conversion
    autotel_telemetry = processor.convert_to_autotel_telemetry(trace)
    assert len(autotel_telemetry.spans) == 2
```

### Integration Tests

```python
def test_pipeline_integration():
    pipeline = PipelineOrchestrator()
    result = pipeline.process_otel_data(sample_data)
    
    assert result["trace_id"] == "0x1234567890abcdef"
    assert "analysis" in result
    assert "autotel_telemetry" in result
```

## Best Practices

### Data Preparation

1. **Validate JSON Structure**: Ensure OTEL data follows standard format
2. **Include Required Fields**: Provide trace_id, spans, metrics, logs
3. **Use ISO Timestamps**: Ensure timestamps are in ISO format
4. **Normalize Data**: Clean and normalize data before processing

### Performance Optimization

1. **Batch Processing**: Process multiple traces together
2. **Selective Analysis**: Enable only needed analysis types
3. **Memory Management**: Process large traces in chunks
4. **Caching**: Cache frequently accessed analysis results

### Error Handling

1. **Validate Input**: Check data format before processing
2. **Handle Failures**: Implement proper error recovery
3. **Log Errors**: Record all errors for debugging
4. **Graceful Degradation**: Continue processing with partial data

## Troubleshooting

### Common Issues

1. **Import Errors**
   ```bash
   # Ensure all dependencies are installed
   pip install opentelemetry-api opentelemetry-sdk
   ```

2. **Memory Issues**
   ```python
   # Reduce limits for large traces
   config = OTELProcessingConfig(max_spans_per_trace=1000)
   ```

3. **Performance Issues**
   ```python
   # Disable unnecessary analysis
   config = OTELProcessingConfig(enable_dependency_analysis=False)
   ```

### Debug Mode

Enable debug logging for troubleshooting:

```python
import logging
logging.basicConfig(level=logging.DEBUG)

processor = OTELProcessor()
# Debug information will be logged
```

## Future Enhancements

### Planned Features

1. **Real-time Processing**: Stream processing capabilities
2. **Advanced Analytics**: Machine learning-based anomaly detection
3. **Visualization**: Built-in trace visualization
4. **Export Formats**: Support for additional export formats
5. **Performance Profiling**: Advanced performance analysis tools

### Extension Points

The OTEL processor is designed for extensibility:

1. **Custom Analyzers**: Add custom analysis modules
2. **Export Plugins**: Implement custom export formats
3. **Filtering**: Add custom filtering capabilities
4. **Aggregation**: Implement custom aggregation functions

## Conclusion

The AutoTel OpenTelemetry Processor provides a comprehensive solution for processing and analyzing OpenTelemetry data within the AutoTel ecosystem. With its rich analysis capabilities, seamless integration, and robust error handling, it serves as a powerful tool for distributed tracing and observability.

For more information, refer to the AutoTel documentation and examples in the project repository. 