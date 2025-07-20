#!/usr/bin/env python3
"""
CNS Binary Materializer - Python Package Setup
Installation script for the Python bindings with C library integration
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext
from distutils.command.build import build
from distutils.command.install import install

# Package information
PACKAGE_NAME = "cns-graph"
VERSION = "1.0.0"
DESCRIPTION = "High-performance graph operations with zero-copy NumPy integration"
AUTHOR = "CNS Team"
AUTHOR_EMAIL = "team@cns.ai"

class CNSBuildExt(build_ext):
    """Custom build extension that compiles the C library"""
    
    def run(self):
        """Build the C library before building Python extensions"""
        self.build_c_library()
        super().run()
    
    def build_c_library(self):
        """Build the CNS C library using make"""
        print("🔨 Building CNS C library...")
        
        # Change to the C source directory
        original_dir = os.getcwd()
        source_dir = Path(__file__).parent
        
        try:
            os.chdir(source_dir)
            
            # Clean any existing builds
            if os.path.exists("libcns_binary_materializer.so"):
                os.remove("libcns_binary_materializer.so")
            if os.path.exists("libcns_binary_simple.a"):
                print("📦 Found existing static library, building shared library...")
            
            # Try to build shared library from existing objects
            if self.try_build_shared_library():
                print("✅ Successfully built shared library from existing objects")
                return
            
            # Build from source using make
            if self.try_build_with_make():
                print("✅ Successfully built library with make")
                return
                
            # Manual compilation fallback
            if self.try_manual_compilation():
                print("✅ Successfully built library manually")
                return
                
            print("❌ Failed to build C library. Please build manually:")
            print("   cd src/binary_materializer && make")
            sys.exit(1)
            
        finally:
            os.chdir(original_dir)
    
    def try_build_shared_library(self):
        """Try to build shared library from existing object files"""
        try:
            # Look for existing object files
            object_files = list(Path(".").glob("*.o"))
            if not object_files:
                return False
                
            print(f"📦 Found {len(object_files)} object files, linking shared library...")
            
            # Link shared library
            cmd = [
                "clang", "-shared", "-fPIC", "-o", "libcns_binary_materializer.so"
            ] + [str(f) for f in object_files] + ["-lm"]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0 and os.path.exists("libcns_binary_materializer.so"):
                return True
                
            print(f"⚠️ Shared library linking failed: {result.stderr}")
            return False
            
        except Exception as e:
            print(f"⚠️ Shared library build failed: {e}")
            return False
    
    def try_build_with_make(self):
        """Try to build using the existing Makefile"""
        try:
            # Try different make targets
            for target in ["", "shared", "libcns_binary_materializer.so"]:
                cmd = ["make"] + ([target] if target else [])
                print(f"🔨 Running: {' '.join(cmd)}")
                
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
                
                if result.returncode == 0:
                    # Check if shared library was created
                    if os.path.exists("libcns_binary_materializer.so"):
                        return True
                    # Check if we can convert static to shared
                    if os.path.exists("libcns_binary_simple.a"):
                        return self.convert_static_to_shared()
                        
            return False
            
        except subprocess.TimeoutExpired:
            print("⚠️ Make command timed out")
            return False
        except Exception as e:
            print(f"⚠️ Make build failed: {e}")
            return False
    
    def convert_static_to_shared(self):
        """Convert static library to shared library"""
        try:
            print("🔄 Converting static library to shared library...")
            
            # Extract objects from static library
            result = subprocess.run(
                ["ar", "x", "libcns_binary_simple.a"],
                capture_output=True, text=True
            )
            
            if result.returncode != 0:
                return False
            
            # Find extracted object files
            object_files = list(Path(".").glob("*.o"))
            if not object_files:
                return False
                
            # Link shared library
            cmd = [
                "clang", "-shared", "-fPIC", "-o", "libcns_binary_materializer.so"
            ] + [str(f) for f in object_files] + ["-lm"]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            return result.returncode == 0 and os.path.exists("libcns_binary_materializer.so")
            
        except Exception as e:
            print(f"⚠️ Static to shared conversion failed: {e}")
            return False
    
    def try_manual_compilation(self):
        """Try manual compilation of source files"""
        try:
            print("🔨 Attempting manual compilation...")
            
            # Find source files
            source_files = ["core.c", "serialize.c", "deserialize.c", "graph.c"]
            missing_files = [f for f in source_files if not os.path.exists(f)]
            
            if missing_files:
                print(f"❌ Missing source files: {missing_files}")
                return False
            
            # Compile to object files
            for source_file in source_files:
                obj_file = source_file.replace('.c', '.o')
                cmd = [
                    "clang", "-c", "-fPIC", "-O3", "-Wall", "-Wextra",
                    "-I../../include", "-march=native", "-ffast-math",
                    "-o", obj_file, source_file
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True)
                if result.returncode != 0:
                    print(f"❌ Failed to compile {source_file}: {result.stderr}")
                    return False
            
            # Link shared library
            object_files = [f.replace('.c', '.o') for f in source_files]
            cmd = [
                "clang", "-shared", "-fPIC", "-o", "libcns_binary_materializer.so"
            ] + object_files + ["-lm"]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0 and os.path.exists("libcns_binary_materializer.so"):
                return True
                
            print(f"❌ Failed to link shared library: {result.stderr}")
            return False
            
        except Exception as e:
            print(f"⚠️ Manual compilation failed: {e}")
            return False


class CNSInstall(install):
    """Custom install command that copies the shared library"""
    
    def run(self):
        """Install package and copy shared library"""
        super().run()
        self.copy_shared_library()
    
    def copy_shared_library(self):
        """Copy shared library to the installation directory"""
        source_dir = Path(__file__).parent
        lib_file = source_dir / "libcns_binary_materializer.so"
        
        if not lib_file.exists():
            print("⚠️ Shared library not found, trying alternative names...")
            for alt_name in ["libcns_binary_materializer.dylib", "libcns_binary_simple.a"]:
                alt_file = source_dir / alt_name
                if alt_file.exists():
                    lib_file = alt_file
                    break
            else:
                print("❌ No library file found to install")
                return
        
        # Find the installation directory
        install_dir = Path(self.install_purelib) / "cns_graph"
        install_dir.mkdir(parents=True, exist_ok=True)
        
        # Copy library file
        dest_file = install_dir / lib_file.name
        print(f"📦 Copying {lib_file} to {dest_file}")
        shutil.copy2(lib_file, dest_file)


def read_readme():
    """Read README file for long description"""
    readme_path = Path(__file__).parent / "README.md"
    if readme_path.exists():
        with open(readme_path, 'r', encoding='utf-8') as f:
            return f.read()
    else:
        return DESCRIPTION


def get_requirements():
    """Get package requirements"""
    requirements = [
        "numpy>=1.19.0",
    ]
    
    # Optional dependencies
    extras_require = {
        'networkx': ['networkx>=2.5'],
        'performance': ['psutil>=5.7.0'],
        'full': ['networkx>=2.5', 'psutil>=5.7.0'],
    }
    
    return requirements, extras_require


def main():
    """Main setup function"""
    requirements, extras_require = get_requirements()
    
    # Setup configuration
    setup_config = {
        'name': PACKAGE_NAME,
        'version': VERSION,
        'description': DESCRIPTION,
        'long_description': read_readme(),
        'long_description_content_type': 'text/markdown',
        'author': AUTHOR,
        'author_email': AUTHOR_EMAIL,
        'url': 'https://github.com/cns-team/binary-materializer',
        'packages': ['cns_graph'],
        'package_dir': {'cns_graph': '.'},
        'py_modules': ['cns_graph'],
        'install_requires': requirements,
        'extras_require': extras_require,
        'python_requires': '>=3.7',
        'classifiers': [
            'Development Status :: 4 - Beta',
            'Intended Audience :: Developers',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: MIT License',
            'Programming Language :: Python :: 3',
            'Programming Language :: Python :: 3.7',
            'Programming Language :: Python :: 3.8',
            'Programming Language :: Python :: 3.9',
            'Programming Language :: Python :: 3.10',
            'Programming Language :: Python :: 3.11',
            'Programming Language :: C',
            'Topic :: Scientific/Engineering',
            'Topic :: Software Development :: Libraries :: Python Modules',
        ],
        'keywords': [
            'graph', 'network', 'numpy', 'performance', 
            'zero-copy', 'binary', 'serialization'
        ],
        'cmdclass': {
            'build_ext': CNSBuildExt,
            'install': CNSInstall,
        },
        'zip_safe': False,  # Due to shared library
    }
    
    # Add C extension if we're building
    if 'build' in sys.argv or 'build_ext' in sys.argv or 'install' in sys.argv:
        # This is mainly for dependency tracking, actual compilation is in CNSBuildExt
        ext_modules = [
            Extension(
                'cns_graph_native',
                sources=[],  # Sources handled by custom build
                libraries=['m'],
                extra_compile_args=['-O3', '-march=native'],
            )
        ]
        setup_config['ext_modules'] = ext_modules
    
    setup(**setup_config)


if __name__ == '__main__':
    print("🐍 CNS Binary Materializer - Python Setup")
    print("=" * 50)
    
    # Check Python version
    if sys.version_info < (3, 7):
        print("❌ Python 3.7+ is required")
        sys.exit(1)
    
    # Check for required tools
    required_tools = ['clang', 'make', 'ar']
    missing_tools = []
    
    for tool in required_tools:
        if not shutil.which(tool):
            missing_tools.append(tool)
    
    if missing_tools:
        print(f"⚠️ Missing build tools: {', '.join(missing_tools)}")
        print("Please install development tools (e.g., build-essential, xcode)")
    
    # Run setup
    try:
        main()
        print("✅ Setup completed successfully!")
        print("\n💡 Usage:")
        print("  from cns_graph import CNSGraph")
        print("  graph = CNSGraph(directed=True, weighted=True)")
        print("  # See demo_python.py for more examples")
        
    except KeyboardInterrupt:
        print("\n❌ Setup interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Setup failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)