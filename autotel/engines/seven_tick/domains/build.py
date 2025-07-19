"""
Build Domain - Compilation and build commands
Commands: stick build <verb>
"""

import typer
import subprocess
import sys
from pathlib import Path
from typing import Optional

app = typer.Typer(
    name="build",
    help="Build and compilation commands for 7T engine components",
    add_completion=False,
)

@app.command()
def clean():
    """Clean all build artifacts."""
    typer.echo("🧹 Cleaning build artifacts...")
    try:
        result = subprocess.run(["make", "clean"], capture_output=True, text=True, check=True)
        typer.echo("✅ Build artifacts cleaned successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Error cleaning build artifacts: {e}")
        raise typer.Exit(1)

@app.command()
def all():
    """Build all components."""
    typer.echo("🔨 Building all 7T engine components...")
    try:
        result = subprocess.run(["make", "all"], capture_output=True, text=True, check=True)
        typer.echo("✅ All components built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Build failed: {e}")
        typer.echo(f"Error output: {e.stderr}")
        raise typer.Exit(1)

@app.command()
def shacl():
    """Build SHACL engine components."""
    typer.echo("🔨 Building SHACL engine...")
    try:
        result = subprocess.run(["make", "shacl"], capture_output=True, text=True, check=True)
        typer.echo("✅ SHACL engine built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ SHACL build failed: {e}")
        raise typer.Exit(1)

@app.command()
def cjinja():
    """Build CJinja template engine."""
    typer.echo("🔨 Building CJinja template engine...")
    try:
        result = subprocess.run(["make", "cjinja"], capture_output=True, text=True, check=True)
        typer.echo("✅ CJinja engine built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ CJinja build failed: {e}")
        raise typer.Exit(1)

@app.command()
def sparql():
    """Build SPARQL query engine."""
    typer.echo("🔨 Building SPARQL query engine...")
    try:
        result = subprocess.run(["make", "sparql"], capture_output=True, text=True, check=True)
        typer.echo("✅ SPARQL engine built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ SPARQL build failed: {e}")
        raise typer.Exit(1)

@app.command()
def runtime():
    """Build runtime components."""
    typer.echo("🔨 Building runtime components...")
    try:
        result = subprocess.run(["make", "runtime"], capture_output=True, text=True, check=True)
        typer.echo("✅ Runtime components built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Runtime build failed: {e}")
        raise typer.Exit(1)

@app.command()
def verification():
    """Build verification and test components."""
    typer.echo("🔨 Building verification components...")
    try:
        result = subprocess.run(["make", "verification"], capture_output=True, text=True, check=True)
        typer.echo("✅ Verification components built successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Verification build failed: {e}")
        raise typer.Exit(1)

@app.command()
def production():
    """Build production-optimized version."""
    typer.echo("🔨 Building production-optimized version...")
    try:
        result = subprocess.run(["make", "production"], capture_output=True, text=True, check=True)
        typer.echo("✅ Production build completed successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Production build failed: {e}")
        raise typer.Exit(1)

@app.command()
def debug():
    """Build debug version with symbols."""
    typer.echo("🔨 Building debug version...")
    try:
        result = subprocess.run(["make", "debug"], capture_output=True, text=True, check=True)
        typer.echo("✅ Debug build completed successfully")
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Debug build failed: {e}")
        raise typer.Exit(1) 