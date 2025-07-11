# AutoTel CLI - Enterprise BPMN 2.0 Orchestration

AutoTel CLI provides a comprehensive command-line interface for enterprise BPMN 2.0 orchestration with zero-touch telemetry integration.

## Installation

The CLI is automatically available when you install the AutoTel package:

```bash
# Using uv (recommended)
uv sync

# Or using pip
pip install -e .
```

## Quick Start

1. **Initialize AutoTel**:
   ```bash
   uv run python autotel_cli.py init
   ```

2. **List available processes**:
   ```bash
   uv run python autotel_cli.py list-processes
   ```

3. **Start a process**:
   ```bash
   uv run python autotel_cli.py start-process "process-id"
   ```

4. **Execute a process**:
   ```bash
   uv run python autotel_cli.py execute-process "instance-id"
   ```

## Commands Overview

### Core Orchestration Commands

#### `init`
Initialize AutoTel orchestrator and framework.

```bash
uv run python autotel_cli.py init [OPTIONS]
```

**Options:**
- `--bpmn-path PATH`: Path to BPMN files (default: `bpmn/`)
- `--telemetry/--no-telemetry`: Enable telemetry (default: enabled)
- `--persistence/--no-persistence`: Enable persistence (default: enabled)

**Example:**
```bash
uv run python autotel_cli.py init --bpmn-path ./my-processes --no-telemetry
```

#### `list-processes`
List available BPMN process definitions.

```bash
uv run python autotel_cli.py list-processes
```

#### `start-process`
Start a new BPMN process instance.

```bash
uv run python autotel_cli.py start-process PROCESS_ID [OPTIONS]
```

**Options:**
- `--variables TEXT`: JSON variables string
- `--instance-id TEXT`: Custom instance ID

**Example:**
```bash
uv run python autotel_cli.py start-process "order-process" \
  --variables '{"customer_id": "123", "amount": 100.50}' \
  --instance-id "order-2024-001"
```

#### `execute-process`
Execute a BPMN process instance.

```bash
uv run python autotel_cli.py execute-process INSTANCE_ID [OPTIONS]
```

**Options:**
- `--max-steps INTEGER`: Maximum execution steps (default: 100)
- `--interactive`: Interactive execution mode

**Example:**
```bash
uv run python autotel_cli.py execute-process "order-2024-001" --max-steps 50
```

#### `list-instances`
List process instances with optional filtering.

```bash
uv run python autotel_cli.py list-instances [OPTIONS]
```

**Options:**
- `--status TEXT`: Filter by status (pending, running, completed, failed, suspended, cancelled)
- `--process-id TEXT`: Filter by process ID

**Example:**
```bash
uv run python autotel_cli.py list-instances --status running --process-id "order-process"
```

#### `cancel-process`
Cancel a running process instance.

```bash
uv run python autotel_cli.py cancel-process INSTANCE_ID
```

**Example:**
```bash
uv run python autotel_cli.py cancel-process "order-2024-001"
```

### Process Management Commands

#### `get-variables`
Get process instance variables.

```bash
uv run python autotel_cli.py get-variables INSTANCE_ID
```

#### `set-variables`
Set process instance variables.

```bash
uv run python autotel_cli.py set-variables INSTANCE_ID VARIABLES
```

**Example:**
```bash
uv run python autotel_cli.py set-variables "order-2024-001" '{"status": "approved"}'
```

#### `get-ready-tasks`
Get ready tasks for a process instance.

```bash
uv run python autotel_cli.py get-ready-tasks INSTANCE_ID
```

#### `complete-task`
Complete a task in a process instance.

```bash
uv run python autotel_cli.py complete-task INSTANCE_ID TASK_ID [OPTIONS]
```

**Options:**
- `--data TEXT`: JSON data for task completion

**Example:**
```bash
uv run python autotel_cli.py complete-task "order-2024-001" "task-123" \
  --data '{"result": "approved", "approver": "john.doe"}'
```

### Monitoring and Maintenance Commands

#### `get-statistics`
Get process execution statistics.

```bash
uv run python autotel_cli.py get-statistics
```

#### `cleanup`
Clean up completed process instances.

```bash
uv run python autotel_cli.py cleanup [OPTIONS]
```

**Options:**
- `--max-age INTEGER`: Maximum age in hours for cleanup (default: 24)

**Example:**
```bash
uv run python autotel_cli.py cleanup --max-age 48
```

### Schema and Workflow Commands

#### `validate-schema`
Validate a LinkML schema.

```bash
uv run python autotel_cli.py validate-schema SCHEMA_FILE
```

**Example:**
```bash
uv run python autotel_cli.py validate-schema ./schemas/my-schema.yaml
```

#### `process-ontology`
Process a LinkML ontology and generate models.

```bash
uv run python autotel_cli.py process-ontology ONTOLOGY_FILE
```

