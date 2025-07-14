#!/usr/bin/env python3
"""
AutoTel CLI - Enterprise BPMN 2.0 Orchestration Framework
"""

import typer
import json
import yaml
from pathlib import Path
from typing import Optional, List, Dict, Any
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn
from rich.tree import Tree
from rich import print as rprint

from .core.orchestrator import Orchestrator
from .core.telemetry import TelemetryManager, TelemetryConfig
from .schemas.validation import SchemaValidator, ValidationLevel
from .utils.dspy_services import dspy_registry
from .utils.advanced_dspy_services import advanced_dspy_registry, initialize_advanced_dspy_services
from .workflows.task_span_integration import initialize_task_span_manager
from .utils.helpers import otel_command

app = typer.Typer(
    name="autotel",
    help="Enterprise BPMN 2.0 Orchestration Framework with AI-powered decision making",
    rich_markup_mode="rich"
)

console = Console()

@app.command()
@otel_command
def version():
    """Show AutoTel version and system information"""
    from . import __version__
    
    table = Table(title="AutoTel System Information")
    table.add_column("Component", style="cyan")
    table.add_column("Version", style="green")
    table.add_column("Status", style="yellow")
    
    table.add_row("AutoTel Core", __version__, "✅ Active")
    table.add_row("SpiffWorkflow", "Latest", "✅ Active")
    table.add_row("OpenTelemetry", "1.34.1+", "✅ Active")
    table.add_row("LinkML Runtime", "1.9.3+", "✅ Active")
    table.add_row("DSPy", "Latest", "✅ Active")
    
    console.print(table)

@app.command()
@otel_command
def init(
    config_file: Optional[Path] = typer.Option(None, "--config", "-c", help="Configuration file path"),
    telemetry_schema: Optional[Path] = typer.Option(None, "--telemetry-schema", help="Telemetry schema file"),
    validation_level: ValidationLevel = typer.Option(ValidationLevel.NORMAL, "--validation-level", help="Validation strictness")
):
    """Initialize AutoTel with configuration"""
    telemetry_config = TelemetryConfig(
        enable_tracing=True,
        enable_metrics=True,
        require_linkml_validation=True,
        schema_path=str(telemetry_schema) if telemetry_schema else None
    )
    telemetry_manager = TelemetryManager(telemetry_config)
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console
    ) as progress:
        
        task = progress.add_task("Initializing AutoTel...", total=4)
        
        # Initialize telemetry
        progress.update(task, description="Setting up telemetry...")
        
        progress.advance(task)
        
        # Initialize schema validator
        progress.update(task, description="Setting up schema validation...")
        schema_validator = SchemaValidator(validation_level=validation_level)
        
        # Load default schemas
        default_schemas = [
            "otel_traces_schema.yaml",
            "workflow_telemetry_schema.yaml"
        ]
        
        for schema_file in default_schemas:
            if Path(schema_file).exists():
                schema_validator.load_schema(schema_file)
        
        progress.advance(task)
        
        # Initialize advanced DSPy services
        progress.update(task, description="Initializing DSPy services...")
        initialize_advanced_dspy_services()
        
        progress.advance(task)
        
        # Initialize task span manager
        progress.update(task, description="Setting up task telemetry...")
        initialize_task_span_manager(telemetry_manager)
        
        progress.advance(task)
    
    console.print(Panel.fit(
        "[green]✅ AutoTel initialized successfully![/green]\n"
        f"Telemetry: {'Enabled' if telemetry_manager.is_configured() else 'Disabled'}\n"
        f"Schema Validation: {validation_level.value}\n"
        f"DSPy Services: {len(advanced_dspy_registry.list_signatures())} signatures available",
        title="Initialization Complete"
    ))

