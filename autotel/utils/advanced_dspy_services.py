#!/usr/bin/env python3
"""
Advanced DSPy Services - Enhanced LLM integration with caching, model selection, and async execution
"""

import json
import asyncio
import time
import hashlib
from typing import Dict, Any, Optional, List, Union, Callable, Type
from dataclasses import dataclass, field
from enum import Enum
from functools import wraps
import threading
import logging

import dspy
from dspy import Predict, Signature

logger = logging.getLogger(__name__)

class ModelProvider(Enum):
    """Supported LLM providers"""
    OLLAMA = "ollama"
    OPENAI = "openai"
    ANTHROPIC = "anthropic"
    GOOGLE = "google"
    LOCAL = "local"

@dataclass
class ModelConfig:
    """Configuration for an LLM model"""
    provider: ModelProvider
    model_name: str
    temperature: float = 0.7
    max_tokens: Optional[int] = None
    api_key: Optional[str] = None
    api_base: Optional[str] = None
    priority: int = 1  # Lower number = higher priority

@dataclass
class CacheEntry:
    """Cache entry for DSPy results"""
    signature_name: str
    input_hash: str
    result: Dict[str, Any]
    timestamp: float
    model_used: str
    duration: float

class DSPyCache:
    """Intelligent caching for DSPy results"""
    
    def __init__(self, max_size: int = 1000, ttl_seconds: int = 3600):
        self.max_size = max_size
        self.ttl_seconds = ttl_seconds
        self.cache: Dict[str, CacheEntry] = {}
        self._lock = threading.Lock()
    
    def _generate_cache_key(self, signature_name: str, **kwargs) -> str:
        """Generate a cache key from signature name and parameters"""
        # Sort parameters for consistent hashing
        sorted_params = sorted(kwargs.items())
        param_str = json.dumps(sorted_params, sort_keys=True)
        
        # Create hash
        hash_input = f"{signature_name}:{param_str}"
        return hashlib.md5(hash_input.encode()).hexdigest()
    
    def get(self, signature_name: str, **kwargs) -> Optional[Dict[str, Any]]:
        """Get cached result if available and not expired"""
        with self._lock:
            cache_key = self._generate_cache_key(signature_name, **kwargs)
            entry = self.cache.get(cache_key)
            
            if entry:
                # Check if expired
                if time.time() - entry.timestamp > self.ttl_seconds:
                    del self.cache[cache_key]
                    return None
                
                logger.info(f"Cache hit for {signature_name}")
                return entry.result
            
            return None
    
    def set(self, signature_name: str, result: Dict[str, Any], model_used: str, duration: float, **kwargs) -> None:
        """Cache a result"""
        with self._lock:
            cache_key = self._generate_cache_key(signature_name, **kwargs)
            
            # Check cache size
            if len(self.cache) >= self.max_size:
                # Remove oldest entry
                oldest_key = min(self.cache.keys(), key=lambda k: self.cache[k].timestamp)
                del self.cache[oldest_key]
            
            entry = CacheEntry(
                signature_name=signature_name,
                input_hash=cache_key,
                result=result,
                timestamp=time.time(),
                model_used=model_used,
                duration=duration
            )
            
            self.cache[cache_key] = entry
            logger.info(f"Cached result for {signature_name}")
    
    def clear(self) -> None:
        """Clear all cached results"""
        with self._lock:
            self.cache.clear()
            logger.info("DSPy cache cleared")
    
    def get_stats(self) -> Dict[str, Any]:
        """Get cache statistics"""
        with self._lock:
            return {
                "size": len(self.cache),
                "max_size": self.max_size,
                "ttl_seconds": self.ttl_seconds,
                "oldest_entry": min((entry.timestamp for entry in self.cache.values()), default=0),
                "newest_entry": max((entry.timestamp for entry in self.cache.values()), default=0)
            }

