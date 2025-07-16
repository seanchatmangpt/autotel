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
from autotel.factory.processors.owl_processor import OWLProcessor
from .factory.ontology_compiler import OntologyCompiler

app = typer.Typer(
    name="autotel",
    help="Enterprise BPMN 2.0 Orchestration Framework with AI-powered decision making",
    rich_markup_mode="rich"
)

console = Console()

# Global telemetry flag
NO_TELEMETRY = False

def set_no_telemetry(value: bool):
    """Set the global no-telemetry flag."""
    global NO_TELEMETRY
    NO_TELEMETRY = value

@app.callback()
def main(
    no_telemetry: bool = typer.Option(False, "--no-telemetry", help="Disable telemetry for all operations")
):
    """AutoTel - Automated Telemetry and Semantic Execution Pipeline"""
    set_no_telemetry(no_telemetry)
    if no_telemetry:
        console.print("[yellow]⚠️  Telemetry disabled[/yellow]")

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
def run(
    workflow_file: Path = typer.Argument(..., help="BPMN workflow file to execute"),
    input_data: Optional[Path] = typer.Option(None, "--input", "-i", help="JSON input data file"),
    input_json: Optional[str] = typer.Option(None, "--data", "-d", help="JSON input data (inline)"),
    export_telemetry: Optional[Path] = typer.Option(None, "--export-telemetry", help="Export telemetry to file"),
    quiet: bool = typer.Option(False, "--quiet", "-q", help="Suppress verbose output")
):
    """Execute a BPMN workflow with input data"""
    
    if not workflow_file.exists():
        console.print(f"[red]❌ Workflow file not found: {workflow_file}[/red]")
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
    
    orchestrator = Orchestrator(
        specific_bpmn_file=str(workflow_file),
        enable_telemetry=True
    )
    
    try:
        # Parse the BPMN file to get the process definition ID
        import xml.etree.ElementTree as ET
        tree = ET.parse(str(workflow_file))
        root = tree.getroot()
        # Find the first process element
        process_elem = root.find('.//{http://www.omg.org/spec/BPMN/20100524/MODEL}process')
        if process_elem is not None and 'id' in process_elem.attrib:
            process_id = process_elem.attrib['id']
        else:
            raise ValueError("Could not find process definition ID in BPMN file")
        
        if not quiet:
            console.print(f"🚀 Starting workflow: {process_id}")
            if input_variables:
                console.print(f"📥 Input data: {json.dumps(input_variables, indent=2)}")
        
        instance = orchestrator.start_process(process_id, input_variables)
        
        if not quiet:
            console.print(f"📋 Instance ID: {instance.instance_id}")
        
        result = orchestrator.execute_process(instance.instance_id)
        
        if not quiet:
            console.print(f"✅ Workflow completed with status: {result.status.value}")
        else:
            # Quiet mode: just output the result
            console.print(json.dumps({
                "status": result.status.value,
                "instance_id": instance.instance_id,
                "result": result.data if hasattr(result, 'data') else {}
            }, indent=2))
        
        if export_telemetry:
            telemetry_data = orchestrator.export_telemetry()
            with open(export_telemetry, 'w') as f:
                json.dump(telemetry_data, f, indent=2)
            if not quiet:
                console.print(f"📊 Telemetry exported to {export_telemetry}")
                
    except Exception as e:
        console.print(f"[red]❌ Workflow execution failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def list(
    directory: Optional[Path] = typer.Option(Path("."), "--directory", "-d", help="Directory to scan for workflows"),
    show_details: bool = typer.Option(False, "--details", help="Show detailed workflow information")
):
    """List available BPMN workflows"""
    
    if not directory.exists():
        console.print(f"[red]❌ Directory not found: {directory}[/red]")
        raise typer.Exit(1)
    
    # Find BPMN files
    bpmn_files = list(directory.glob("**/*.bpmn"))
    
    if not bpmn_files:
        console.print(f"[yellow]⚠️  No BPMN files found in {directory}[/yellow]")
        return
    
    if show_details:
        table = Table(title=f"Available Workflows in {directory}")
        table.add_column("File", style="cyan")
        table.add_column("Process ID", style="green")
        table.add_column("Name", style="yellow")
        table.add_column("Status", style="blue")
        
        for bpmn_file in bpmn_files:
            try:
                import xml.etree.ElementTree as ET
                tree = ET.parse(str(bpmn_file))
                root = tree.getroot()
                
                # Find process element
                process_elem = root.find('.//{http://www.omg.org/spec/BPMN/20100524/MODEL}process')
                if process_elem is not None:
                    process_id = process_elem.attrib.get('id', 'Unknown')
                    process_name = process_elem.attrib.get('name', 'Unnamed')
                    status = "✅ Valid"
                else:
                    process_id = "Unknown"
                    process_name = "Invalid BPMN"
                    status = "❌ Invalid"
                
                table.add_row(
                    str(bpmn_file.relative_to(directory)),
                    process_id,
                    process_name,
                    status
                )
            except Exception as e:
                table.add_row(
                    str(bpmn_file.relative_to(directory)),
                    "Error",
                    "Parse failed",
                    f"❌ {str(e)[:30]}..."
                )
        
        console.print(table)
    else:
        console.print(f"📋 Found {len(bpmn_files)} BPMN workflow(s) in {directory}:")
        for bpmn_file in bpmn_files:
            console.print(f"  • {bpmn_file.relative_to(directory)}")

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

