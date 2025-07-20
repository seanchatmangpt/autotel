# Graph Binary Format Visual Diagram

## File Layout Visualization

```
┌─────────────────────────────────────────────────────────────┐
│                        HEADER (32 bytes)                     │
├─────────────────────────────────────────────────────────────┤
│ Magic: 'GRAP' │ Version │ File Size │ Flags │ CRC32 │ Meta │
│   (4 bytes)   │(2+2 b)  │ (8 bytes) │(4 b)  │ (4 b) │ Off  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    METADATA SECTION                          │
├─────────────────────────────────────────────────────────────┤
│ Node Count │ Edge Count │ Index Offsets │ Data Offsets │... │
│ (8 bytes)  │ (8 bytes)  │   (32 bytes)  │  (32 bytes)  │    │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      NODE INDEX                              │
├─────────────────────────────────────────────────────────────┤
│ Node 0: [Data Offset │ Out Degree │ In Degree]              │
│ Node 1: [Data Offset │ Out Degree │ In Degree]              │
│ Node 2: [Data Offset │ Out Degree │ In Degree]              │
│ ...                                                          │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    NODE DATA SECTION                         │
├─────────────────────────────────────────────────────────────┤
│ Node 0: [ID │ Props │ Out Edges: [E1│E2│...] │ In Edges]   │
│ Node 1: [ID │ Props │ Out Edges: [E3│E4│...] │ In Edges]   │
│ ...                                                          │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    EDGE DATA SECTION                         │
├─────────────────────────────────────────────────────────────┤
│ Edge 0: [Weight │ Property Count │ Properties]              │
│ Edge 1: [Weight │ Property Count │ Properties]              │
│ ...                                                          │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   PROPERTY DATA POOL                         │
├─────────────────────────────────────────────────────────────┤
│ Strings, Values, Arrays, Maps - Contiguous Storage          │
│ [Len│"name"] [Len│"weight"] [Int:42] [Float:3.14] ...      │
└─────────────────────────────────────────────────────────────┘
```

## Detailed Structure Examples

### Node Index Entry (16 bytes each)
```
┌────────────────┬─────────────┬─────────────┐
│  Data Offset   │ Out Degree  │  In Degree  │
│   (8 bytes)    │  (4 bytes)  │  (4 bytes)  │
└────────────────┴─────────────┴─────────────┘
```

### Node Data Structure
```
┌──────────┬──────────────┬─────────────────────────────────────┐
│ Node ID  │ Prop Count   │         Properties Array            │
│ (var)    │ (2 bytes)    │    [Key│Val│Type] × prop_count     │
└──────────┴──────────────┴─────────────────────────────────────┘
                                          ↓
┌─────────────────────────────────────────────────────────────────┐
│                      Adjacency Lists                             │
├─────────────────────────────────────────────────────────────────┤
│ Out Edges: [Target ID│Edge Offset] × out_degree                 │
│ In Edges:  [Source ID│Edge Offset] × in_degree                  │
└─────────────────────────────────────────────────────────────────┘
```

### Edge Reference Structure (8 bytes)
```
┌─────────────────┬──────────────────┐
│   Target Node   │   Edge Data      │
│      ID         │     Offset       │
│   (4 bytes)     │   (4 bytes)      │
└─────────────────┴──────────────────┘
```

### Property Reference Structure (12 bytes)
```
┌───────────────┬───────────────┬──────────┬──────────┐
│  Key Offset   │ Value Offset  │   Type   │ Padding  │
│  (4 bytes)    │  (4 bytes)    │ (1 byte) │(3 bytes) │
└───────────────┴───────────────┴──────────┴──────────┘
```

## Example: Small Directed Graph

Graph: A → B → C, A → C

### Binary Layout
```
HEADER (0x0000-0x001F):
  4752 4150  0001 0000  0000 1000 0000 0000  # Magic, Version, Size
  0000 0001  XXXX XXXX  0000 0020 0000 0000  # Flags, CRC, Meta offset

METADATA (0x0020-0x005F):
  0000 0003 0000 0000  # 3 nodes
  0000 0003 0000 0000  # 3 edges
  0000 0060 0000 0000  # Node index at 0x60
  0000 00C0 0000 0000  # Node data at 0xC0
  ...

NODE INDEX (0x0060-0x008F):
  [0x00C0, 2, 0]  # Node A: offset 0xC0, 2 out, 0 in
  [0x00D0, 1, 1]  # Node B: offset 0xD0, 1 out, 1 in
  [0x00E0, 0, 2]  # Node C: offset 0xE0, 0 out, 2 in

NODE DATA (0x00C0-):
  Node A: [ID:0] [Props:0] [Out: (1,0) (2,0)]
  Node B: [ID:1] [Props:0] [Out: (2,0)] [In: (0)]
  Node C: [ID:2] [Props:0] [In: (0) (1)]
```

## Memory Access Patterns

### Sequential Access (Cache-Friendly)
```
Read Pattern for BFS from Node A:
1. Index[A] → Node[A] → Edges → Node[B], Node[C]
2. Index[B] → Node[B] → Edges → (already visited C)
3. All accesses are forward sequential
```

### Random Access (O(1) Lookup)
```
Find Node X:
1. Offset = NODE_INDEX_BASE + (X * 16)
2. Read NodeIndexEntry at Offset
3. Jump to NodeData using data_offset
```

## Optimization Features Illustrated

### 1. Delta Encoding for Edges
```
Original edges from Node 0: [5, 12, 15, 23, 30]
Delta encoded:             [5, 7,  3,  8,  7]
Saves space for sequential node IDs
```

### 2. Variable-Length Encoding
```
Small ID (42):     0x2A         (1 byte)
Medium ID (1000):  0x83 0xE8    (2 bytes)  
Large ID (100000): 0xC1 0x86 0xA0 (3 bytes)
```

### 3. Property Pool Deduplication
```
Multiple nodes with property "type"="router":
- Store "type" string once in pool
- All nodes reference same offset
- Significant savings for repeated values
```

## Performance Characteristics

### Space Efficiency
```
Dense Graph (1M nodes, 10M edges):
- Traditional adjacency list: ~160 MB
- This format: ~96 MB (40% reduction)

Sparse Graph (1M nodes, 2M edges):
- Traditional: ~80 MB  
- This format: ~32 MB (60% reduction)
```

### Access Times
```
Operation           Time Complexity
-----------        ---------------
Find node          O(1)
Get neighbors      O(degree)
Add property       O(1) amortized
Iterate all nodes  O(n)
Find edge          O(log degree) with sorted edges
```