class ModelSelector:
    """Intelligent model selection based on task requirements"""
    
    def __init__(self):
        self.models: Dict[str, ModelConfig] = {}
        self.model_performance: Dict[str, List[float]] = {}  # model_name -> [durations]
        self._lock = threading.Lock()
    
    def add_model(self, model_config: ModelConfig) -> None:
        """Add a model configuration"""
        with self._lock:
            model_key = f"{model_config.provider.value}:{model_config.model_name}"
            self.models[model_key] = model_config
            self.model_performance[model_key] = []
            logger.info(f"Added model: {model_key}")
    
    def select_model(self, signature_name: str, **kwargs) -> ModelConfig:
        """Select the best model for a given signature and parameters"""
        with self._lock:
            if not self.models:
                raise ValueError("No models configured")
            
            # Simple selection strategy: choose model with highest priority
            # In a real implementation, this could consider:
            # - Task complexity (based on signature name)
            # - Input size
            # - Historical performance
            # - Cost considerations
            # - Model availability
            
            best_model = min(self.models.values(), key=lambda m: m.priority)
            return best_model
    
    def record_performance(self, model_name: str, duration: float) -> None:
        """Record performance metrics for model selection"""
        with self._lock:
            if model_name in self.model_performance:
                self.model_performance[model_name].append(duration)
                # Keep only last 100 measurements
                if len(self.model_performance[model_name]) > 100:
                    self.model_performance[model_name] = self.model_performance[model_name][-100:]
    
    def get_model_stats(self) -> Dict[str, Any]:
        """Get model performance statistics"""
        with self._lock:
            stats = {}
            for model_name, durations in self.model_performance.items():
                if durations:
                    stats[model_name] = {
                        "avg_duration": sum(durations) / len(durations),
                        "min_duration": min(durations),
                        "max_duration": max(durations),
                        "count": len(durations)
                    }
            return stats

