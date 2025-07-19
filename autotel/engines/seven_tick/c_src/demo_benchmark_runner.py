#!/usr/bin/env python3
"""
7T Demo Use Cases Benchmark Runner

This script provides automated execution, analysis, and reporting for the 7T demo use cases benchmark suite.
It integrates with the C benchmark framework and provides comprehensive performance analysis.
"""

import os
import sys
import json
import csv
import time
import subprocess
import argparse
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from datetime import datetime
from pathlib import Path

class DemoBenchmarkRunner:
    def __init__(self, config_file=None):
        self.config = self.load_config(config_file)
        self.results_dir = Path("demo_benchmark_results")
        self.results_dir.mkdir(exist_ok=True)
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
    def load_config(self, config_file):
        """Load configuration from file or use defaults"""
        default_config = {
            "target_cycles": 49,
            "target_ns": 1000,
            "target_fitness_min": 0.7,
            "target_fitness_max": 0.95,
            "iterations": {
                "iris_classification": 1000,
                "boston_regression": 1000,
                "digits_classification": 500,
                "breast_cancer_classification": 1000,
                "wine_quality_classification": 1000,
                "latency_benchmark": 10000,
                "throughput_benchmark": 5000,
                "fitness_quality_benchmark": 1000,
                "memory_efficiency_benchmark": 100,
                "integration_stress_benchmark": 2000
            },
            "output_formats": ["json", "csv", "html"],
            "generate_plots": True,
            "save_results": True,
            "continuous_monitoring": False,
            "monitoring_interval": 300,  # 5 minutes
            "monitoring_duration": 3600   # 1 hour
        }
        
        if config_file and os.path.exists(config_file):
            with open(config_file, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def compile_benchmark_suite(self):
        """Compile the demo benchmark suite"""
        print("Compiling demo benchmark suite...")
        
        # Check if we're in the right directory
        if not os.path.exists("demo_benchmark_suite.c"):
            print("Error: demo_benchmark_suite.c not found. Please run from c_src directory.")
            return False
        
        # Compile command
        compile_cmd = [
            "gcc", "-O3", "-march=native", "-mtune=native",
            "-ffast-math", "-funroll-loops", "-fomit-frame-pointer",
            "-I../runtime/src",
            "-o", "demo_benchmark_suite",
            "demo_benchmark_suite.c",
            "demo_benchmark_framework.c",
            "../runtime/src/seven_t_runtime.c",
            "-lm"
        ]
        
        try:
            result = subprocess.run(compile_cmd, capture_output=True, text=True)
            if result.returncode == 0:
                print("✅ Demo benchmark suite compiled successfully")
                return True
            else:
                print(f"❌ Compilation failed: {result.stderr}")
                return False
        except Exception as e:
            print(f"❌ Compilation error: {e}")
            return False
    
    def run_benchmark_suite(self):
        """Run the demo benchmark suite"""
        print("Running demo benchmark suite...")
        
        if not os.path.exists("demo_benchmark_suite"):
            print("Error: demo_benchmark_suite executable not found. Please compile first.")
            return None
        
        try:
            result = subprocess.run(["./demo_benchmark_suite"], 
                                  capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                print("✅ Demo benchmark suite completed successfully")
                return result.stdout
            else:
                print(f"❌ Benchmark execution failed: {result.stderr}")
                return None
        except subprocess.TimeoutExpired:
            print("❌ Benchmark execution timed out")
            return None
        except Exception as e:
            print(f"❌ Benchmark execution error: {e}")
            return None
    
    def parse_benchmark_output(self, output):
        """Parse benchmark output and extract results"""
        if not output:
            return None
        
        results = {
            "timestamp": self.timestamp,
            "config": self.config,
            "tests": [],
            "summary": {}
        }
        
        lines = output.split('\n')
        current_test = None
        
        for line in lines:
            line = line.strip()
            
            # Parse test results
            if line.startswith("=== Demo Benchmark Result:"):
                test_name = line.split(":", 1)[1].strip()
                current_test = {"test_name": test_name, "metrics": {}}
                results["tests"].append(current_test)
            
            elif current_test and ":" in line:
                key, value = line.split(":", 1)
                key = key.strip()
                value = value.strip()
                
                # Parse numeric values
                if "cycles" in key.lower() or "ns" in key.lower() or "ops/sec" in key.lower():
                    try:
                        if "ops/sec" in key.lower():
                            value = float(value.replace(",", ""))
                        else:
                            value = float(value)
                    except:
                        pass
                
                current_test["metrics"][key] = value
            
            # Parse summary
            elif line.startswith("Overall Performance Score:"):
                results["summary"]["performance_score"] = float(line.split(":")[1].strip().replace("%", ""))
            elif line.startswith("Overall Fitness Score:"):
                results["summary"]["fitness_score"] = float(line.split(":")[1].strip())
            elif line.startswith("Tests Passed:"):
                passed_info = line.split(":")[1].strip()
                passed, total = passed_info.split("/")[0], passed_info.split("/")[1].split()[0]
                results["summary"]["tests_passed"] = int(passed)
                results["summary"]["total_tests"] = int(total)
        
        return results
    
    def save_results(self, results):
        """Save benchmark results to files"""
        if not results or not self.config["save_results"]:
            return
        
        base_filename = f"demo_benchmark_results_{self.timestamp}"
        
        # Save JSON results
        if "json" in self.config["output_formats"]:
            json_file = self.results_dir / f"{base_filename}.json"
            with open(json_file, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"✅ Results saved to {json_file}")
        
        # Save CSV results
        if "csv" in self.config["output_formats"]:
            csv_file = self.results_dir / f"{base_filename}.csv"
            self.save_csv_results(results, csv_file)
            print(f"✅ Results saved to {csv_file}")
        
        # Generate HTML report
        if "html" in self.config["output_formats"]:
            html_file = self.results_dir / f"{base_filename}.html"
            self.generate_html_report(results, html_file)
            print(f"✅ HTML report saved to {html_file}")
        
        # Generate plots
        if self.config["generate_plots"]:
            plots_dir = self.results_dir / "plots"
            plots_dir.mkdir(exist_ok=True)
            self.generate_plots(results, plots_dir)
            print(f"✅ Plots saved to {plots_dir}")
    
    def save_csv_results(self, results, csv_file):
        """Save results in CSV format"""
        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            
            # Write header
            writer.writerow([
                "Test Name", "Use Case", "Iterations", "Total Time (ns)", "Total Cycles",
                "Avg Cycles/Iteration", "Avg Time/Iteration (ns)", "Throughput (ops/sec)",
                "P50 Cycles", "P95 Cycles", "P99 Cycles", "Min Cycles", "Max Cycles",
                "Target Achievement (%)", "Fitness Achievement (%)", "Overall Fitness", "Status"
            ])
            
            # Write test results
            for test in results["tests"]:
                metrics = test["metrics"]
                writer.writerow([
                    test["test_name"],
                    metrics.get("Use Case", ""),
                    metrics.get("Iterations", ""),
                    metrics.get("Total cycles", ""),
                    metrics.get("Total time", ""),
                    metrics.get("Average cycles per iteration", ""),
                    metrics.get("Average time per iteration", ""),
                    metrics.get("Throughput", ""),
                    metrics.get("P50", ""),
                    metrics.get("P95", ""),
                    metrics.get("P99", ""),
                    metrics.get("Min", ""),
                    metrics.get("Max", ""),
                    metrics.get("Target Achievement", ""),
                    metrics.get("Fitness Achievement", ""),
                    metrics.get("Overall fitness", ""),
                    "PASS" if metrics.get("Status", "").startswith("✅") else "FAIL"
                ])
    
    def generate_html_report(self, results, html_file):
        """Generate HTML report"""
        html_content = f"""
<!DOCTYPE html>
<html>
<head>
    <title>7T Demo Use Cases Benchmark Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        .header {{ background-color: #f0f0f0; padding: 20px; border-radius: 5px; }}
        .summary {{ background-color: #e8f5e8; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .test-result {{ border: 1px solid #ddd; margin: 10px 0; padding: 15px; border-radius: 5px; }}
        .pass {{ border-left: 5px solid #4CAF50; }}
        .fail {{ border-left: 5px solid #f44336; }}
        .metrics {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }}
        .metric {{ background-color: #f9f9f9; padding: 10px; border-radius: 3px; }}
        .metric-label {{ font-weight: bold; color: #666; }}
        .metric-value {{ font-size: 1.2em; color: #333; }}
    </style>
</head>
<body>
    <div class="header">
        <h1>7T Demo Use Cases Benchmark Report</h1>
        <p>Generated: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</p>
        <p>Framework Version: {self.config.get('version', '2.0.0')}</p>
    </div>
    
    <div class="summary">
        <h2>Summary</h2>
        <div class="metrics">
            <div class="metric">
                <div class="metric-label">Performance Score</div>
                <div class="metric-value">{results['summary'].get('performance_score', 0):.1f}%</div>
            </div>
            <div class="metric">
                <div class="metric-label">Fitness Score</div>
                <div class="metric-value">{results['summary'].get('fitness_score', 0):.4f}</div>
            </div>
            <div class="metric">
                <div class="metric-label">Tests Passed</div>
                <div class="metric-value">{results['summary'].get('tests_passed', 0)}/{results['summary'].get('total_tests', 0)}</div>
            </div>
        </div>
    </div>
    
    <h2>Test Results</h2>
"""
        
        for test in results["tests"]:
            metrics = test["metrics"]
            status_class = "pass" if metrics.get("Status", "").startswith("✅") else "fail"
            
            html_content += f"""
    <div class="test-result {status_class}">
        <h3>{test['test_name']}</h3>
        <div class="metrics">
"""
            
            for key, value in metrics.items():
                if key != "Test Name":
                    html_content += f"""
            <div class="metric">
                <div class="metric-label">{key}</div>
                <div class="metric-value">{value}</div>
            </div>
"""
            
            html_content += """
        </div>
    </div>
"""
        
        html_content += """
</body>
</html>
"""
        
        with open(html_file, 'w') as f:
            f.write(html_content)
    
    def generate_plots(self, results, plots_dir):
        """Generate performance plots"""
        if not results["tests"]:
            return
        
        # Prepare data for plotting
        test_names = []
        avg_cycles = []
        avg_times = []
        fitness_scores = []
        target_achievement = []
        
        for test in results["tests"]:
            metrics = test["metrics"]
            test_names.append(test["test_name"])
            avg_cycles.append(float(metrics.get("Average cycles per iteration", 0)))
            avg_times.append(float(metrics.get("Average time per iteration", 0)))
            fitness_scores.append(float(metrics.get("Overall fitness", 0)))
            target_achievement.append(float(metrics.get("Target Achievement", "0").replace("%", "")))
        
        # Create subplots
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('7T Demo Use Cases Benchmark Results', fontsize=16)
        
        # Plot 1: Average Cycles per Iteration
        bars1 = ax1.bar(range(len(test_names)), avg_cycles, color='skyblue')
        ax1.axhline(y=self.config["target_cycles"], color='red', linestyle='--', label=f'Target ({self.config["target_cycles"]} cycles)')
        ax1.set_title('Average Cycles per Iteration')
        ax1.set_ylabel('Cycles')
        ax1.set_xticks(range(len(test_names)))
        ax1.set_xticklabels(test_names, rotation=45, ha='right')
        ax1.legend()
        
        # Plot 2: Average Time per Iteration
        bars2 = ax2.bar(range(len(test_names)), avg_times, color='lightgreen')
        ax2.axhline(y=self.config["target_ns"], color='red', linestyle='--', label=f'Target ({self.config["target_ns"]} ns)')
        ax2.set_title('Average Time per Iteration')
        ax2.set_ylabel('Time (ns)')
        ax2.set_xticks(range(len(test_names)))
        ax2.set_xticklabels(test_names, rotation=45, ha='right')
        ax2.legend()
        
        # Plot 3: Fitness Scores
        bars3 = ax3.bar(range(len(test_names)), fitness_scores, color='gold')
        ax3.axhline(y=self.config["target_fitness_min"], color='red', linestyle='--', label=f'Min Target ({self.config["target_fitness_min"]})')
        ax3.axhline(y=self.config["target_fitness_max"], color='red', linestyle='--', label=f'Max Target ({self.config["target_fitness_max"]})')
        ax3.set_title('Fitness Scores')
        ax3.set_ylabel('Fitness Score')
        ax3.set_xticks(range(len(test_names)))
        ax3.set_xticklabels(test_names, rotation=45, ha='right')
        ax3.legend()
        
        # Plot 4: Target Achievement
        bars4 = ax4.bar(range(len(test_names)), target_achievement, color='lightcoral')
        ax4.axhline(y=95, color='red', linestyle='--', label='Target (95%)')
        ax4.set_title('Target Achievement')
        ax4.set_ylabel('Achievement (%)')
        ax4.set_xticks(range(len(test_names)))
        ax4.set_xticklabels(test_names, rotation=45, ha='right')
        ax4.legend()
        
        plt.tight_layout()
        plt.savefig(plots_dir / 'benchmark_results.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def run_continuous_monitoring(self):
        """Run continuous monitoring"""
        if not self.config["continuous_monitoring"]:
            return
        
        print(f"Starting continuous monitoring for {self.config['monitoring_duration']} seconds...")
        
        start_time = time.time()
        end_time = start_time + self.config["monitoring_duration"]
        interval = self.config["monitoring_interval"]
        
        monitoring_results = []
        
        while time.time() < end_time:
            print(f"\n--- Monitoring cycle at {datetime.now().strftime('%H:%M:%S')} ---")
            
            # Run benchmark
            output = self.run_benchmark_suite()
            if output:
                results = self.parse_benchmark_output(output)
                if results:
                    monitoring_results.append(results)
                    print(f"✅ Monitoring cycle completed. Performance score: {results['summary'].get('performance_score', 0):.1f}%")
            
            # Wait for next cycle
            if time.time() < end_time:
                time.sleep(interval)
        
        # Save monitoring results
        if monitoring_results:
            monitoring_file = self.results_dir / f"continuous_monitoring_{self.timestamp}.json"
            with open(monitoring_file, 'w') as f:
                json.dump(monitoring_results, f, indent=2)
            print(f"✅ Continuous monitoring results saved to {monitoring_file}")
    
    def run(self):
        """Main execution method"""
        print("=== 7T Demo Use Cases Benchmark Runner ===")
        print(f"Configuration: {json.dumps(self.config, indent=2)}")
        print()
        
        # Compile benchmark suite
        if not self.compile_benchmark_suite():
            return 1
        
        # Run continuous monitoring if enabled
        if self.config["continuous_monitoring"]:
            self.run_continuous_monitoring()
            return 0
        
        # Run single benchmark execution
        output = self.run_benchmark_suite()
        if not output:
            return 1
        
        # Parse and save results
        results = self.parse_benchmark_output(output)
        if results:
            self.save_results(results)
            
            # Print summary
            print("\n=== Benchmark Summary ===")
            print(f"Performance Score: {results['summary'].get('performance_score', 0):.1f}%")
            print(f"Fitness Score: {results['summary'].get('fitness_score', 0):.4f}")
            print(f"Tests Passed: {results['summary'].get('tests_passed', 0)}/{results['summary'].get('total_tests', 0)}")
            
            return 0
        else:
            print("❌ Failed to parse benchmark results")
            return 1

def main():
    parser = argparse.ArgumentParser(description="7T Demo Use Cases Benchmark Runner")
    parser.add_argument("--config", help="Configuration file path")
    parser.add_argument("--continuous", type=int, help="Run continuous monitoring for N seconds")
    parser.add_argument("--output-dir", help="Output directory for results")
    parser.add_argument("--no-plots", action="store_true", help="Disable plot generation")
    parser.add_argument("--no-save", action="store_true", help="Disable result saving")
    
    args = parser.parse_args()
    
    # Create runner
    runner = DemoBenchmarkRunner(args.config)
    
    # Override config with command line arguments
    if args.continuous:
        runner.config["continuous_monitoring"] = True
        runner.config["monitoring_duration"] = args.continuous
    
    if args.output_dir:
        runner.results_dir = Path(args.output_dir)
        runner.results_dir.mkdir(exist_ok=True)
    
    if args.no_plots:
        runner.config["generate_plots"] = False
    
    if args.no_save:
        runner.config["save_results"] = False
    
    # Run benchmark
    return runner.run()

if __name__ == "__main__":
    sys.exit(main()) 