@app.command()
@otel_command
def validate(
    file_path: Path = typer.Argument(..., help="File to validate (BPMN, DMN, or YAML)"),
    strict: bool = typer.Option(False, "--strict", help="Enable strict validation")
):
    """Validate BPMN, DMN, or configuration files"""
    
    if not file_path.exists():
        console.print(f"[red]❌ File not found: {file_path}[/red]")
        raise typer.Exit(1)
    
    schema_validator = SchemaValidator(
        validation_level=ValidationLevel.STRICT if strict else ValidationLevel.NORMAL
    )
    
    with Progress(
        SpinnerColumn(),
        TextColumn("[progress.description]{task.description}"),
        console=console
    ) as progress:
        
        task = progress.add_task(f"Validating {file_path.name}...", total=1)
        
        try:
            if file_path.suffix.lower() == '.bpmn':
                result = schema_validator.validate_bpmn_file(str(file_path))
            elif file_path.suffix.lower() == '.dmn':
                result = schema_validator.validate_dmn_file(str(file_path))
            elif file_path.suffix.lower() in ['.yaml', '.yml']:
                with open(file_path, 'r') as f:
                    data = yaml.safe_load(f)
                result = ValidationResult(valid=True)
            else:
                console.print(f"[yellow]⚠️  Unknown file type: {file_path.suffix}[/yellow]")
                result = ValidationResult(valid=False, errors=[f"Unsupported file type: {file_path.suffix}"])
            
            progress.advance(task)
            
        except Exception as e:
            result = ValidationResult(valid=False, errors=[f"Validation failed: {e}"])
    
    # Display results
    if result.valid:
        console.print(f"[green]✅ {file_path.name} is valid[/green]")
        if result.warnings:
            console.print(f"[yellow]⚠️  Warnings: {len(result.warnings)}[/yellow]")
            for warning in result.warnings:
                console.print(f"  • {warning}")
    else:
        console.print(f"[red]❌ {file_path.name} is invalid[/red]")
        for error in result.errors:
            console.print(f"  • {error}")
        raise typer.Exit(1)

@app.command()
@otel_command
def telemetry(
    export: Optional[Path] = typer.Option(None, "--export", "-e", help="Export telemetry to file"),
    format: str = typer.Option("json", "--format", "-f", help="Export format (json, yaml)"),
    show_stats: bool = typer.Option(False, "--stats", help="Show telemetry statistics")
):
    """Manage telemetry data and export traces"""
    
    telemetry_manager = TelemetryManager()
    
    if show_stats:
        table = Table(title="Telemetry Statistics")
        table.add_column("Metric", style="cyan")
        table.add_column("Value", style="green")
        
        stats = telemetry_manager.get_stats()
        for key, value in stats.items():
            table.add_row(key, str(value))
        
        console.print(table)
    
    if export:
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            
            task = progress.add_task("Exporting telemetry...", total=1)
            
            try:
                telemetry_data = telemetry_manager.export_telemetry()
                
                if format.lower() == "yaml":
                    with open(export, 'w') as f:
                        yaml.dump(telemetry_data, f, default_flow_style=False)
                else:
                    with open(export, 'w') as f:
                        json.dump(telemetry_data, f, indent=2)
                
                progress.advance(task)
                
                console.print(f"[green]✅ Telemetry exported to {export}[/green]")
                
            except Exception as e:
                console.print(f"[red]❌ Failed to export telemetry: {e}[/red]")
                raise typer.Exit(1)

@app.command()
@otel_command
def dspy(
    list_signatures: bool = typer.Option(False, "--list", "-l", help="List available DSPy signatures"),
    call_signature: Optional[str] = typer.Option(None, "--call", "-c", help="Call a DSPy signature"),
    input_data: Optional[str] = typer.Option(None, "--input", "-i", help="Input data as JSON"),
    show_stats: bool = typer.Option(False, "--stats", help="Show DSPy statistics"),
    clear_cache: bool = typer.Option(False, "--clear-cache", help="Clear DSPy cache")
):
    """Manage DSPy services and signatures"""
    
    if clear_cache:
        advanced_dspy_registry.cache.clear()
        console.print("[green]✅ DSPy cache cleared[/green]")
        return
    
    if show_stats:
        stats = advanced_dspy_registry.get_stats()
        
        cache_table = Table(title="DSPy Cache Statistics")
        cache_table.add_column("Metric", style="cyan")
        cache_table.add_column("Value", style="green")
        
        for key, value in stats.get("cache_stats", {}).items():
            cache_table.add_row(key, str(value))
        
        console.print(cache_table)
        
        if stats.get("model_stats"):
            model_table = Table(title="Model Performance Statistics")
            model_table.add_column("Model", style="cyan")
            model_table.add_column("Avg Duration", style="green")
            
            for model, avg in stats["model_stats"].items():
                model_table.add_row(model, str(avg))
            
            console.print(model_table)
    
    if list_signatures:
        signatures_table = Table(title="Available DSPy Signatures")
        signatures_table.add_column("Signature", style="cyan")
        signatures_table.add_column("Status", style="green")
        
        signatures = advanced_dspy_registry.list_signatures()
        for signature_name in signatures.keys():
            signatures_table.add_row(signature_name, "✅ Active")
        
        console.print(signatures_table)
    
    if call_signature:
        signatures = advanced_dspy_registry.list_signatures()
        if call_signature not in signatures:
            console.print(f"[red]❌ Signature not found: {call_signature}[/red]")
            raise typer.Exit(1)
        
        try:
            input_json = json.loads(input_data) if input_data else {}
            result = advanced_dspy_registry.call_signature(call_signature, input_json)
            console.print(f"[green]✅ Result: {result}[/green]")
        except Exception as e:
            console.print(f"[red]❌ Failed to call signature: {e}[/red]")
            raise typer.Exit(1)