class AdvancedDSPyRegistry:
    """Enhanced DSPy registry with advanced features"""
    
    def __init__(self, cache_size: int = 1000, cache_ttl: int = 3600):
        self.cache = DSPyCache(max_size=cache_size, ttl_seconds=cache_ttl)
        self.model_selector = ModelSelector()
        self.signatures: Dict[str, Type[Signature]] = {}
        self.predict_modules: Dict[str, Predict] = {}
        self.rate_limiters: Dict[str, float] = {}  # signature_name -> last_call_time
        self._lock = threading.Lock()
    
    def register_signature(self, name: str, signature_class: Type[Signature], 
                          model_config: Optional[ModelConfig] = None,
                          rate_limit_seconds: float = 0.0) -> None:
        """Register a DSPy signature with advanced configuration"""
        with self._lock:
            self.signatures[name] = signature_class
            self.predict_modules[name] = Predict(signature_class)
            
            if model_config:
                self.model_selector.add_model(model_config)
            
            if rate_limit_seconds > 0:
                self.rate_limiters[name] = rate_limit_seconds
            
            logger.info(f"Registered advanced DSPy signature: {name}")
    
    def call_signature(self, signature_name: str, use_cache: bool = True, **kwargs) -> Dict[str, Any]:
        """Call a DSPy signature with advanced features"""
        with self._lock:
            if signature_name not in self.signatures:
                raise ValueError(f"Unknown DSPy signature: {signature_name}")
            
            # Check cache first
            if use_cache:
                cached_result = self.cache.get(signature_name, **kwargs)
                if cached_result:
                    return cached_result
            
            # Rate limiting
            if signature_name in self.rate_limiters:
                self._check_rate_limit(signature_name)
            
            # Model selection
            model_config = self.model_selector.select_model(signature_name, **kwargs)
            
            # Configure DSPy with selected model
            self._configure_dspy_model(model_config)
            
            # Execute with timing
            start_time = time.time()
            try:
                result = self.predict_modules[signature_name](**kwargs)
                duration = time.time() - start_time
                
                # Record performance
                model_key = f"{model_config.provider.value}:{model_config.model_name}"
                self.model_selector.record_performance(model_key, duration)
                
                # Extract result
                if hasattr(result, '_store') and isinstance(result._store, dict):
                    output_dict = dict(result._store)
                else:
                    output_dict = {k: v for k, v in vars(result).items() if not k.startswith('_')}
                
                # Cache result
                if use_cache:
                    self.cache.set(signature_name, output_dict, model_key, duration, **kwargs)
                
                logger.info(f"Executed {signature_name} with {model_key} in {duration:.3f}s")
                return output_dict
                
            except Exception as e:
                duration = time.time() - start_time
                logger.error(f"Failed to execute {signature_name}: {e}")
                raise
    
    async def call_signature_async(self, signature_name: str, use_cache: bool = True, **kwargs) -> Dict[str, Any]:
        """Async version of call_signature"""
        # Run in thread pool to avoid blocking
        loop = asyncio.get_event_loop()
        return await loop.run_in_executor(
            None, 
            lambda: self.call_signature(signature_name, use_cache, **kwargs)
        )
    
    def batch_call_signatures(self, calls: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Execute multiple DSPy calls in batch"""
        results = []
        
        for call in calls:
            signature_name = call['signature_name']
            kwargs = call.get('kwargs', {})
            use_cache = call.get('use_cache', True)
            
            try:
                result = self.call_signature(signature_name, use_cache, **kwargs)
                results.append({
                    'signature_name': signature_name,
                    'success': True,
                    'result': result
                })
            except Exception as e:
                results.append({
                    'signature_name': signature_name,
                    'success': False,
                    'error': str(e)
                })
        
        return results
    
    def _check_rate_limit(self, signature_name: str) -> None:
        """Check and enforce rate limiting"""
        current_time = time.time()
        last_call = getattr(self, f'_last_call_{signature_name}', 0)
        rate_limit = self.rate_limiters[signature_name]
        
        if current_time - last_call < rate_limit:
            sleep_time = rate_limit - (current_time - last_call)
            time.sleep(sleep_time)
        
        setattr(self, f'_last_call_{signature_name}', time.time())
    
    def _configure_dspy_model(self, model_config: ModelConfig) -> None:
        """Configure DSPy with the selected model"""
        if model_config.provider == ModelProvider.OLLAMA:
            lm = dspy.LM(f'ollama/{model_config.model_name}', 
                        temperature=model_config.temperature,
                        api_base=model_config.api_base)
        elif model_config.provider == ModelProvider.OPENAI:
            lm = dspy.OpenAI(model=model_config.model_name,
                           temperature=model_config.temperature,
                           api_key=model_config.api_key)
        elif model_config.provider == ModelProvider.ANTHROPIC:
            lm = dspy.Anthropic(model=model_config.model_name,
                              temperature=model_config.temperature,
                              api_key=model_config.api_key)
        else:
            # Default to Ollama
            lm = dspy.LM(f'ollama/{model_config.model_name}', 
                        temperature=model_config.temperature)
        
        dspy.configure(lm=lm)
    
    def get_stats(self) -> Dict[str, Any]:
        """Get comprehensive statistics"""
        return {
            "cache_stats": self.cache.get_stats(),
            "model_stats": self.model_selector.get_model_stats(),
            "signatures": list(self.signatures.keys()),
            "rate_limited_signatures": list(self.rate_limiters.keys())
        }

# Global advanced registry instance
advanced_dspy_registry = AdvancedDSPyRegistry()

def initialize_advanced_dspy_services() -> None:
    """Initialize advanced DSPy services with default models"""
    # Add default models
    ollama_model = ModelConfig(
        provider=ModelProvider.OLLAMA,
        model_name="qwen2.5:7b",
        temperature=0.7,
        priority=1
    )
    
    openai_model = ModelConfig(
        provider=ModelProvider.OPENAI,
        model_name="gpt-4o-mini",
        temperature=0.7,
        priority=2
    )
    
    advanced_dspy_registry.model_selector.add_model(ollama_model)
    advanced_dspy_registry.model_selector.add_model(openai_model)
    
    logger.info("Advanced DSPy services initialized")

def call_advanced_dspy_service(signature_name: str, use_cache: bool = True, **kwargs) -> Dict[str, Any]:
    """Convenience function to call advanced DSPy services"""
    return advanced_dspy_registry.call_signature(signature_name, use_cache, **kwargs)

async def call_advanced_dspy_service_async(signature_name: str, use_cache: bool = True, **kwargs) -> Dict[str, Any]:
    """Async convenience function to call advanced DSPy services"""
    return await advanced_dspy_registry.call_signature_async(signature_name, use_cache, **kwargs) 