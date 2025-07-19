#!/usr/bin/env python3
"""
7T Engine Benchmark Runner

This script provides a comprehensive interface for running, analyzing, and reporting
benchmark results for the 7T Engine performance validation.
"""

import subprocess
import json
import time
import argparse
import sys
import os
from datetime import datetime
from typing import Dict, List, Any, Optional
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

class BenchmarkRunner:
    def __init__(self, config_file: Optional[str] = None):
        self.config = self.load_config(config_file)
        self.results = []
        self.history = []
        
    def load_config(self, config_file: Optional[str]) -> Dict[str, Any]:
        """Load benchmark configuration."""
        default_config = {
            "target_cycles": 7,
            "target_ns": 10,
            "target_ops_per_sec": 100000000,
            "iterations": {
                "basic_operation": 100000,
                "batch_operation": 10000,
                "telemetry_overhead": 50000,
                "memory_efficiency": 100000,
                "cache_performance": 50000,
                "branch_prediction": 100000,
                "simd_performance": 50000,
                "stress_test": 10000
            },
            "batch_sizes": [10, 50, 100, 500, 1000],
            "memory_limits": [1024, 4096, 8192, 16384],  # MB
            "output_formats": ["json", "csv", "html"],
            "generate_plots": True,
            "save_results": True
        }
        
        if config_file and os.path.exists(config_file):
            with open(config_file, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def run_benchmark(self, test_name: str, iterations: int, 
                     batch_size: Optional[int] = None) -> Dict[str, Any]:
        """Run a single benchmark test."""
        print(f"Running benchmark: {test_name}")
        
        # Build command
        cmd = ["./7t_benchmark_suite"]
        if batch_size:
            cmd.extend(["--batch-size", str(batch_size)])
        
        cmd.extend(["--test", test_name, "--iterations", str(iterations)])
        
        # Run benchmark
        start_time = time.time()
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            end_time = time.time()
            
            if result.returncode != 0:
                print(f"Benchmark failed: {result.stderr}")
                return None
            
            # Parse output
            benchmark_result = self.parse_benchmark_output(result.stdout, test_name)
            benchmark_result["execution_time"] = end_time - start_time
            benchmark_result["command"] = " ".join(cmd)
            
            return benchmark_result
            
        except subprocess.TimeoutExpired:
            print(f"Benchmark timed out: {test_name}")
            return None
        except Exception as e:
            print(f"Benchmark error: {e}")
            return None
    
    def parse_benchmark_output(self, output: str, test_name: str) -> Dict[str, Any]:
        """Parse benchmark output and extract metrics."""
        result = {
            "test_name": test_name,
            "timestamp": datetime.now().isoformat(),
            "raw_output": output
        }
        
        # Extract metrics from output
        lines = output.split('\n')
        for line in lines:
            if "Average cycles per operation:" in line:
                result["avg_cycles"] = float(line.split(':')[1].strip())
            elif "Average time per operation:" in line:
                result["avg_time_ns"] = float(line.split(':')[1].strip().split()[0])
            elif "Throughput:" in line:
                result["ops_per_sec"] = float(line.split(':')[1].strip().split()[0])
            elif "Operations within" in line and "%" in line:
                result["target_achievement"] = float(line.split('(')[1].split('%')[0])
            elif "Status:" in line:
                result["passed"] = "PASSED" in line
        
        return result
    
    def run_full_suite(self) -> List[Dict[str, Any]]:
        """Run the complete benchmark suite."""
        print("=== Running Full 7T Engine Benchmark Suite ===")
        
        results = []
        
        # Run all configured tests
        for test_name, iterations in self.config["iterations"].items():
            result = self.run_benchmark(test_name, iterations)
            if result:
                results.append(result)
                print(f"✓ {test_name}: {result.get('avg_cycles', 'N/A')} cycles")
        
        # Run batch size variations
        for batch_size in self.config["batch_sizes"]:
            result = self.run_benchmark("batch_operation", 1000, batch_size)
            if result:
                result["test_name"] = f"batch_operation_{batch_size}"
                results.append(result)
                print(f"✓ Batch {batch_size}: {result.get('avg_cycles', 'N/A')} cycles")
        
        self.results = results
        return results
    
    def analyze_results(self) -> Dict[str, Any]:
        """Analyze benchmark results and generate insights."""
        if not self.results:
            return {}
        
        analysis = {
            "summary": {},
            "performance_metrics": {},
            "target_achievement": {},
            "recommendations": []
        }
        
        # Calculate summary statistics
        cycles_list = [r.get("avg_cycles", 0) for r in self.results if r.get("avg_cycles")]
        times_list = [r.get("avg_time_ns", 0) for r in self.results if r.get("avg_time_ns")]
        throughput_list = [r.get("ops_per_sec", 0) for r in self.results if r.get("ops_per_sec")]
        
        if cycles_list:
            analysis["summary"]["avg_cycles"] = np.mean(cycles_list)
            analysis["summary"]["min_cycles"] = np.min(cycles_list)
            analysis["summary"]["max_cycles"] = np.max(cycles_list)
            analysis["summary"]["std_cycles"] = np.std(cycles_list)
        
        if times_list:
            analysis["summary"]["avg_time_ns"] = np.mean(times_list)
            analysis["summary"]["min_time_ns"] = np.min(times_list)
            analysis["summary"]["max_time_ns"] = np.max(times_list)
        
        if throughput_list:
            analysis["summary"]["avg_throughput"] = np.mean(throughput_list)
            analysis["summary"]["max_throughput"] = np.max(throughput_list)
        
        # Target achievement analysis
        target_achievements = [r.get("target_achievement", 0) for r in self.results if r.get("target_achievement")]
        if target_achievements:
            analysis["target_achievement"]["avg_achievement"] = np.mean(target_achievements)
            analysis["target_achievement"]["min_achievement"] = np.min(target_achievements)
            analysis["target_achievement"]["tests_passing"] = sum(1 for a in target_achievements if a >= 95.0)
            analysis["target_achievement"]["total_tests"] = len(target_achievements)
        
        # Performance recommendations
        if analysis["summary"].get("avg_cycles", 0) > self.config["target_cycles"]:
            analysis["recommendations"].append(
                "Average cycles exceed target. Consider optimizing memory access patterns."
            )
        
        if analysis["summary"].get("avg_time_ns", 0) > self.config["target_ns"]:
            analysis["recommendations"].append(
                "Average latency exceeds target. Consider SIMD optimizations."
            )
        
        if analysis["summary"].get("avg_throughput", 0) < self.config["target_ops_per_sec"]:
            analysis["recommendations"].append(
                "Throughput below target. Consider batch processing optimizations."
            )
        
        return analysis
    
    def generate_plots(self, output_dir: str = "benchmark_plots"):
        """Generate performance visualization plots."""
        if not self.results or not self.config["generate_plots"]:
            return
        
        os.makedirs(output_dir, exist_ok=True)
        
        # Prepare data
        df = pd.DataFrame(self.results)
        
        # Cycle distribution plot
        plt.figure(figsize=(12, 8))
        
        plt.subplot(2, 2, 1)
        if "avg_cycles" in df.columns:
            plt.bar(range(len(df)), df["avg_cycles"])
            plt.axhline(y=self.config["target_cycles"], color='r', linestyle='--', label=f'Target ({self.config["target_cycles"]} cycles)')
            plt.xlabel('Test')
            plt.ylabel('Average Cycles')
            plt.title('Cycle Performance by Test')
            plt.xticks(range(len(df)), df["test_name"], rotation=45)
            plt.legend()
        
        # Throughput plot
        plt.subplot(2, 2, 2)
        if "ops_per_sec" in df.columns:
            plt.bar(range(len(df)), df["ops_per_sec"])
            plt.axhline(y=self.config["target_ops_per_sec"]/1e6, color='r', linestyle='--', label=f'Target ({self.config["target_ops_per_sec"]/1e6:.0f}M ops/sec)')
            plt.xlabel('Test')
            plt.ylabel('Operations per Second (M)')
            plt.title('Throughput by Test')
            plt.xticks(range(len(df)), df["test_name"], rotation=45)
            plt.legend()
        
        # Target achievement plot
        plt.subplot(2, 2, 3)
        if "target_achievement" in df.columns:
            plt.bar(range(len(df)), df["target_achievement"])
            plt.axhline(y=95, color='r', linestyle='--', label='Target (95%)')
            plt.xlabel('Test')
            plt.ylabel('Target Achievement (%)')
            plt.title('Target Achievement by Test')
            plt.xticks(range(len(df)), df["test_name"], rotation=45)
            plt.legend()
        
        # Latency plot
        plt.subplot(2, 2, 4)
        if "avg_time_ns" in df.columns:
            plt.bar(range(len(df)), df["avg_time_ns"])
            plt.axhline(y=self.config["target_ns"], color='r', linestyle='--', label=f'Target ({self.config["target_ns"]} ns)')
            plt.xlabel('Test')
            plt.ylabel('Average Time (ns)')
            plt.title('Latency by Test')
            plt.xticks(range(len(df)), df["test_name"], rotation=45)
            plt.legend()
        
        plt.tight_layout()
        plt.savefig(f"{output_dir}/performance_summary.png", dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Plots saved to {output_dir}/")
    
    def save_results(self, output_dir: str = "benchmark_results"):
        """Save benchmark results in multiple formats."""
        if not self.results or not self.config["save_results"]:
            return
        
        os.makedirs(output_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Save as JSON
        if "json" in self.config["output_formats"]:
            json_file = f"{output_dir}/benchmark_results_{timestamp}.json"
            with open(json_file, 'w') as f:
                json.dump({
                    "config": self.config,
                    "results": self.results,
                    "analysis": self.analyze_results()
                }, f, indent=2)
            print(f"JSON results saved to {json_file}")
        
        # Save as CSV
        if "csv" in self.config["output_formats"]:
            csv_file = f"{output_dir}/benchmark_results_{timestamp}.csv"
            df = pd.DataFrame(self.results)
            df.to_csv(csv_file, index=False)
            print(f"CSV results saved to {csv_file}")
        
        # Save as HTML report
        if "html" in self.config["output_formats"]:
            html_file = f"{output_dir}/benchmark_report_{timestamp}.html"
            self.generate_html_report(html_file)
            print(f"HTML report saved to {html_file}")
    
    def generate_html_report(self, filename: str):
        """Generate an HTML report of benchmark results."""
        analysis = self.analyze_results()
        
        html_content = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <title>7T Engine Benchmark Report</title>
            <style>
                body {{ font-family: Arial, sans-serif; margin: 20px; }}
                .header {{ background-color: #f0f0f0; padding: 20px; border-radius: 5px; }}
                .summary {{ background-color: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; }}
                .warning {{ background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 10px 0; }}
                .results {{ margin: 20px 0; }}
                table {{ border-collapse: collapse; width: 100%; }}
                th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
                th {{ background-color: #f2f2f2; }}
                .passed {{ color: green; }}
                .failed {{ color: red; }}
            </style>
        </head>
        <body>
            <div class="header">
                <h1>7T Engine Benchmark Report</h1>
                <p>Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>
                <p>Target: ≤{self.config['target_cycles']} cycles, ≤{self.config['target_ns']} ns, ≥{self.config['target_ops_per_sec']/1e6:.0f}M ops/sec</p>
            </div>
            
            <div class="summary">
                <h2>Summary</h2>
                <p><strong>Average Cycles:</strong> {analysis['summary'].get('avg_cycles', 'N/A'):.2f}</p>
                <p><strong>Average Latency:</strong> {analysis['summary'].get('avg_time_ns', 'N/A'):.2f} ns</p>
                <p><strong>Average Throughput:</strong> {analysis['summary'].get('avg_throughput', 'N/A'):.0f} ops/sec</p>
                <p><strong>Target Achievement:</strong> {analysis['target_achievement'].get('avg_achievement', 'N/A'):.1f}%</p>
            </div>
            
            <div class="results">
                <h2>Detailed Results</h2>
                <table>
                    <tr>
                        <th>Test</th>
                        <th>Avg Cycles</th>
                        <th>Avg Time (ns)</th>
                        <th>Throughput (ops/sec)</th>
                        <th>Target Achievement (%)</th>
                        <th>Status</th>
                    </tr>
        """
        
        for result in self.results:
            status_class = "passed" if result.get("passed") else "failed"
            status_text = "PASSED" if result.get("passed") else "FAILED"
            
            html_content += f"""
                    <tr>
                        <td>{result['test_name']}</td>
                        <td>{result.get('avg_cycles', 'N/A'):.2f}</td>
                        <td>{result.get('avg_time_ns', 'N/A'):.2f}</td>
                        <td>{result.get('ops_per_sec', 'N/A'):.0f}</td>
                        <td>{result.get('target_achievement', 'N/A'):.1f}</td>
                        <td class="{status_class}">{status_text}</td>
                    </tr>
            """
        
        html_content += """
                </table>
            </div>
        """
        
        if analysis["recommendations"]:
            html_content += """
            <div class="warning">
                <h2>Recommendations</h2>
                <ul>
            """
            for rec in analysis["recommendations"]:
                html_content += f"<li>{rec}</li>"
            html_content += """
                </ul>
            </div>
            """
        
        html_content += """
        </body>
        </html>
        """
        
        with open(filename, 'w') as f:
            f.write(html_content)
    
    def run_continuous_monitoring(self, duration_minutes: int = 60, interval_seconds: int = 30):
        """Run continuous monitoring for performance regression detection."""
        print(f"Starting continuous monitoring for {duration_minutes} minutes...")
        
        start_time = time.time()
        end_time = start_time + (duration_minutes * 60)
        
        while time.time() < end_time:
            print(f"\n--- Monitoring run at {datetime.now().strftime('%H:%M:%S')} ---")
            
            # Run a subset of critical tests
            critical_tests = ["basic_operation", "batch_operation", "memory_efficiency"]
            run_results = []
            
            for test_name in critical_tests:
                if test_name in self.config["iterations"]:
                    result = self.run_benchmark(test_name, self.config["iterations"][test_name] // 10)
                    if result:
                        run_results.append(result)
            
            if run_results:
                self.history.append({
                    "timestamp": datetime.now().isoformat(),
                    "results": run_results
                })
                
                # Check for regressions
                self.check_regressions()
            
            time.sleep(interval_seconds)
        
        print("Continuous monitoring completed.")
    
    def check_regressions(self):
        """Check for performance regressions in continuous monitoring."""
        if len(self.history) < 2:
            return
        
        current = self.history[-1]
        previous = self.history[-2]
        
        for curr_result, prev_result in zip(current["results"], previous["results"]):
            if curr_result["test_name"] == prev_result["test_name"]:
                cycles_change = ((curr_result.get("avg_cycles", 0) - prev_result.get("avg_cycles", 0)) / 
                               prev_result.get("avg_cycles", 1)) * 100
                
                if cycles_change > 10:  # 10% regression threshold
                    print(f"⚠️  Performance regression detected in {curr_result['test_name']}: {cycles_change:.1f}% increase")

def main():
    parser = argparse.ArgumentParser(description="7T Engine Benchmark Runner")
    parser.add_argument("--config", help="Configuration file path")
    parser.add_argument("--test", help="Run specific test only")
    parser.add_argument("--iterations", type=int, help="Number of iterations")
    parser.add_argument("--continuous", type=int, help="Run continuous monitoring for N minutes")
    parser.add_argument("--output-dir", default="benchmark_results", help="Output directory")
    parser.add_argument("--no-plots", action="store_true", help="Disable plot generation")
    parser.add_argument("--no-save", action="store_true", help="Disable result saving")
    
    args = parser.parse_args()
    
    # Initialize runner
    runner = BenchmarkRunner(args.config)
    
    if args.no_plots:
        runner.config["generate_plots"] = False
    
    if args.no_save:
        runner.config["save_results"] = False
    
    # Run benchmarks
    if args.continuous:
        runner.run_continuous_monitoring(args.continuous)
    elif args.test:
        iterations = args.iterations or runner.config["iterations"].get(args.test, 10000)
        result = runner.run_benchmark(args.test, iterations)
        if result:
            print(json.dumps(result, indent=2))
    else:
        results = runner.run_full_suite()
        
        # Analyze and report
        analysis = runner.analyze_results()
        print("\n=== Analysis ===")
        print(json.dumps(analysis, indent=2))
        
        # Generate outputs
        runner.generate_plots(args.output_dir)
        runner.save_results(args.output_dir)

if __name__ == "__main__":
    main() 