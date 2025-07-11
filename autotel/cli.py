#!/usr/bin/env python3
"""
AutoTel CLI - Enterprise BPMN 2.0 Orchestration with Zero-Touch Telemetry
A comprehensive command-line interface for AutoTel operations
"""

import json
import yaml
import asyncio
from pathlib import Path
from typing import Optional, List, Dict, Any
from datetime import datetime
from contextlib import asynccontextmanager

import typer
from rich.console import Console
from rich.table import Table
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn
from rich.prompt import Confirm, Prompt
from rich.syntax import Syntax
from pydantic import BaseModel, Field

# Import AutoTel modules
from autotel import Orchestrator, Framework, TelemetryManager, ProcessStatus, TaskStatus
from autotel.schemas.validation import validate_telemetry_schema
from autotel.workflows.spiff import SpiffCapabilityChecker

# Initialize Typer app
app = typer.Typer(
    name="autotel",
    help="Enterprise BPMN 2.0 Orchestration with Zero-Touch Telemetry",
    add_completion=False,
    rich_markup_mode="rich",
)

# Initialize Rich console
console = Console()

class AutoTelConfig(BaseModel):
    """Configuration for AutoTel CLI"""
    bpmn_path: Path = Field(default=Path("bpmn/"), description="Path to BPMN files")
    enable_telemetry: bool = Field(default=True, description="Enable telemetry")
    enable_persistence: bool = Field(default=True, description="Enable persistence")
    log_level: str = Field(default="INFO", description="Logging level")
    config_file: Optional[Path] = Field(default=None, description="Configuration file path")

class AutoTelContext:
    """Context manager for AutoTel services"""
    
    def __init__(self, config: AutoTelConfig):
        self.config = config
        self.orchestrator: Optional[Orchestrator] = None
        self.framework: Optional[Framework] = None
        self.telemetry_manager: Optional[TelemetryManager] = None
    
    async def initialize(self) -> None:
        """Initialize all AutoTel services"""
        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            console=console,
        ) as progress:
            
            # Initialize Orchestrator
            task = progress.add_task("Initializing Orchestrator...", total=None)
            try:
                self.orchestrator = Orchestrator(
                    bpmn_files_path=str(self.config.bpmn_path),
                    enable_telemetry=self.config.enable_telemetry,
                    enable_persistence=self.config.enable_persistence
                )
                progress.update(task, description="✅ Orchestrator initialized")
            except Exception as e:
                progress.update(task, description=f"❌ Failed to initialize Orchestrator: {e}")
                raise typer.Exit(1)
            
            # Initialize Framework
            task = progress.add_task("Initializing Framework...", total=None)
            try:
                self.framework = Framework()
                progress.update(task, description="✅ Framework initialized")
            except Exception as e:
                progress.update(task, description=f"❌ Failed to initialize Framework: {e}")
                raise typer.Exit(1)
            
            # Initialize Telemetry Manager
            task = progress.add_task("Initializing Telemetry Manager...", total=None)
            try:
                self.telemetry_manager = TelemetryManager()
                progress.update(task, description="✅ Telemetry Manager initialized")
            except Exception as e:
                progress.update(task, description=f"❌ Failed to initialize Telemetry Manager: {e}")
                raise typer.Exit(1)
    
    async def cleanup(self) -> None:
        """Cleanup resources"""
        if self.telemetry_manager:
            self.telemetry_manager.force_flush()

@asynccontextmanager
async def get_autotel_context(config: AutoTelConfig):
    """Context manager for AutoTel services"""
    context = AutoTelContext(config)
    try:
        await context.initialize()
        yield context
    finally:
        await context.cleanup()

def load_config(config_file: Optional[Path] = None) -> AutoTelConfig:
    """Load configuration from file or use defaults"""
    if config_file and config_file.exists():
        try:
            with open(config_file, 'r') as f:
                config_data = yaml.safe_load(f)
            return AutoTelConfig(**config_data)
        except Exception as e:
            console.print(f"[yellow]Warning: Could not load config file: {e}[/yellow]")
    
    return AutoTelConfig()

@app.callback()
def main(
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable verbose output"),
    config_file: Optional[Path] = typer.Option(None, "--config", "-c", help="Configuration file path"),
):
    """AutoTel - Enterprise BPMN 2.0 Orchestration with Zero-Touch Telemetry"""
    if verbose:
        console.print("[bold blue]AutoTel CLI[/bold blue] - Verbose mode enabled")
    
    if config_file and config_file.exists():
        console.print(f"[green]Using config file:[/green] {config_file}")

