#!/usr/bin/env python3
"""
AutoTel CLI - Enterprise BPMN 2.0 Orchestration Framework
"""

import typer
import json
import yaml
from pathlib import Path
from typing import Optional, List, Dict, Any
from datetime import datetime
from dataclasses import dataclass, field
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

@dataclass
class ValidationResult:
    """Result of validation operation"""
    valid: bool
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)

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
            "schemas/otel_traces_schema.yaml",
            "schemas/workflow_telemetry_schema.yaml"
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
def run(
    workflow: Path = typer.Argument(..., help="BPMN workflow file to execute"),
    input_data: Optional[str] = typer.Option(None, "--input", "-i", help="Input data as JSON string"),
    input_file: Optional[Path] = typer.Option(None, "--input-file", help="Input data file (JSON)"),
    process_id: Optional[str] = typer.Option(None, "--process-id", help="Process ID to execute"),
    dmn_files: Optional[List[Path]] = typer.Option(None, "--dmn", help="DMN decision files"),
    output_file: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file for results"),
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Verbose output")
):
    """Run a BPMN workflow with input data"""
    
    if not workflow.exists():
        console.print(f"[red]❌ Workflow file not found: {workflow}[/red]")
        raise typer.Exit(1)
    
    # Parse input data
    context = {}
    if input_data:
        try:
            context = json.loads(input_data)
        except json.JSONDecodeError as e:
            console.print(f"[red]❌ Invalid JSON input: {e}[/red]")
            raise typer.Exit(1)
    elif input_file:
        if not input_file.exists():
            console.print(f"[red]❌ Input file not found: {input_file}[/red]")
            raise typer.Exit(1)
        try:
            with open(input_file, 'r') as f:
                context = json.load(f)
        except Exception as e:
            console.print(f"[red]❌ Failed to read input file: {e}[/red]")
            raise typer.Exit(1)
    
    # Convert DMN files to strings
    dmn_file_paths = [str(dmn) for dmn in dmn_files] if dmn_files else None
    
    try:
        from .workflows.spiff import run_dspy_bpmn_process
        
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            
            task = progress.add_task(f"Executing workflow {workflow.name}...", total=1)
            
            # Execute workflow
            result = run_dspy_bpmn_process(
                bpmn_path=str(workflow),
                process_id=process_id or "default",
                context=context,
                dmn_files=dmn_file_paths
            )
            
            progress.advance(task)
        
        # Display results
        if verbose:
            console.print(f"[green]✅ Workflow completed successfully![/green]")
            console.print(f"📊 Results: {json.dumps(result, indent=2)}")
        else:
            console.print(f"[green]✅ Workflow completed successfully![/green]")
            console.print(f"📊 Output keys: {list(result.keys())}")
        
        # Save results to file if requested
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(result, f, indent=2)
            console.print(f"💾 Results saved to {output_file}")
        
        # Return results for programmatic use
        return result
        
    except Exception as e:
        console.print(f"[red]❌ Workflow execution failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
@otel_command
def list(
    workflows: bool = typer.Option(False, "--workflows", help="List available workflows"),
    processes: bool = typer.Option(False, "--processes", help="List processes in workflow files"),
    dmn: bool = typer.Option(False, "--dmn", help="List DMN decisions"),
    all: bool = typer.Option(False, "--all", help="List all available components")
):
    """List available workflows, processes, and DMN decisions"""
    
    if not any([workflows, processes, dmn, all]):
        console.print("[yellow]⚠️  Use --workflows, --processes, --dmn, or --all to specify what to list[/yellow]")
        return
    
    try:
        from .workflows.autotel_camunda import AutoTelCamundaEngine
        
        engine = AutoTelCamundaEngine()
        
        if workflows or all:
            # List BPMN files in bpmn directory
            bpmn_dir = Path("bpmn")
            if bpmn_dir.exists():
                bpmn_files = list(bpmn_dir.glob("*.bpmn"))
                
                table = Table(title="Available BPMN Workflows")
                table.add_column("File", style="cyan")
                table.add_column("Size", style="green")
                table.add_column("Modified", style="yellow")
                
                for bpmn_file in bpmn_files:
                    stat = bpmn_file.stat()
                    table.add_row(
                        bpmn_file.name,
                        f"{stat.st_size:,} bytes",
                        datetime.fromtimestamp(stat.st_mtime).strftime("%Y-%m-%d %H:%M")
                    )
                
                console.print(table)
            else:
                console.print("[yellow]⚠️  No bpmn directory found[/yellow]")
        
        if processes or all:
            # List processes in workflow files
            bpmn_dir = Path("bpmn")
            if bpmn_dir.exists():
                bpmn_files = list(bpmn_dir.glob("*.bpmn"))
                
                table = Table(title="Available Processes")
                table.add_column("Workflow", style="cyan")
                table.add_column("Process ID", style="green")
                table.add_column("Process Name", style="yellow")
                
                for bpmn_file in bpmn_files:
                    try:
                        engine.parser.add_bpmn_file(str(bpmn_file))
                        processes = engine.list_processes()
                        for process_id, process_name in processes.items():
                            table.add_row(bpmn_file.name, process_id, process_name)
                    except Exception as e:
                        table.add_row(bpmn_file.name, "Error", str(e))
                
                console.print(table)
        
        if dmn or all:
            # List DMN files and decisions
            bpmn_dir = Path("bpmn")
            if bpmn_dir.exists():
                dmn_files = list(bpmn_dir.glob("*.dmn"))
                
                table = Table(title="Available DMN Decisions")
                table.add_column("File", style="cyan")
                table.add_column("Decision ID", style="green")
                table.add_column("Decision Name", style="yellow")
                
                for dmn_file in dmn_files:
                    try:
                        engine.parser.add_dmn_file(str(dmn_file))
                        decisions = engine.list_dmn_decisions()
                        for decision_id, decision_name in decisions.items():
                            table.add_row(dmn_file.name, decision_id, decision_name)
                    except Exception as e:
                        table.add_row(dmn_file.name, "Error", str(e))
                
                console.print(table)
    
    except Exception as e:
        console.print(f"[red]❌ Failed to list components: {e}[/red]")
        return

@app.command()
@otel_command
def workflow(
    validate: Optional[Path] = typer.Option(None, "--validate", help="Validate BPMN workflow file"),
    info: Optional[Path] = typer.Option(None, "--info", help="Show workflow information"),
    export: Optional[Path] = typer.Option(None, "--export", help="Export workflow to different format")
):
    """Workflow management commands"""
    
    if not any([validate, info, export]):
        console.print("[yellow]⚠️  Use --validate, --info, or --export to specify action[/yellow]")
        return
    
    if validate:
        if not validate.exists():
            console.print(f"[red]❌ Workflow file not found: {validate}[/red]")
            raise typer.Exit(1)
        
        try:
            from .workflows.dspy_bpmn_parser import DspyBpmnParser
            
            parser = DspyBpmnParser()
            parser.add_bpmn_file(str(validate))
            
            specs = parser.find_all_specs()
            
            console.print(f"[green]✅ Workflow {validate.name} is valid[/green]")
            console.print(f"📊 Found {len(specs)} process(es):")
            
            for process_id, spec in specs.items():
                console.print(f"  • {process_id}: {spec.name}")
            
            # Check for DSPy signatures
            if parser.dynamic_signatures:
                console.print(f"🤖 Found {len(parser.dynamic_signatures)} DSPy signatures")
            
            # Check for DMN dependencies
            if parser.dmn_dependencies:
                console.print(f"📋 Found {len(parser.dmn_dependencies)} DMN dependencies")
        
        except Exception as e:
            console.print(f"[red]❌ Workflow validation failed: {e}[/red]")
            raise typer.Exit(1)
    
    if info:
        if not info.exists():
            console.print(f"[red]❌ Workflow file not found: {info}[/red]")
            raise typer.Exit(1)
        
        try:
            import xml.etree.ElementTree as ET
            
            tree = ET.parse(info)
            root = tree.getroot()
            
            # Extract basic information
            processes = root.findall(".//{http://www.omg.org/spec/BPMN/20100524/MODEL}process")
            
            table = Table(title=f"Workflow Information: {info.name}")
            table.add_column("Property", style="cyan")
            table.add_column("Value", style="green")
            
            table.add_row("Processes", str(len(processes)))
            
            for process in processes:
                process_id = process.get('id', 'Unknown')
                process_name = process.get('name', 'Unnamed')
                table.add_row(f"Process: {process_id}", process_name)
            
            console.print(table)
        
        except Exception as e:
            console.print(f"[red]❌ Failed to get workflow info: {e}[/red]")
            raise typer.Exit(1)

@app.command()
@otel_command
def validate(
    file_path: Path = typer.Argument(..., help="File to validate (OWL, SHACL, DSPy, BPMN, DMN, or YAML)"),
    strict: bool = typer.Option(False, "--strict", help="Enable strict validation")
):
    """Validate OWL, SHACL, DSPy, BPMN, DMN, or configuration files"""
    
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
            if file_path.suffix.lower() == '.owl':
                result = schema_validator.validate_owl_file(str(file_path))
            elif file_path.suffix.lower() == '.shacl':
                result = schema_validator.validate_shacl_file(str(file_path))
            elif file_path.suffix.lower() == '.dspy':
                result = schema_validator.validate_dspy_file(str(file_path))
            elif file_path.suffix.lower() == '.bpmn':
                # Validate BPMN using DspyBpmnParser
                from .workflows.dspy_bpmn_parser import DspyBpmnParser
                parser = DspyBpmnParser()
                parser.add_bpmn_file(str(file_path))
                specs = parser.find_all_specs()
                result = ValidationResult(valid=True, warnings=[f"Found {len(specs)} process(es)"])
            elif file_path.suffix.lower() == '.dmn':
                # Validate DMN using parser
                from .workflows.dspy_bpmn_parser import DspyBpmnParser
                parser = DspyBpmnParser()
                parser.add_dmn_file(str(file_path))
                result = ValidationResult(valid=True, warnings=[f"Found {len(parser.dmn_parsers)} decision(s)"])
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
def pipeline(
    execute: bool = typer.Option(False, "--execute", "-e", help="Execute semantic pipeline"),
    owl_file: Optional[Path] = typer.Option(None, "--owl", help="OWL ontology file"),
    shacl_file: Optional[Path] = typer.Option(None, "--shacl", help="SHACL shapes file"),
    dspy_file: Optional[Path] = typer.Option(None, "--dspy", help="DSPy signatures file"),
    input_data: Optional[Path] = typer.Option(None, "--input", "-i", help="JSON input data file"),
    input_json: Optional[str] = typer.Option(None, "--data", "-d", help="JSON input data (inline)"),
    export_telemetry: Optional[Path] = typer.Option(None, "--export-telemetry", help="Export telemetry to file"),
    quiet: bool = typer.Option(False, "--quiet", "-q", help="Suppress verbose output")
):
    """Execute the semantic execution pipeline"""
    
    if not execute:
        console.print("[yellow]⚠️  Use --execute to run the pipeline[/yellow]")
        return
    
    # Validate required files
    required_files = [owl_file, shacl_file, dspy_file]
    for file_path in required_files:
        if file_path and not file_path.exists():
            console.print(f"[red]❌ File not found: {file_path}[/red]")
            raise typer.Exit(1)
    
    # Parse input data
    input_variables = {}
    if input_data:
        if not input_data.exists():
            console.print(f"[red]❌ Input data file not found: {input_data}[/red]")
            raise typer.Exit(1)
        try:
            with open(input_data, 'r') as f:
                input_variables = json.load(f)
        except Exception as e:
            console.print(f"[red]❌ Failed to parse input data file: {e}[/red]")
            raise typer.Exit(1)
    elif input_json:
        try:
            input_variables = json.loads(input_json)
        except Exception as e:
            console.print(f"[red]❌ Failed to parse input JSON: {e}[/red]")
            raise typer.Exit(1)
    
    try:
        from autotel.factory.pipeline import PipelineOrchestrator
        
        # Initialize pipeline orchestrator
        orchestrator = PipelineOrchestrator()
        
        if not quiet:
            console.print("🚀 Starting semantic execution pipeline...")
            console.print(f"📥 Input data: {json.dumps(input_variables, indent=2)}")
        
        # Execute pipeline
        result = orchestrator.execute_from_files(
            owl_file=str(owl_file),
            shacl_file=str(shacl_file),
            dspy_file=str(dspy_file),
            inputs=input_variables
        )
        
        if not quiet:
            console.print(f"✅ Pipeline completed successfully!")
            console.print(f"📊 Outputs: {list(result.outputs.keys())}")
        else:
            # Quiet mode: just output the result
            console.print(json.dumps({
                "status": "completed",
                "outputs": result.outputs,
                "execution_metadata": result.execution_metadata
            }, indent=2))
        
        if export_telemetry:
            telemetry_data = orchestrator.telemetry.export_telemetry()
            with open(export_telemetry, 'w') as f:
                json.dump(telemetry_data, f, indent=2)
            if not quiet:
                console.print(f"📊 Telemetry exported to {export_telemetry}")
                
    except Exception as e:
        console.print(f"[red]❌ Pipeline execution failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
@otel_command
def jinja(
    process: bool = typer.Option(False, "--process", "-p", help="Process Jinja templates"),
    template_file: Optional[Path] = typer.Option(None, "--template", "-t", help="Jinja template XML file"),
    variables_file: Optional[Path] = typer.Option(None, "--variables", "-v", help="Variables JSON file"),
    variables_json: Optional[str] = typer.Option(None, "--data", "-d", help="Variables JSON (inline)"),
    output_file: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file for rendered templates"),
    validate_only: bool = typer.Option(False, "--validate", help="Only validate templates without rendering"),
    list_templates: bool = typer.Option(False, "--list", "-l", help="List available templates"),
    quiet: bool = typer.Option(False, "--quiet", "-q", help="Suppress verbose output")
):
    """Process Jinja templates from XML definitions"""
    
    if not process and not list_templates:
        console.print("[yellow]⚠️  Use --process to render templates or --list to see available templates[/yellow]")
        return
    
    # Validate template file
    if template_file and not template_file.exists():
        console.print(f"[red]❌ Template file not found: {template_file}[/red]")
        raise typer.Exit(1)
    
    # Parse variables
    variables = {}
    if variables_file:
        if not variables_file.exists():
            console.print(f"[red]❌ Variables file not found: {variables_file}[/red]")
            raise typer.Exit(1)
        try:
            with open(variables_file, 'r') as f:
                variables = json.load(f)
        except Exception as e:
            console.print(f"[red]❌ Failed to parse variables file: {e}[/red]")
            raise typer.Exit(1)
    elif variables_json:
        try:
            variables = json.loads(variables_json)
        except Exception as e:
            console.print(f"[red]❌ Failed to parse variables JSON: {e}[/red]")
            raise typer.Exit(1)
    
    try:
        from .factory.processors.jinja_processor import JinjaProcessor
        
        # Initialize Jinja processor
        processor = JinjaProcessor()
        
        if list_templates:
            if not template_file:
                console.print("[red]❌ Template file required for --list[/red]")
                raise typer.Exit(1)
            
            # Read template file
            with open(template_file, 'r') as f:
                xml_content = f.read()
            
            # Parse template definitions
            template_definitions = processor.parse_template_definitions(xml_content)
            
            # Display templates
            templates_table = Table(title="Available Jinja Templates")
            templates_table.add_column("Template", style="cyan")
            templates_table.add_column("Type", style="green")
            templates_table.add_column("Variables", style="yellow")
            templates_table.add_column("Description", style="white")
            
            for template in template_definitions.templates:
                var_count = len(template.variables)
                templates_table.add_row(
                    template.name,
                    template.template_type.value,
                    str(var_count),
                    template.description[:50] + "..." if len(template.description) > 50 else template.description
                )
            
            console.print(templates_table)
            return
        
        if process:
            if not template_file:
                console.print("[red]❌ Template file required for processing[/red]")
                raise typer.Exit(1)
            
            # Read template file
            with open(template_file, 'r') as f:
                xml_content = f.read()
            
            if not quiet:
                console.print("🚀 Processing Jinja templates...")
                console.print(f"📄 Template file: {template_file}")
                console.print(f"📊 Variables: {json.dumps(variables, indent=2)}")
            
            # Process templates
            result = processor.process_templates(xml_content, variables)
            
            if not quiet:
                console.print(f"✅ Template processing completed!")
                console.print(f"📊 Templates processed: {len(result.template_definitions.templates)}")
                console.print(f"📊 Templates rendered: {len([r for r in result.rendering_results if r.success])}")
                console.print(f"⏱️  Processing time: {result.processing_time_ms:.2f}ms")
            
            # Display validation results
            if result.validation_results:
                validation_table = Table(title="Template Validation Results")
                validation_table.add_column("Template", style="cyan")
                validation_table.add_column("Status", style="green")
                validation_table.add_column("Issues", style="yellow")
                
                for validation in result.validation_results:
                    status = "✅ Valid" if validation.valid else "❌ Invalid"
                    issues = []
                    if validation.missing_variables:
                        issues.append(f"Missing: {', '.join(validation.missing_variables)}")
                    if validation.invalid_variables:
                        issues.append(f"Invalid: {', '.join(validation.invalid_variables)}")
                    if validation.type_errors:
                        issues.append(f"Type errors: {len(validation.type_errors)}")
                    
                    issues_text = "; ".join(issues) if issues else "None"
                    validation_table.add_row(validation.template_name, status, issues_text)
                
                console.print(validation_table)
            
            # Display rendering results
            if result.rendering_results:
                rendering_table = Table(title="Template Rendering Results")
                rendering_table.add_column("Template", style="cyan")
                rendering_table.add_column("Status", style="green")
                rendering_table.add_column("Output Size", style="yellow")
                rendering_table.add_column("Time (ms)", style="blue")
                
                for rendering in result.rendering_results:
                    status = "✅ Success" if rendering.success else "❌ Failed"
                    output_size = len(rendering.rendered_content)
                    rendering_table.add_row(
                        rendering.template_name,
                        status,
                        str(output_size),
                        f"{rendering.rendering_time_ms:.2f}"
                    )
            
                console.print(rendering_table)
            
            # Output rendered content
            if output_file:
                # Combine all successful renders
                combined_output = ""
                for rendering in result.rendering_results:
                    if rendering.success:
                        combined_output += f"<!-- Template: {rendering.template_name} -->\n"
                        combined_output += rendering.rendered_content
                        combined_output += "\n\n"
                
                with open(output_file, 'w') as f:
                    f.write(combined_output)
                
                if not quiet:
                    console.print(f"📄 Output written to: {output_file}")
            else:
                # Display rendered content in console
                for rendering in result.rendering_results:
                    if rendering.success:
                        console.print(f"\n[bold cyan]Template: {rendering.template_name}[/bold cyan]")
                        console.print(Panel(rendering.rendered_content, title="Rendered Output"))
            
            # Handle errors
            if not result.success:
                console.print("[red]❌ Template processing failed[/red]")
                for error in result.errors:
                    console.print(f"  • {error}")
                raise typer.Exit(1)
                
    except Exception as e:
        console.print(f"[red]❌ Jinja processing failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
@otel_command
def otel(
    process: bool = typer.Option(False, "--process", "-p", help="Process OpenTelemetry data"),
    file: Optional[Path] = typer.Option(None, "--file", "-f", help="OTEL data file (JSON)"),
    data: Optional[str] = typer.Option(None, "--data", "-d", help="OTEL data as JSON string"),
    analyze: bool = typer.Option(False, "--analyze", "-a", help="Analyze OTEL trace data"),
    convert: bool = typer.Option(False, "--convert", "-c", help="Convert to AutoTel telemetry format"),
    output_file: Optional[Path] = typer.Option(None, "--output", "-o", help="Output file for results"),
    show_spans: bool = typer.Option(False, "--spans", help="Show span analysis"),
    show_metrics: bool = typer.Option(False, "--metrics", help="Show metric analysis"),
    show_logs: bool = typer.Option(False, "--logs", help="Show log analysis"),
    show_performance: bool = typer.Option(False, "--performance", help="Show performance analysis"),
    show_errors: bool = typer.Option(False, "--errors", help="Show error analysis"),
    show_dependencies: bool = typer.Option(False, "--dependencies", help="Show dependency analysis"),
    quiet: bool = typer.Option(False, "--quiet", "-q", help="Suppress verbose output")
):
    """Process and analyze OpenTelemetry data"""
    
    if not process and not analyze and not convert:
        console.print("[red]❌ Please specify an action: --process, --analyze, or --convert[/red]")
        raise typer.Exit(1)
    
    # Load OTEL data
    otel_data = None
    if file:
        if not file.exists():
            console.print(f"[red]❌ OTEL file not found: {file}[/red]")
            raise typer.Exit(1)
        try:
            with open(file, 'r') as f:
                otel_data = json.load(f)
        except Exception as e:
            console.print(f"[red]❌ Failed to read OTEL file: {e}[/red]")
            raise typer.Exit(1)
    elif data:
        try:
            otel_data = json.loads(data)
        except json.JSONDecodeError as e:
            console.print(f"[red]❌ Invalid JSON data: {e}[/red]")
            raise typer.Exit(1)
    else:
        console.print("[red]❌ Please provide OTEL data via --file or --data[/red]")
        raise typer.Exit(1)
    
    try:
        from .factory.pipeline import PipelineOrchestrator
        
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            
            task = progress.add_task("Processing OTEL data...", total=1)
            
            # Process OTEL data
            pipeline = PipelineOrchestrator()
            result = pipeline.process_otel_data(otel_data)
            
            progress.advance(task)
        
        # Display results based on options
        if not quiet:
            console.print(f"[green]✅ OTEL processing completed![/green]")
            console.print(f"📊 Trace ID: {result['trace_id']}")
            console.print(f"📈 Spans: {result['metadata']['spans_count']}")
            console.print(f"📊 Metrics: {result['metadata']['metrics_count']}")
            console.print(f"📝 Logs: {result['metadata']['logs_count']}")
        
        # Show specific analyses
        analysis = result['analysis']
        
        if show_spans or not any([show_spans, show_metrics, show_logs, show_performance, show_errors, show_dependencies]):
            span_analysis = analysis['span_analysis']
            table = Table(title="Span Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Spans", str(span_analysis['total_spans']))
            if span_analysis['total_spans'] > 0:
                table.add_row("Avg Duration (ms)", f"{span_analysis['avg_duration_ms']:.2f}")
                table.add_row("Min Duration (ms)", f"{span_analysis['min_duration_ms']:.2f}")
                table.add_row("Max Duration (ms)", f"{span_analysis['max_duration_ms']:.2f}")
                table.add_row("Error Rate", f"{span_analysis['error_rate']:.2%}")
            else:
                table.add_row("Avg Duration (ms)", "N/A")
                table.add_row("Min Duration (ms)", "N/A")
                table.add_row("Max Duration (ms)", "N/A")
                table.add_row("Error Rate", "N/A")
            console.print(table)
        
        if show_metrics:
            metric_analysis = analysis['metric_analysis']
            table = Table(title="Metric Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Metrics", str(metric_analysis['total_metrics']))
            table.add_row("Unique Metric Names", str(metric_analysis['unique_metric_names']))
            console.print(table)
        
        if show_logs:
            log_analysis = analysis['log_analysis']
            table = Table(title="Log Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Logs", str(log_analysis['total_logs']))
            table.add_row("Error Count", str(log_analysis['error_count']))
            table.add_row("Warning Count", str(log_analysis['warning_count']))
            table.add_row("Error Rate", f"{log_analysis['error_rate']:.2%}")
            console.print(table)
        
        if show_performance:
            perf_analysis = analysis['performance_analysis']
            table = Table(title="Performance Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Duration (ms)", f"{perf_analysis['total_duration_ms']:.2f}")
            if perf_analysis['span_count'] > 0:
                table.add_row("Avg Span Duration (ms)", f"{perf_analysis['avg_span_duration_ms']:.2f}")
                table.add_row("Min Span Duration (ms)", f"{perf_analysis['min_span_duration_ms']:.2f}")
                table.add_row("Max Span Duration (ms)", f"{perf_analysis['max_span_duration_ms']:.2f}")
            else:
                table.add_row("Avg Span Duration (ms)", "N/A")
                table.add_row("Min Span Duration (ms)", "N/A")
                table.add_row("Max Span Duration (ms)", "N/A")
            table.add_row("Span Count", str(perf_analysis['span_count']))
            console.print(table)
        
        if show_errors:
            error_analysis = analysis['error_analysis']
            table = Table(title="Error Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Error Span Count", str(error_analysis['error_span_count']))
            table.add_row("Error Log Count", str(error_analysis['error_log_count']))
            table.add_row("Error Rate", f"{error_analysis['error_rate']:.2%}")
            console.print(table)
        
        if show_dependencies:
            dep_analysis = analysis['dependency_analysis']
            table = Table(title="Dependency Analysis")
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Service Count", str(dep_analysis['service_count']))
            table.add_row("Parent-Child Relationships", str(dep_analysis['parent_child_relationships']))
            table.add_row("Max Depth", str(dep_analysis['max_depth']))
            console.print(table)
        
        # Save results to file if requested
        if output_file:
            with open(output_file, 'w') as f:
                json.dump(result, f, indent=2)
            console.print(f"💾 Results saved to {output_file}")
        
    except Exception as e:
        console.print(f"[red]❌ OTEL processing failed: {e}[/red]")
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