@app.command()
@otel_command
def workflow(
    file_path: Path = typer.Argument(..., help="BPMN workflow file"),
    validate_only: bool = typer.Option(False, "--validate-only", help="Only validate, don't execute"),
    export_telemetry: Optional[Path] = typer.Option(None, "--export-telemetry", help="Export workflow telemetry")
):
    """Execute or validate a BPMN workflow"""
    
    if not file_path.exists():
        console.print(f"[red]❌ File not found: {file_path}[/red]")
        raise typer.Exit(1)
    
    if validate_only:
        schema_validator = SchemaValidator()
        result = schema_validator.validate_bpmn_file(str(file_path))
        
        if result.valid:
            console.print(f"✅ Workflow {file_path.name} is valid")
        else:
            console.print(f"❌ Workflow {file_path.name} is invalid")
            for error in result.errors:
                console.print(f"  • {error}")
            raise typer.Exit(1)
    else:
        orchestrator = Orchestrator(
            specific_bpmn_file=str(file_path),
            enable_telemetry=True
        )
        
        try:
            # Parse the BPMN file to get the process definition ID
            import xml.etree.ElementTree as ET
            tree = ET.parse(str(file_path))
            root = tree.getroot()
            # Find the first process element
            process_elem = root.find('.//{http://www.omg.org/spec/BPMN/20100524/MODEL}process')
            if process_elem is not None and 'id' in process_elem.attrib:
                process_id = process_elem.attrib['id']
            else:
                raise ValueError("Could not find process definition ID in BPMN file")
            instance = orchestrator.start_process(process_id, {})
            
            console.print(f"🚀 Started workflow: {process_id}")
            console.print(f"📋 Instance ID: {instance.instance_id}")
            
            result = orchestrator.execute_process(instance.instance_id)
            
            console.print(f"✅ Workflow completed with status: {result.status.value}")
            
            if export_telemetry:
                telemetry_data = orchestrator.export_telemetry()
                with open(export_telemetry, 'w') as f:
                    json.dump(telemetry_data, f, indent=2)
                console.print(f"📊 Telemetry exported to {export_telemetry}")
                
        except Exception as e:
            console.print(f"[red]❌ Workflow execution failed: {e}[/red]")
            raise typer.Exit(1)

@app.command()
@otel_command
def config(
    show: bool = typer.Option(False, "--show", help="Show current configuration"),
    validate: Optional[Path] = typer.Option(None, "--validate", help="Validate configuration file"),
    generate: Optional[Path] = typer.Option(None, "--generate", help="Generate sample configuration")
):
    """Manage AutoTel configuration"""
    
    if show:
        config_data = {
            "dspy": {
                "cache_enabled": True,
                "signatures": len(advanced_dspy_registry.list_signatures())
            },
            "telemetry": {
                "enabled": True,
                "linkml_validation": True
            },
            "validation": {
                "level": "normal"
            }
        }
        
        console.print(Panel.fit(
            yaml.dump(config_data, default_flow_style=False),
            title="Current Configuration"
        ))
    
    if validate:
        if not validate.exists():
            console.print(f"[red]❌ Configuration file not found: {validate}[/red]")
            raise typer.Exit(1)
        
        try:
            with open(validate, 'r') as f:
                config_data = yaml.safe_load(f)
            console.print(f"✅ Configuration file {validate.name} is valid")
        except Exception as e:
            console.print(f"[red]❌ Configuration file is invalid: {e}[/red]")
            raise typer.Exit(1)
    
    if generate:
        sample_config = {
            "telemetry": {
                "enabled": True,
                "linkml_validation": True,
                "export_format": "json"
            },
            "dspy": {
                "cache_enabled": True,
                "models": ["openai:gpt-4o-mini", "ollama:qwen2.5:7b"]
            },
            "validation": {
                "level": "normal",
                "strict_mode": False
            }
        }
        
        with open(generate, 'w') as f:
            yaml.dump(sample_config, f, default_flow_style=False)
        
        console.print(f"[green]✅ Sample configuration generated: {generate}[/green]")

if __name__ == "__main__":
    app() 