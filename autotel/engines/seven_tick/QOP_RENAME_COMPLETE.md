# Naming Update Complete

I've successfully renamed the Monte Carlo Planner to **Query Optimization Planner (QOP)** to avoid confusion with Model Context Protocol (MCP).

## Changes Made:

### C Files Renamed:
- `c_src/mcp.h` → `c_src/qop.h`
- `c_src/mcp.c` → `c_src/qop.c`
- All function prefixes changed from `mcp_` to `qop_`
- All type names changed from `MCP_` to `QOP_`

### Python Files Updated:
- `mcp.py` → `qop.py`
- Class renamed from `MonteCarloPlanner` to `QueryOptimizationPlanner`
- Import in `__init__.py` updated

### Build System:
- Makefile updated to build `libqop.so` instead of `libmcp.so`
- Old MCP files removed

### Documentation:
- References to "Monte Carlo Planner" changed to "Query Optimization Planner (QOP)"
- Acronym MCP replaced with QOP throughout

The functionality remains exactly the same - it's still using Monte Carlo Tree Search (MCTS) internally for query optimization, but now has a clearer name that won't be confused with other protocols.