**Example:**
```bash
uv run python autotel_cli.py process-ontology ./ontologies/business-ontology.yaml
```

#### `execute-workflow`
Execute a workflow from a JSON file.

```bash
uv run python autotel_cli.py execute-workflow WORKFLOW_FILE
```

**Example:**
```bash
uv run python autotel_cli.py execute-workflow ./workflows/order-workflow.json
```

#### `check-spiff-capabilities`
Check SpiffWorkflow capabilities and configuration.

```bash
uv run python autotel_cli.py check-spiff-capabilities [OPTIONS]
```

**Options:**
- `--config PATH`: Configuration file for capability check

**Example:**
```bash
uv run python autotel_cli.py check-spiff-capabilities --config ./spiff-config.yaml
```

### Utility Commands

#### `demo`
Run AutoTel demonstration workflow.

```bash
uv run python autotel_cli.py demo
```

#### `version`
Show AutoTel version information.

```bash
uv run python autotel_cli.py version
```

## Workflow Examples

### Example 1: Order Processing Workflow

1. **Initialize the system**:
   ```bash
   uv run python autotel_cli.py init
   ```

2. **Start an order process**:
   ```bash
   uv run python autotel_cli.py start-process "order-process" \
     --variables '{"customer_id": "CUST-123", "items": [{"product_id": "PROD-001", "quantity": 2}]}'
   ```

3. **Execute the process**:
   ```bash
   uv run python autotel_cli.py execute-process "order-process_abc12345"
   ```

4. **Check status**:
   ```bash
   uv run python autotel_cli.py list-instances --status running
   ```

### Example 2: Using Workflow Files

1. **Create a workflow file** (`my-workflow.json`):
   ```json
   [
     {
       "operation": "ProcessOrder",
       "inputs": {
         "order_id": "ORD-001",
         "customer_id": "CUST-123"
       }
     },
     {
       "operation": "ValidatePayment",
       "inputs": {
         "payment_method": "credit_card",
         "amount": 100.50
       }
     }
   ]
   ```

2. **Execute the workflow**:
   ```bash
   uv run python autotel_cli.py execute-workflow my-workflow.json
   ```

### Example 3: Interactive Process Execution

1. **Start a process**:
   ```bash
   uv run python autotel_cli.py start-process "human-task-process"
   ```

2. **Execute interactively**:
   ```bash
   uv run python autotel_cli.py execute-process "human-task-process_abc12345" --interactive
   ```

3. **Complete tasks as they become ready**:
   ```bash
   uv run python autotel_cli.py get-ready-tasks "human-task-process_abc12345"
   uv run python autotel_cli.py complete-task "human-task-process_abc12345" "task-123"
   ```

## Configuration

### Environment Variables

- `AUTOTEL_LOG_LEVEL`: Set logging level (DEBUG, INFO, WARNING, ERROR)
- `AUTOTEL_CONFIG_FILE`: Path to configuration file
- `AUTOTEL_BPMN_PATH`: Default path to BPMN files

### Configuration File

Create a `autotel-config.yaml` file:

```yaml
telemetry:
  enabled: true
  service_name: "autotel-service"
  service_version: "1.0.0"

bpmn:
  path: "./bpmn"
  auto_load: true

persistence:
  enabled: true
  storage_path: "./data"

logging:
  level: "INFO"
  format: "json"
```

## Troubleshooting

### Common Issues

1. **"Orchestrator not initialized"**
   - Run `uv run python autotel_cli.py init` first

2. **"Process definition not found"**
   - Check that BPMN files exist in the specified path
   - Verify BPMN file syntax

3. **"Module not found" errors**
   - Run `uv sync` to install dependencies
   - Check that you're using the correct Python environment

4. **OpenTelemetry warnings**
   - These are normal and don't affect functionality
   - Can be suppressed by setting log level to ERROR

### Debug Mode

Enable verbose output for debugging:

```bash
uv run python autotel_cli.py --verbose init
```

## Integration

### With CI/CD Pipelines

```yaml
# Example GitHub Actions workflow
- name: Test AutoTel CLI
  run: |
    uv sync
    uv run python autotel_cli.py init
    uv run python autotel_cli.py version
    uv run python autotel_cli.py validate-schema ./schemas/telemetry.yaml
```

### With Docker

```dockerfile
FROM python:3.11-slim

WORKDIR /app
COPY . .

RUN pip install uv
RUN uv sync

ENTRYPOINT ["uv", "run", "python", "autotel_cli.py"]
```

## Contributing

When adding new CLI commands:

1. Follow the existing command structure
2. Use descriptive command names
3. Include proper help text
4. Add type hints
5. Include error handling
6. Test with `uv run python autotel_cli.py --help`

## Support

For issues and questions:
- Check the troubleshooting section
- Review the command help: `uv run python autotel_cli.py <command> --help`
- Check the main AutoTel documentation 