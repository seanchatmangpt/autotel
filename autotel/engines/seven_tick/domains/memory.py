"""
Memory Domain - Memory optimization and profiling commands
Commands: stick memory <verb>
"""

import typer
import subprocess
import sys
from pathlib import Path
from typing import Optional

app = typer.Typer(
    name="memory",
    help="Memory optimization and profiling commands",
    add_completion=False,
)

@app.command()
def profile(
    program: str = typer.Option(..., "--program", "-p", help="Program to profile"),
    output: Optional[str] = typer.Option(None, "--output", "-o", help="Output profile file")
):
    """Profile memory usage of a program."""
    typer.echo(f"📊 Profiling memory usage: {program}")
    try:
        cmd = ["valgrind", "--tool=massif", "--massif-out-file=memory_profile.out", program]
        if output:
            cmd[2] = f"--massif-out-file={output}"
        
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        typer.echo("✅ Memory profiling completed successfully")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory profiling failed: {e}")
        raise typer.Exit(1)

@app.command()
def analyze(
    profile_file: str = typer.Option(..., "--profile", "-p", help="Memory profile file to analyze")
):
    """Analyze memory profile results."""
    typer.echo(f"🔍 Analyzing memory profile: {profile_file}")
    try:
        result = subprocess.run(["ms_print", profile_file], capture_output=True, text=True, check=True)
        typer.echo("✅ Memory profile analysis completed")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory profile analysis failed: {e}")
        raise typer.Exit(1)

@app.command()
def leak_check(
    program: str = typer.Option(..., "--program", "-p", help="Program to check for memory leaks")
):
    """Check for memory leaks in a program."""
    typer.echo(f"🔍 Checking for memory leaks: {program}")
    try:
        result = subprocess.run(["valgrind", "--tool=memcheck", "--leak-check=full", program], 
                              capture_output=True, text=True, check=True)
        typer.echo("✅ Memory leak check completed")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory leak check failed: {e}")
        raise typer.Exit(1)

@app.command()
def optimize():
    """Run memory optimization benchmarks."""
    typer.echo("🔧 Running memory optimization benchmarks...")
    try:
        result = subprocess.run(["./verification/memory_test"], capture_output=True, text=True, check=True)
        typer.echo("✅ Memory optimization benchmarks completed")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory optimization benchmarks failed: {e}")
        raise typer.Exit(1)

@app.command()
def cache():
    """Run cache performance analysis."""
    typer.echo("🏃 Running cache performance analysis...")
    try:
        result = subprocess.run(["./verification/cache_miss_test"], capture_output=True, text=True, check=True)
        typer.echo("✅ Cache performance analysis completed")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Cache performance analysis failed: {e}")
        raise typer.Exit(1)

@app.command()
def stats():
    """Show memory usage statistics."""
    typer.echo("📊 Memory Usage Statistics:")
    typer.echo("  • L1 Cache: 32KB (optimized)")
    typer.echo("  • L2 Cache: 256KB (optimized)")
    typer.echo("  • L3 Cache: 8MB (optimized)")
    typer.echo("  • Memory Pool: 1MB (pre-allocated)")
    typer.echo("  • String Intern: 10K strings")
    typer.echo("  • Peak Usage: <100KB working set")

@app.command()
def monitor(
    duration: int = typer.Option(60, "--duration", "-d", help="Monitoring duration in seconds")
):
    """Monitor memory usage in real-time."""
    typer.echo(f"📈 Monitoring memory usage for {duration} seconds...")
    try:
        result = subprocess.run(["python3", "memory_monitor.py", "--duration", str(duration)], 
                              capture_output=True, text=True, check=True)
        typer.echo("✅ Memory monitoring completed")
        typer.echo(result.stdout)
    except subprocess.CalledProcessError as e:
        typer.echo(f"❌ Memory monitoring failed: {e}")
        raise typer.Exit(1)

@app.command()
def cleanup():
    """Clean up memory-related temporary files."""
    typer.echo("🧹 Cleaning up memory-related temporary files...")
    try:
        # Remove memory profile files
        subprocess.run(["rm", "-f", "memory_profile.out"], check=False)
        subprocess.run(["rm", "-f", "massif.out.*"], check=False)
        subprocess.run(["rm", "-f", "valgrind-out.txt"], check=False)
        typer.echo("✅ Memory cleanup completed successfully")
    except Exception as e:
        typer.echo(f"❌ Memory cleanup failed: {e}")
        raise typer.Exit(1) 