ontology_app = typer.Typer(help="Ontology processing commands")

@ontology_app.command("parse")
def parse_ontology(
    file: Path = typer.Option(..., "--file", "-f", help="OWL file to parse"),
    export: Optional[Path] = typer.Option(None, "--export", "-e", help="Export parsed ontology as JSON")
):
    """Parse an OWL ontology file and print a summary."""
    if not file.exists():
        console.print(f"[red]❌ OWL file not found: {file}[/red]")
        raise typer.Exit(1)
    
    try:
        with open(file, 'r') as f:
            xml_content = f.read()
        
        console.print(f"[green]📁 Loading OWL file: {file}[/green]")
        console.print(f"[blue]📄 Loaded {len(xml_content)} characters[/blue]")
        
        # Create processor with telemetry based on global flag
        processor = OWLProcessor()
        if NO_TELEMETRY:
            from .core.telemetry import NoOpTelemetryManager
            processor.telemetry = NoOpTelemetryManager("autotel-owl-processor")
        
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            task = progress.add_task("Parsing OWL...", total=None)
            ontology_def = processor.parse_ontology_definition(xml_content, prefix='cli')
            progress.update(task, description="✅ OWL parsed successfully")
        
        console.print(f"\n[green]✅ OWL parsed successfully:[/green]")
        console.print(f"  - Classes: {len(ontology_def.classes)}")
        console.print(f"  - Object Properties: {len(ontology_def.object_properties)}")
        console.print(f"  - Data Properties: {len(ontology_def.data_properties)}")
        console.print(f"  - Individuals: {len(ontology_def.individuals)}")
        console.print(f"  - Axioms: {len(ontology_def.axioms)}")
        
        # Compile with OntologyCompiler
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console
        ) as progress:
            task = progress.add_task("Compiling ontology...", total=None)
            
            # Create compiler with telemetry based on global flag
            compiler = OntologyCompiler()
            if NO_TELEMETRY:
                from .core.telemetry import NoOpTelemetryManager
                compiler.telemetry = NoOpTelemetryManager("autotel-ontology-compiler")
            
            ontology_schema = compiler.compile(ontology_def)
            progress.update(task, description="✅ Ontology compiled successfully")
        
        console.print(f"\n[green]✅ Ontology compiled successfully:[/green]")
        console.print(f"  - Compiled Classes: {len(ontology_schema.classes)}")
        console.print(f"  - Semantic Context: {len(ontology_schema.semantic_context)} keys")
        console.print(f"  - Examples: {len(ontology_schema.examples)}")
        
        # Show semantic types breakdown
        console.print(f"\n[cyan]📊 Semantic Types Breakdown:[/cyan]")
        for semantic_type, classes in ontology_schema.semantic_context['semantic_types'].items():
            console.print(f"  - {semantic_type}: {len(classes)} classes")
            if classes:
                console.print(f"    Examples: {', '.join(classes[:3])}")
        
        # Show property types breakdown
        console.print(f"\n[cyan]🔗 Property Types Breakdown:[/cyan]")
        for prop_type, properties in ontology_schema.semantic_context['property_types'].items():
            console.print(f"  - {prop_type}: {len(properties)} properties")
            if properties:
                console.print(f"    Examples: {', '.join(properties[:3])}")
        
        # Show sample class details
        console.print(f"\n[cyan]🏗️ Sample Class Details:[/cyan]")
        for i, (class_name, class_schema) in enumerate(list(ontology_schema.classes.items())[:3]):
            console.print(f"  {i+1}. {class_name} ({class_schema.semantic_type})")
            console.print(f"     URI: {class_schema.uri}")
            console.print(f"     Properties: {len(class_schema.properties)}")
            console.print(f"     Superclasses: {len(class_schema.superclasses)}")
            console.print(f"     Description: {class_schema.description[:100]}...")
        
        # Export if requested
        if export:
            console.print(f"\n[green]💾 Exporting compiled ontology to {export}...[/green]")
            
            # Convert to dict for JSON serialization
            ontology_dict = {
                'ontology_uri': ontology_schema.ontology_uri,
                'namespace': ontology_schema.namespace,
                'prefix': ontology_schema.prefix,
                'classes': {
                    name: {
                        'name': cls.name,
                        'uri': cls.uri,
                        'semantic_type': cls.semantic_type,
                        'properties': {
                            prop_name: {
                                'name': prop.name,
                                'uri': prop.uri,
                                'data_type': prop.data_type,
                                'domain': prop.domain,
                                'range': prop.range,
                                'cardinality': prop.cardinality
                            } for prop_name, prop in cls.properties.items()
                        },
                        'superclasses': cls.superclasses,
                        'description': cls.description
                    } for name, cls in ontology_schema.classes.items()
                },
                'semantic_context': ontology_schema.semantic_context,
                'examples': ontology_schema.examples
            }
            
            with open(export, 'w') as f:
                json.dump(ontology_dict, f, indent=2)
            
            console.print(f"[green]✅ Compiled ontology exported to {export}[/green]")
        
    except Exception as e:
        console.print(f"[red]❌ Error during parsing: {e}[/red]")
        raise typer.Exit(1)

app.add_typer(ontology_app, name="ontology")

if __name__ == "__main__":
    app() 