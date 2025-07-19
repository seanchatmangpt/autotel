"""
Documentation Domain - Documentation generation and management commands
Commands: stick docs <verb>
"""

import typer
import subprocess
import sys
from pathlib import Path
from typing import Optional

app = typer.Typer(
    name="docs",
    help="Documentation generation and management commands",
    add_completion=False,
)

@app.command()
def generate():
    """Generate all documentation."""
    typer.echo("📚 Generating all documentation...")
    try:
        # Generate API documentation
        subprocess.run(["python3", "generate_api_docs.py"], check=True)
        
        # Generate performance reports
        subprocess.run(["python3", "generate_performance_docs.py"], check=True)
        
        # Generate cookbook examples
        subprocess.run(["python3", "generate_cookbook_docs.py"], check=True)
        
        typer.echo("✅ All documentation generated successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Documentation generation failed: {e}")
        raise typer.Exit(1)

@app.command()
def api():
    """Generate API documentation."""
    typer.echo("📖 Generating API documentation...")
    try:
        result = subprocess.run(["python3", "generate_api_docs.py"], capture_output=True, text=True, check=True)
        typer.echo("✅ API documentation generated successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ API documentation generation failed: {e}")
        raise typer.Exit(1)

@app.command()
def performance():
    """Generate performance documentation."""
    typer.echo("📊 Generating performance documentation...")
    try:
        result = subprocess.run(["python3", "generate_performance_docs.py"], capture_output=True, text=True, check=True)
        typer.echo("✅ Performance documentation generated successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Performance documentation generation failed: {e}")
        raise typer.Exit(1)

@app.command()
def cookbook():
    """Generate cookbook documentation."""
    typer.echo("📖 Generating cookbook documentation...")
    try:
        result = subprocess.run(["python3", "generate_cookbook_docs.py"], capture_output=True, text=True, check=True)
        typer.echo("✅ Cookbook documentation generated successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Cookbook documentation generation failed: {e}")
        raise typer.Exit(1)

@app.command()
def serve(
    port: int = typer.Option(8000, "--port", "-p", help="Port to serve documentation on")
):
    """Serve documentation locally."""
    typer.echo(f"🌐 Serving documentation on port {port}...")
    try:
        result = subprocess.run(["python3", "-m", "http.server", str(port), "--directory", "docs"], 
                              capture_output=True, text=True, check=True)
        typer.echo(f"✅ Documentation server started on port {port}")
        typer.echo(f"📖 Visit: http://localhost:{port}")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Documentation server failed: {e}")
        raise typer.Exit(1)

@app.command()
def validate():
    """Validate documentation links and formatting."""
    typer.echo("🔍 Validating documentation...")
    try:
        result = subprocess.run(["python3", "validate_docs.py"], capture_output=True, text=True, check=True)
        typer.echo("✅ Documentation validation completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Documentation validation failed: {e}")
        raise typer.Exit(1)

@app.command()
def clean():
    """Clean generated documentation files."""
    typer.echo("🧹 Cleaning generated documentation...")
    try:
        # Remove generated files
        subprocess.run(["rm", "-rf", "docs/generated"], check=False)
        subprocess.run(["rm", "-f", "docs/*.html"], check=False)
        subprocess.run(["rm", "-f", "docs/*.pdf"], check=False)
        typer.echo("✅ Documentation cleanup completed successfully")
    except Exception as e:
        typer.echo(f"❌ Documentation cleanup failed: {e}")
        raise typer.Exit(1)

@app.command()
def export(
    format: str = typer.Option("html", "--format", "-f", help="Export format (html, pdf, markdown)"),
    output: str = typer.Option("docs/export", "--output", "-o", help="Output directory")
):
    """Export documentation in specified format."""
    typer.echo(f"📤 Exporting documentation in {format} format...")
    try:
        result = subprocess.run(["python3", "export_docs.py", "--format", format, "--output", output], 
                              capture_output=True, text=True, check=True)
        typer.echo(f"✅ Documentation exported to {output}")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Documentation export failed: {e}")
        raise typer.Exit(1) 