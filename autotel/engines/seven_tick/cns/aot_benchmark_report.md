# AOT Compiler 80/20 Benchmark Report

## Summary

- Total Tests: 22
- Passed: 5 (22.7%)
- Failed: 17
- Average Compilation Time: 99.26ms
- Peak Memory Usage: 16384KB

## 80/20 Distribution

- 80% Common Cases: Simple ontologies, basic shapes, standard queries
- 15% Moderate Cases: Dependencies, joins, imports
- 5% Edge Cases: Complex restrictions, recursive queries, error handling

## Performance Targets

- Simple Compilation: < 100ms
- Generated Code Execution: < 49 cycles
- Memory Usage: < 50MB for 10K triples
- Incremental Builds: < 50% of full compilation time