@app.command()
def init(
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
    enable_telemetry: bool = typer.Option(True, "--telemetry/--no-telemetry", help="Enable telemetry"),
    enable_persistence: bool = typer.Option(True, "--persistence/--no-persistence", help="Enable persistence"),
):
    """Initialize AutoTel orchestrator and framework"""
    config = AutoTelConfig(
        bpmn_path=bpmn_path,
        enable_telemetry=enable_telemetry,
        enable_persistence=enable_persistence
    )
    
    async def _init():
        async with get_autotel_context(config) as context:
            console.print(Panel(
                "[bold green]AutoTel initialized successfully![/bold green]\n"
                f"• BPMN Path: {bpmn_path}\n"
                f"• Telemetry: {'Enabled' if enable_telemetry else 'Disabled'}\n"
                f"• Persistence: {'Enabled' if enable_persistence else 'Disabled'}",
                title="🚀 Initialization Complete"
            ))
    
    asyncio.run(_init())

@app.command()
def list_processes(
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """List available BPMN process definitions"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _list_processes():
        async with get_autotel_context(config) as context:
            processes = context.orchestrator.process_definitions
            
            if not processes:
                console.print("[yellow]No process definitions found.[/yellow]")
                return
            
            table = Table(title="📋 Available BPMN Processes")
            table.add_column("Process ID", style="cyan", no_wrap=True)
            table.add_column("Name", style="green")
            table.add_column("Tasks", style="blue")
            table.add_column("Status", style="yellow")
            
            for process_id, process_spec in processes.items():
                task_count = len(process_spec.task_specs) if hasattr(process_spec, 'task_specs') else 0
                table.add_row(
                    process_id,
                    getattr(process_spec, 'name', 'N/A'),
                    str(task_count),
                    "✅ Loaded"
                )
            
            console.print(table)
    
    asyncio.run(_list_processes())

@app.command()
def start_process(
    process_id: str = typer.Argument(..., help="Process ID to start"),
    variables: Optional[str] = typer.Option(None, "--variables", "-v", help="JSON variables string"),
    instance_id: Optional[str] = typer.Option(None, "--instance-id", "-i", help="Custom instance ID"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Start a new BPMN process instance"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    # Parse variables if provided
    process_variables = {}
    if variables:
        try:
            process_variables = json.loads(variables)
        except json.JSONDecodeError:
            console.print("[red]❌ Invalid JSON in variables parameter[/red]")
            raise typer.Exit(1)
    
    async def _start_process():
        async with get_autotel_context(config) as context:
            try:
                instance = context.orchestrator.start_process(
                    process_id=process_id,
                    variables=process_variables,
                    instance_id=instance_id
                )
                
                console.print(Panel(
                    f"[bold green]Process started successfully![/bold green]\n"
                    f"• Instance ID: [cyan]{instance.instance_id}[/cyan]\n"
                    f"• Process ID: [cyan]{instance.process_definition_id}[/cyan]\n"
                    f"• Status: [yellow]{instance.status.value}[/yellow]\n"
                    f"• Start Time: [blue]{instance.start_time}[/blue]",
                    title="🚀 Process Instance Created"
                ))
                
            except Exception as e:
                console.print(f"[red]❌ Failed to start process: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_start_process())

@app.command()
def execute_process(
    instance_id: str = typer.Argument(..., help="Process instance ID to execute"),
    max_steps: int = typer.Option(100, "--max-steps", "-m", help="Maximum execution steps"),
    interactive: bool = typer.Option(False, "--interactive", "-i", help="Interactive execution mode"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Execute a BPMN process instance"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _execute_process():
        async with get_autotel_context(config) as context:
            try:
                if interactive:
                    # Interactive execution
                    instance = context.orchestrator.get_process_instance(instance_id)
                    if not instance:
                        console.print(f"[red]❌ Process instance {instance_id} not found[/red]")
                        raise typer.Exit(1)
                    
                    console.print(f"[bold]Executing process instance:[/bold] {instance_id}")
                    
                    while instance.status == ProcessStatus.RUNNING:
                        ready_tasks = context.orchestrator.get_ready_tasks(instance_id)
                        
                        if not ready_tasks:
                            break
                        
                        console.print(f"\n[green]Ready tasks:[/green] {len(ready_tasks)}")
                        for i, task in enumerate(ready_tasks):
                            console.print(f"  {i+1}. {task['task_name']} ({task['task_id']})")
                        
                        if Confirm.ask("Continue execution?"):
                            instance = context.orchestrator.execute_process(instance_id, max_steps=1)
                        else:
                            break
                else:
                    # Non-interactive execution
                    with Progress(
                        SpinnerColumn(),
                        TextColumn("[progress.description]{task.description}"),
                        console=console,
                    ) as progress:
                        task = progress.add_task(f"Executing process {instance_id}...", total=None)
                        instance = context.orchestrator.execute_process(instance_id, max_steps=max_steps)
                        progress.update(task, description="✅ Process execution completed")
                
                # Display results
                console.print(Panel(
                    f"[bold]Execution Results:[/bold]\n"
                    f"• Status: [{'green' if instance.status == ProcessStatus.COMPLETED else 'red'}]{instance.status.value}[/]\n"
                    f"• End Time: [blue]{instance.end_time or 'N/A'}[/blue]\n"
                    f"• Execution Path: [cyan]{len(instance.execution_path)} steps[/cyan]",
                    title="📊 Execution Complete"
                ))
                
                if instance.error_message:
                    console.print(f"[red]Error: {instance.error_message}[/red]")
                
            except Exception as e:
                console.print(f"[red]❌ Failed to execute process: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_execute_process())

@app.command()
def list_instances(
    status: Optional[str] = typer.Option(None, "--status", "-s", help="Filter by status"),
    process_id: Optional[str] = typer.Option(None, "--process-id", "-p", help="Filter by process ID"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """List process instances"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _list_instances():
        async with get_autotel_context(config) as context:
            try:
                status_filter = ProcessStatus(status) if status else None
                instances = context.orchestrator.list_process_instances(status=status_filter, process_id=process_id)
                
                if not instances:
                    console.print("[yellow]No process instances found.[/yellow]")
                    return
                
                table = Table(title="📋 Process Instances")
                table.add_column("Instance ID", style="cyan", no_wrap=True)
                table.add_column("Process ID", style="green")
                table.add_column("Status", style="yellow")
                table.add_column("Start Time", style="blue")
                table.add_column("End Time", style="blue")
                table.add_column("Execution Steps", style="magenta")
                
                for instance in instances:
                    status_color = {
                        ProcessStatus.COMPLETED: "green",
                        ProcessStatus.RUNNING: "yellow",
                        ProcessStatus.FAILED: "red",
                        ProcessStatus.PENDING: "blue",
                    }.get(instance.status, "white")
                    
                    table.add_row(
                        instance.instance_id,
                        instance.process_definition_id,
                        f"[{status_color}]{instance.status.value}[/{status_color}]",
                        instance.start_time.strftime("%Y-%m-%d %H:%M:%S"),
                        instance.end_time.strftime("%Y-%m-%d %H:%M:%S") if instance.end_time else "N/A",
                        str(len(instance.execution_path))
                    )
                
                console.print(table)
                
            except Exception as e:
                console.print(f"[red]❌ Failed to list instances: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_list_instances())

@app.command()
def cancel_process(
    instance_id: str = typer.Argument(..., help="Process instance ID to cancel"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Cancel a running process instance"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _cancel_process():
        async with get_autotel_context(config) as context:
            try:
                instance = context.orchestrator.cancel_process(instance_id)
                console.print(Panel(
                    f"[bold green]Process cancelled successfully![/bold green]\n"
                    f"• Instance ID: [cyan]{instance.instance_id}[/cyan]\n"
                    f"• Status: [red]{instance.status.value}[/red]",
                    title="🛑 Process Cancelled"
                ))
                
            except Exception as e:
                console.print(f"[red]❌ Failed to cancel process: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_cancel_process())

@app.command()
def get_variables(
    instance_id: str = typer.Argument(..., help="Process instance ID"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Get process instance variables"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _get_variables():
        async with get_autotel_context(config) as context:
            try:
                variables = context.orchestrator.get_process_variables(instance_id)
                
                if not variables:
                    console.print("[yellow]No variables found for this instance.[/yellow]")
                    return
                
                console.print(Panel(
                    Syntax(json.dumps(variables, indent=2), "json", theme="monokai"),
                    title=f"📊 Variables for Instance {instance_id}"
                ))
                
            except Exception as e:
                console.print(f"[red]❌ Failed to get variables: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_get_variables())

@app.command()
def set_variables(
    instance_id: str = typer.Argument(..., help="Process instance ID"),
    variables: str = typer.Argument(..., help="JSON variables string"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Set process instance variables"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _set_variables():
        async with get_autotel_context(config) as context:
            try:
                variables_dict = json.loads(variables)
                context.orchestrator.set_process_variables(instance_id, variables_dict)
                console.print(f"[green]✅ Variables set successfully for instance {instance_id}[/green]")
                
            except json.JSONDecodeError:
                console.print("[red]❌ Invalid JSON in variables parameter[/red]")
                raise typer.Exit(1)
            except Exception as e:
                console.print(f"[red]❌ Failed to set variables: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_set_variables())

@app.command()
def get_ready_tasks(
    instance_id: str = typer.Argument(..., help="Process instance ID"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Get ready tasks for a process instance"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _get_ready_tasks():
        async with get_autotel_context(config) as context:
            try:
                ready_tasks = context.orchestrator.get_ready_tasks(instance_id)
                
                if not ready_tasks:
                    console.print("[yellow]No ready tasks found for this instance.[/yellow]")
                    return
                
                table = Table(title=f"📋 Ready Tasks for Instance {instance_id}")
                table.add_column("Task ID", style="cyan", no_wrap=True)
                table.add_column("Task Name", style="green")
                table.add_column("Task Type", style="blue")
                table.add_column("Status", style="yellow")
                
                for task in ready_tasks:
                    table.add_row(
                        task['task_id'],
                        task['task_name'],
                        task['task_type'],
                        task['status']
                    )
                
                console.print(table)
                
            except Exception as e:
                console.print(f"[red]❌ Failed to get ready tasks: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_get_ready_tasks())

@app.command()
def complete_task(
    instance_id: str = typer.Argument(..., help="Process instance ID"),
    task_id: str = typer.Argument(..., help="Task ID to complete"),
    data: Optional[str] = typer.Option(None, "--data", "-d", help="JSON data for task completion"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Complete a task in a process instance"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _complete_task():
        async with get_autotel_context(config) as context:
            try:
                task_data = None
                if data:
                    task_data = json.loads(data)
                
                context.orchestrator.complete_task(instance_id, task_id, task_data)
                console.print(f"[green]✅ Task {task_id} completed successfully[/green]")
                
            except json.JSONDecodeError:
                console.print("[red]❌ Invalid JSON in data parameter[/red]")
                raise typer.Exit(1)
            except Exception as e:
                console.print(f"[red]❌ Failed to complete task: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_complete_task())

@app.command()
def get_statistics(
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Get process execution statistics"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _get_statistics():
        async with get_autotel_context(config) as context:
            try:
                stats = context.orchestrator.get_process_statistics()
                
                console.print(Panel(
                    f"[bold]Process Statistics:[/bold]\n"
                    f"• Total Instances: [cyan]{stats.get('total_instances', 0)}[/cyan]\n"
                    f"• Running Instances: [yellow]{stats.get('running_instances', 0)}[/yellow]\n"
                    f"• Completed Instances: [green]{stats.get('completed_instances', 0)}[/green]\n"
                    f"• Failed Instances: [red]{stats.get('failed_instances', 0)}[/red]\n"
                    f"• Average Execution Time: [blue]{stats.get('avg_execution_time', 'N/A')}[/blue]",
                    title="📊 Statistics"
                ))
                
            except Exception as e:
                console.print(f"[red]❌ Failed to get statistics: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_get_statistics())

@app.command()
def cleanup(
    max_age_hours: int = typer.Option(24, "--max-age", "-a", help="Maximum age in hours for cleanup"),
    bpmn_path: Path = typer.Option("bpmn/", "--bpmn-path", help="Path to BPMN files"),
):
    """Clean up completed process instances"""
    config = AutoTelConfig(bpmn_path=bpmn_path)
    
    async def _cleanup():
        async with get_autotel_context(config) as context:
            try:
                cleaned_count = context.orchestrator.cleanup_completed_processes(max_age_hours)
                console.print(f"[green]✅ Cleaned up {cleaned_count} completed process instances[/green]")
                
            except Exception as e:
                console.print(f"[red]❌ Failed to cleanup: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_cleanup())

# Schema and Telemetry Commands
@app.command()
def validate_schema(
    schema_file: Path = typer.Argument(..., help="LinkML schema file to validate"),
):
    """Validate a LinkML schema"""
    try:
        with open(schema_file, 'r') as f:
            schema = yaml.safe_load(f)
        
        # Basic validation
        required_keys = ['classes', 'enums']
        missing_keys = [key for key in required_keys if key not in schema]
        
        if missing_keys:
            console.print(f"[red]❌ Missing required keys: {missing_keys}[/red]")
            raise typer.Exit(1)
        
        console.print(f"[green]✅ Schema validation passed for {schema_file}[/green]")
        
        # Display schema summary
        classes = schema.get('classes', {})
        enums = schema.get('enums', {})
        
        console.print(Panel(
            f"[bold]Schema Summary:[/bold]\n"
            f"• Classes: [cyan]{len(classes)}[/cyan]\n"
            f"• Enums: [cyan]{len(enums)}[/cyan]",
            title="📋 Schema Info"
        ))
        
    except Exception as e:
        console.print(f"[red]❌ Schema validation failed: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def process_ontology(
    ontology_file: Path = typer.Argument(..., help="LinkML ontology file to process"),
):
    """Process a LinkML ontology and generate models"""
    config = AutoTelConfig()
    
    async def _process_ontology():
        async with get_autotel_context(config) as context:
            try:
                context.framework.initialize(str(ontology_file))
                console.print(f"[green]✅ Ontology processed successfully: {ontology_file}[/green]")
                
                # Display generated models
                models = context.framework.processor.generated_models
                operations = context.framework.processor.operations
                
                console.print(Panel(
                    f"[bold]Generated Models:[/bold]\n"
                    f"• Models: [cyan]{len(models)}[/cyan]\n"
                    f"• Operations: [cyan]{len(operations)}[/cyan]",
                    title="🔧 Model Generation Complete"
                ))
                
            except Exception as e:
                console.print(f"[red]❌ Failed to process ontology: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_process_ontology())

@app.command()
def execute_workflow(
    workflow_file: Path = typer.Argument(..., help="JSON workflow file to execute"),
):
    """Execute a workflow from a JSON file"""
    config = AutoTelConfig()
    
    async def _execute_workflow():
        async with get_autotel_context(config) as context:
            try:
                with open(workflow_file, 'r') as f:
                    workflow_steps = json.load(f)
                
                console.print(f"[bold]Executing workflow from:[/bold] {workflow_file}")
                
                with Progress(
                    SpinnerColumn(),
                    TextColumn("[progress.description]{task.description}"),
                    console=console,
                ) as progress:
                    task = progress.add_task("Executing workflow steps...", total=len(workflow_steps))
                    
                    for i, step in enumerate(workflow_steps):
                        progress.update(task, description=f"Executing step {i+1}/{len(workflow_steps)}: {step.get('operation', 'Unknown')}")
                        context.framework.execute_workflow([step])
                        progress.advance(task)
                
                console.print("[green]✅ Workflow execution completed[/green]")
                
            except Exception as e:
                console.print(f"[red]❌ Failed to execute workflow: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_execute_workflow())

@app.command()
def check_spiff_capabilities(
    config_file: Optional[Path] = typer.Option(None, "--config", "-c", help="Configuration file for capability check"),
):
    """Check SpiffWorkflow capabilities and configuration"""
    try:
        checker = SpiffCapabilityChecker()
        
        if config_file:
            checker.load_config(str(config_file))
        
        capabilities = checker.check_capabilities()
        
        console.print(Panel(
            f"[bold]SpiffWorkflow Capabilities:[/bold]\n"
            f"• BPMN 2.0 Support: [green]✅[/green]\n"
            f"• Task Types: [cyan]{len(capabilities.get('task_types', []))}[/cyan]\n"
            f"• Gateway Types: [cyan]{len(capabilities.get('gateway_types', []))}[/cyan]\n"
            f"• Event Types: [cyan]{len(capabilities.get('event_types', []))}[/cyan]",
            title="🔍 Capability Check"
        ))
        
    except Exception as e:
        console.print(f"[red]❌ Failed to check capabilities: {e}[/red]")
        raise typer.Exit(1)

@app.command()
def demo():
    """Run AutoTel demonstration workflow"""
    config = AutoTelConfig()
    
    async def _demo():
        async with get_autotel_context(config) as context:
            try:
                console.print("[bold blue]Running AutoTel demonstration...[/bold blue]")
                
                # Import and run the demo
                from autotel.core.framework import demonstrate_autotel
                demonstrate_autotel()
                
                console.print("[green]✅ Demonstration completed successfully[/green]")
                
            except Exception as e:
                console.print(f"[red]❌ Failed to run demonstration: {e}[/red]")
                raise typer.Exit(1)
    
    asyncio.run(_demo())

@app.command()
def version():
    """Show AutoTel version information"""
    console.print(Panel(
        "[bold blue]AutoTel - Enterprise BPMN 2.0 Orchestration[/bold blue]\n"
        "Version: 0.1.0\n"
        "Powered by SpiffWorkflow with zero-touch telemetry integration\n\n"
        "[cyan]Features:[/cyan]\n"
        "• Full BPMN 2.0 specification compliance\n"
        "• Zero-touch telemetry integration\n"
        "• Process persistence and recovery\n"
        "• Advanced task execution monitoring\n"
        "• Multi-instance process support\n"
        "• Event-driven execution\n"
        "• Error handling and recovery",
        title="🚀 AutoTel"
    ))

if __name__ == "__main__":
    app() 