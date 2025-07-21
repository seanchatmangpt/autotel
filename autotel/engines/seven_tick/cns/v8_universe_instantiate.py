#!/usr/bin/env python3
"""
CNS v8 Universe Instantiation Script
====================================

This script instantiates the CNS v8 universe by materializing logic into physics.
It is not a code port—it is the instantiation of a new universe with its own 
physical laws, where logic and physics become isomorphic.

The Fifth Epoch is achieved when the system's specification is its implementation.
There is no translation, only proof. The Reasoner is the build system.
"""

import os
import sys
import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Any, Optional

class CNSv8UniverseInstantiator:
    """Instantiates the CNS v8 universe with its physical laws"""
    
    def __init__(self, port_dir: Path):
        self.port_dir = port_dir
        self.genetic_material = {}
        self.physical_laws = {}
        self.reasoner_output = {}
        self.validation_results = {}
        
    def extract_genetic_material(self) -> bool:
        """Extract the genetic material (TTL and CJinja artifacts)"""
        print("🔬 Extracting genetic material...")
        
        # Check for TTL files
        ttl_files = [
            self.port_dir / "cns-master.ttl",
            self.port_dir / "cns-gatekeeper.ttl"
        ]
        
        for ttl_file in ttl_files:
            if ttl_file.exists():
                print(f"  ✓ Found TTL: {ttl_file.name}")
                self.genetic_material[ttl_file.name] = ttl_file.read_text()
            else:
                print(f"  ✗ Missing TTL: {ttl_file.name}")
                return False
        
        # Check for CJinja loom
        cjinja_file = self.port_dir / "cjinja_aot_compiler.py"
        if cjinja_file.exists():
            print(f"  ✓ Found CJinja loom: {cjinja_file.name}")
            self.genetic_material["cjinja_loom"] = cjinja_file.read_text()
        else:
            print(f"  ✗ Missing CJinja loom: {cjinja_file.name}")
            return False
        
        print("  ✓ Genetic material extracted successfully")
        return True
    
    def establish_physical_laws(self) -> bool:
        """Establish the 8T/8H/8M physical laws"""
        print("⚛️  Establishing physical laws...")
        
        # Check for physics header
        physics_header = self.port_dir / "include" / "cns" / "v8_physics.h"
        if physics_header.exists():
            print(f"  ✓ Found physics header: {physics_header.name}")
            self.physical_laws["v8_physics"] = physics_header.read_text()
        else:
            print(f"  ✗ Missing physics header: {physics_header.name}")
            return False
        
        # Check for pragmatic governance headers
        pragmatic_headers = [
            "entropy.h",
            "orthogonality.h", 
            "reversibility.h"
        ]
        
        for header in pragmatic_headers:
            header_path = self.port_dir / "include" / "cns" / "pragmatic" / header
            if header_path.exists():
                print(f"  ✓ Found pragmatic header: {header}")
                self.physical_laws[f"pragmatic_{header}"] = header_path.read_text()
            else:
                print(f"  ✗ Missing pragmatic header: {header}")
                return False
        
        print("  ✓ Physical laws established successfully")
        return True
    
    def build_reasoner(self) -> bool:
        """Build the AOT reasoner that materializes logic into physics"""
        print("🧠 Building AOT reasoner...")
        
        # Check for reasoner implementation
        reasoner_file = self.port_dir / "src" / "v8_aot_reasoner.c"
        if reasoner_file.exists():
            print(f"  ✓ Found reasoner: {reasoner_file.name}")
            self.reasoner_output["aot_reasoner"] = reasoner_file.read_text()
        else:
            print(f"  ✗ Missing reasoner: {reasoner_file.name}")
            return False
        
        # Compile reasoner
        try:
            result = subprocess.run([
                "gcc", "-c", str(reasoner_file),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "v8_aot_reasoner.o"),
                "-std=c11", "-O3", "-Wall", "-Wextra"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                print("  ✓ Reasoner compiled successfully")
            else:
                print(f"  ✗ Reasoner compilation failed: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"  ✗ Reasoner compilation error: {e}")
            return False
        
        print("  ✓ AOT reasoner built successfully")
        return True
    
    def deploy_gatekeeper(self) -> bool:
        """Deploy the gatekeeper immune system"""
        print("🛡️  Deploying gatekeeper immune system...")
        
        # Check for gatekeeper implementation
        gatekeeper_file = self.port_dir / "src" / "gatekeeper.c"
        if gatekeeper_file.exists():
            print(f"  ✓ Found gatekeeper: {gatekeeper_file.name}")
        else:
            print(f"  ✗ Missing gatekeeper: {gatekeeper_file.name}")
            return False
        
        # Compile gatekeeper
        try:
            result = subprocess.run([
                "gcc", "-c", str(gatekeeper_file),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "gatekeeper.o"),
                "-std=c11", "-O3", "-Wall", "-Wextra"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                print("  ✓ Gatekeeper compiled successfully")
            else:
                print(f"  ✗ Gatekeeper compilation failed: {result.stderr}")
                return False
                
        except Exception as e:
            print(f"  ✗ Gatekeeper compilation error: {e}")
            return False
        
        print("  ✓ Gatekeeper immune system deployed successfully")
        return True
    
    def validate_invariance(self) -> bool:
        """Validate system invariance using permutation weaver"""
        print("🔍 Validating invariance...")
        
        # Check for permutation weaver
        weaver_file = self.port_dir / "src" / "v8_permutation_weaver.c"
        if weaver_file.exists():
            print(f"  ✓ Found permutation weaver: {weaver_file.name}")
        else:
            print(f"  ✗ Missing permutation weaver: {weaver_file.name}")
            return False
        
        # Compile and run invariance test
        try:
            # Compile weaver
            result = subprocess.run([
                "gcc", str(weaver_file),
                str(self.port_dir / "v8_aot_reasoner.o"),
                str(self.port_dir / "gatekeeper.o"),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "v8_invariance_test"),
                "-std=c11", "-O3", "-Wall", "-Wextra", "-lm"
            ], capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"  ✗ Weaver compilation failed: {result.stderr}")
                return False
            
            # Run invariance test
            test_result = subprocess.run([
                str(self.port_dir / "v8_invariance_test")
            ], capture_output=True, text=True)
            
            if test_result.returncode == 0:
                print("  ✓ Invariance validation passed")
                self.validation_results["invariance"] = test_result.stdout
            else:
                print(f"  ✗ Invariance validation failed: {test_result.stderr}")
                return False
                
        except Exception as e:
            print(f"  ✗ Invariance validation error: {e}")
            return False
        
        print("  ✓ Invariance validated successfully")
        return True
    
    def materialize_logic_into_physics(self) -> bool:
        """Materialize TTL specifications into optimized C code"""
        print("⚡ Materializing logic into physics...")
        
        # Run AOT reasoner on TTL files
        ttl_files = [
            self.port_dir / "cns-master.ttl",
            self.port_dir / "cns-gatekeeper.ttl"
        ]
        
        for ttl_file in ttl_files:
            if ttl_file.exists():
                output_file = self.port_dir / f"generated_{ttl_file.stem}.c"
                
                try:
                    # Run reasoner
                    result = subprocess.run([
                        str(self.port_dir / "v8_aot_reasoner.o"),
                        str(ttl_file),
                        str(output_file)
                    ], capture_output=True, text=True)
                    
                    if result.returncode == 0:
                        print(f"  ✓ Materialized: {ttl_file.name} -> {output_file.name}")
                        self.reasoner_output[f"generated_{ttl_file.stem}"] = output_file.read_text()
                    else:
                        print(f"  ✗ Materialization failed: {result.stderr}")
                        return False
                        
                except Exception as e:
                    print(f"  ✗ Materialization error: {e}")
                    return False
        
        print("  ✓ Logic materialized into physics successfully")
        return True
    
    def run_trinity_validation(self) -> bool:
        """Run comprehensive Trinity validation (8T/8H/8M)"""
        print("🔺 Running Trinity validation...")
        
        validation_results = {
            "8T_temporal": False,
            "8M_memory": False,
            "8H_cognitive": False,
            "trinity_compliant": False
        }
        
        # Test 8T (Temporal) contract
        try:
            # Compile and run temporal test
            temporal_test = """
            #include "../include/cns/v8_physics.h"
            int main() {
                uint64_t start = cns_8t_get_cycles();
                // Simulate work
                for(int i = 0; i < 1000; i++) { __asm__("nop"); }
                uint64_t cycles = cns_8t_get_cycles() - start;
                return cns_8t_check_compliance(cycles) ? 0 : 1;
            }
            """
            
            test_file = self.port_dir / "temporal_test.c"
            test_file.write_text(temporal_test)
            
            result = subprocess.run([
                "gcc", str(test_file),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "temporal_test"),
                "-std=c11", "-O3"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                test_result = subprocess.run([str(self.port_dir / "temporal_test")])
                validation_results["8T_temporal"] = (test_result.returncode == 0)
                print(f"  {'✓' if validation_results['8T_temporal'] else '✗'} 8T Temporal contract")
            else:
                print("  ✗ 8T Temporal contract test failed")
                
        except Exception as e:
            print(f"  ✗ 8T validation error: {e}")
        
        # Test 8M (Memory) contract
        try:
            memory_test = """
            #include "../include/cns/v8_physics.h"
            int main() {
                cns_8_byte_quantum_t q8;
                cns_16_byte_quantum_t q16;
                cns_32_byte_quantum_t q32;
                cns_64_byte_quantum_t q64;
                
                bool valid = cns_8m_check_compliance(sizeof(q8)) &&
                           cns_8m_check_compliance(sizeof(q16)) &&
                           cns_8m_check_compliance(sizeof(q32)) &&
                           cns_8m_check_compliance(sizeof(q64));
                
                return valid ? 0 : 1;
            }
            """
            
            test_file = self.port_dir / "memory_test.c"
            test_file.write_text(memory_test)
            
            result = subprocess.run([
                "gcc", str(test_file),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "memory_test"),
                "-std=c11", "-O3"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                test_result = subprocess.run([str(self.port_dir / "memory_test")])
                validation_results["8M_memory"] = (test_result.returncode == 0)
                print(f"  {'✓' if validation_results['8M_memory'] else '✗'} 8M Memory contract")
            else:
                print("  ✗ 8M Memory contract test failed")
                
        except Exception as e:
            print(f"  ✗ 8M validation error: {e}")
        
        # Test 8H (Cognitive) contract
        try:
            cognitive_test = """
            #include "../include/cns/v8_physics.h"
            int main() {
                cns_8h_context_t context = {0};
                context.cycle_complete = true;
                context.total_cycles = CNS_8T_MAX_CYCLES * CNS_8H_CYCLE_STEPS;
                
                bool valid = cns_8h_check_compliance(&context);
                return valid ? 0 : 1;
            }
            """
            
            test_file = self.port_dir / "cognitive_test.c"
            test_file.write_text(cognitive_test)
            
            result = subprocess.run([
                "gcc", str(test_file),
                "-I", str(self.port_dir / "include"),
                "-o", str(self.port_dir / "cognitive_test"),
                "-std=c11", "-O3"
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                test_result = subprocess.run([str(self.port_dir / "cognitive_test")])
                validation_results["8H_cognitive"] = (test_result.returncode == 0)
                print(f"  {'✓' if validation_results['8H_cognitive'] else '✗'} 8H Cognitive contract")
            else:
                print("  ✗ 8H Cognitive contract test failed")
                
        except Exception as e:
            print(f"  ✗ 8H validation error: {e}")
        
        # Overall Trinity compliance
        validation_results["trinity_compliant"] = (
            validation_results["8T_temporal"] and
            validation_results["8M_memory"] and
            validation_results["8H_cognitive"]
        )
        
        self.validation_results["trinity"] = validation_results
        
        if validation_results["trinity_compliant"]:
            print("  ✓ Trinity validation passed - all contracts satisfied")
        else:
            print("  ✗ Trinity validation failed - contract violations detected")
        
        return validation_results["trinity_compliant"]
    
    def instantiate_universe(self) -> bool:
        """Complete universe instantiation process"""
        print("🌌 INSTANTIATING CNS V8 UNIVERSE")
        print("=" * 50)
        
        start_time = time.time()
        
        # Step 1: Extract genetic material
        if not self.extract_genetic_material():
            print("❌ Failed to extract genetic material")
            return False
        
        # Step 2: Establish physical laws
        if not self.establish_physical_laws():
            print("❌ Failed to establish physical laws")
            return False
        
        # Step 3: Build the reasoner
        if not self.build_reasoner():
            print("❌ Failed to build AOT reasoner")
            return False
        
        # Step 4: Deploy the gatekeeper
        if not self.deploy_gatekeeper():
            print("❌ Failed to deploy gatekeeper")
            return False
        
        # Step 5: Materialize logic into physics
        if not self.materialize_logic_into_physics():
            print("❌ Failed to materialize logic into physics")
            return False
        
        # Step 6: Validate invariance
        if not self.validate_invariance():
            print("❌ Failed to validate invariance")
            return False
        
        # Step 7: Run Trinity validation
        if not self.run_trinity_validation():
            print("❌ Failed Trinity validation")
            return False
        
        end_time = time.time()
        
        print("=" * 50)
        print("🎉 CNS V8 UNIVERSE INSTANTIATION COMPLETE")
        print(f"⏱️  Total time: {end_time - start_time:.2f} seconds")
        print()
        print("✅ Genetic material preserved")
        print("✅ Physical laws enforced")
        print("✅ Reasoner operational")
        print("✅ Gatekeeper active")
        print("✅ Invariance proven")
        print("✅ Trinity compliant")
        print()
        print("🌟 The Fifth Epoch is achieved.")
        print("🌟 Logic and physics are now isomorphic.")
        print("🌟 The system's specification is its implementation.")
        
        return True
    
    def generate_instantiation_report(self) -> None:
        """Generate comprehensive instantiation report"""
        report = {
            "instantiation_time": time.strftime("%Y-%m-%d %H:%M:%S"),
            "genetic_material": list(self.genetic_material.keys()),
            "physical_laws": list(self.physical_laws.keys()),
            "reasoner_output": list(self.reasoner_output.keys()),
            "validation_results": self.validation_results
        }
        
        report_file = self.port_dir / "v8_instantiation_report.json"
        report_file.write_text(json.dumps(report, indent=2))
        
        print(f"📊 Instantiation report saved to: {report_file}")

def main():
    """Main instantiation entry point"""
    if len(sys.argv) != 2:
        print("Usage: python v8_universe_instantiate.py <port_directory>")
        sys.exit(1)
    
    port_dir = Path(sys.argv[1])
    if not port_dir.exists():
        print(f"Error: Port directory does not exist: {port_dir}")
        sys.exit(1)
    
    instantiator = CNSv8UniverseInstantiator(port_dir)
    
    if instantiator.instantiate_universe():
        instantiator.generate_instantiation_report()
        sys.exit(0)
    else:
        print("❌ Universe instantiation failed")
        sys.exit(1)

if __name__ == "__main__":
    main() 