"""
Benchmark Domain - Performance testing and benchmarking commands
Commands: stick benchmark <verb>
"""

import typer
import subprocess
import sys
from pathlib import Path
from typing import Optional

app = typer.Typer(
    name="benchmark",
    help="Performance benchmarking and testing commands",
    add_completion=False,
)

@app.command()
def all():
    """Run all performance benchmarks."""
    typer.echo("🏃 Running all performance benchmarks...")
    try:
        result = subprocess.run(["make", "run-all-benchmarks"], capture_output=True, text=True, check=True)
        typer.echo("✅ All benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Benchmark run failed: {e}")
        typer.echo(f"Error output: {e.stderr}")
        raise typer.Exit(1)

@app.command()
def shacl():
    """Run SHACL performance benchmarks."""
    typer.echo("🏃 Running SHACL performance benchmarks...")
    try:
        result = subprocess.run(["make", "run-shacl-benchmark"], capture_output=True, text=True, check=True)
        typer.echo("✅ SHACL benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ SHACL benchmark failed: {e}")
        raise typer.Exit(1)

@app.command()
def cjinja():
    """Run CJinja performance benchmarks."""
    typer.echo("🏃 Running CJinja performance benchmarks...")
    try:
        result = subprocess.run(["make", "run-cjinja-benchmark"], capture_output=True, text=True, check=True)
        typer.echo("✅ CJinja benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ CJinja benchmark failed: {e}")
        raise typer.Exit(1)

@app.command()
def sparql():
    """Run SPARQL performance benchmarks."""
    typer.echo("🏃 Running SPARQL performance benchmarks...")
    try:
        result = subprocess.run(["make", "run-sparql-benchmark"], capture_output=True, text=True, check=True)
        typer.echo("✅ SPARQL benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ SPARQL benchmark failed: {e}")
        raise typer.Exit(1)

@app.command()
def memory():
    """Run memory optimization benchmarks."""
    typer.echo("🏃 Running memory optimization benchmarks...")
    try:
        result = subprocess.run(["./verification/memory_test"], capture_output=True, text=True, check=True)
        typer.echo("✅ Memory benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory benchmark failed: {e}")
        raise typer.Exit(1)

@app.command()
def cache():
    """Run cache performance benchmarks."""
    typer.echo("🏃 Running cache performance benchmarks...")
    try:
        result = subprocess.run(["./verification/cache_miss_test"], capture_output=True, text=True, check=True)
        typer.echo("✅ Cache benchmarks completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Cache benchmark failed: {e}")
        raise typer.Exit(1)

@app.command()
def regression():
    """Run performance regression tests."""
    typer.echo("🏃 Running performance regression tests...")
    try:
        result = subprocess.run(["python3", "run_benchmarks.py", "--regression"], capture_output=True, text=True, check=True)
        typer.echo("✅ Regression tests completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Regression tests failed: {e}")
        raise typer.Exit(1)

@app.command()
def report():
    """Generate performance benchmark report."""
    typer.echo("📊 Generating performance benchmark report...")
    try:
        result = subprocess.run(["python3", "run_benchmarks.py", "--report"], capture_output=True, text=True, check=True)
        typer.echo("✅ Performance report generated successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Report generation failed: {e}")
        raise typer.Exit(1)

@app.command()
def compare(
    baseline: str = typer.Option(..., "--baseline", "-b", help="Baseline benchmark file"),
    current: str = typer.Option(..., "--current", "-c", help="Current benchmark file")
):
    """Compare benchmark results between baseline and current."""
    typer.echo(f"📊 Comparing benchmarks: {baseline} vs {current}")
    try:
        result = subprocess.run(["python3", "run_benchmarks.py", "--compare", baseline, current], 
                              capture_output=True, text=True, check=True)
        typer.echo("✅ Benchmark comparison completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Benchmark comparison failed: {e}")
        raise typer.Exit(1) 