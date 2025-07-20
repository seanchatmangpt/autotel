#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph_binary_format.h"

// Example: Creating a simple directed graph and writing it to binary format
// Graph structure: A -> B -> C, A -> C

typedef struct {
    FILE* file;
    uint64_t current_offset;
    GraphHeader header;
    GraphMetadata metadata;
} GraphWriter;

// Initialize writer
GraphWriter* graph_writer_create(const char* filename) {
    GraphWriter* writer = malloc(sizeof(GraphWriter));
    writer->file = fopen(filename, "wb");
    writer->current_offset = 0;
    
    // Initialize header
    writer->header.magic = GRAPH_MAGIC;
    writer->header.version_major = GRAPH_VERSION_MAJOR;
    writer->header.version_minor = GRAPH_VERSION_MINOR;
    writer->header.flags = GRAPH_FLAG_DIRECTED; // Directed graph
    writer->header.metadata_offset = sizeof(GraphHeader);
    
    // Write placeholder header
    fwrite(&writer->header, sizeof(GraphHeader), 1, writer->file);
    writer->current_offset = sizeof(GraphHeader);
    
    return writer;
}

// Write metadata section
void write_metadata(GraphWriter* writer, uint64_t node_count, uint64_t edge_count) {
    writer->metadata.node_count = node_count;
    writer->metadata.edge_count = edge_count;
    writer->metadata.node_id_size = 4; // 32-bit node IDs
    writer->metadata.edge_id_size = 4; // 32-bit edge IDs
    writer->metadata.extension_count = 0;
    
    // Calculate offsets (aligned to 8 bytes)
    uint64_t offset = writer->header.metadata_offset + sizeof(GraphMetadata);
    offset = ALIGN_TO_8(offset);
    
    writer->metadata.node_index_offset = offset;
    offset += node_count * sizeof(NodeIndexEntry);
    offset = ALIGN_TO_8(offset);
    
    writer->metadata.node_data_offset = offset;
    // Reserve space for node data (estimate)
    offset += node_count * 64; // Rough estimate
    offset = ALIGN_TO_8(offset);
    
    writer->metadata.edge_index_offset = offset;
    offset += edge_count * sizeof(EdgeRef);
    offset = ALIGN_TO_8(offset);
    
    writer->metadata.edge_data_offset = offset;
    offset += edge_count * 16; // Rough estimate
    offset = ALIGN_TO_8(offset);
    
    writer->metadata.property_pool_offset = offset;
    
    // Write metadata
    fwrite(&writer->metadata, sizeof(GraphMetadata), 1, writer->file);
    writer->current_offset += sizeof(GraphMetadata);
}

// Example: Write a simple 3-node graph
void write_example_graph(const char* filename) {
    GraphWriter* writer = graph_writer_create(filename);
    
    // Define our graph: 3 nodes, 3 edges
    write_metadata(writer, 3, 3);
    
    // Seek to node index location
    fseek(writer->file, writer->metadata.node_index_offset, SEEK_SET);
    
    // Write node index entries
    NodeIndexEntry node_entries[3] = {
        {.data_offset = 0, .out_degree = 2, .in_degree = 0}, // Node A
        {.data_offset = 0, .out_degree = 1, .in_degree = 1}, // Node B
        {.data_offset = 0, .out_degree = 0, .in_degree = 2}  // Node C
    };
    
    // Calculate actual offsets for node data
    uint64_t node_data_base = writer->metadata.node_data_offset;
    uint64_t offset = node_data_base;
    
    // Node A: ID(4) + prop_count(2) + 2 edges(8 each) = 22 bytes
    node_entries[0].data_offset = offset;
    offset += 4 + 2 + (2 * sizeof(EdgeRef));
    offset = ALIGN_TO_8(offset);
    
    // Node B: ID(4) + prop_count(2) + 1 out edge(8) + 1 in ref(4) = 18 bytes
    node_entries[1].data_offset = offset;
    offset += 4 + 2 + sizeof(EdgeRef) + 4;
    offset = ALIGN_TO_8(offset);
    
    // Node C: ID(4) + prop_count(2) + 2 in refs(4 each) = 14 bytes
    node_entries[2].data_offset = offset;
    
    // Write node index
    fwrite(node_entries, sizeof(NodeIndexEntry), 3, writer->file);
    
    // Write node data
    fseek(writer->file, node_data_base, SEEK_SET);
    
    // Node A (ID=0)
    uint32_t node_id = 0;
    uint16_t prop_count = 0;
    fwrite(&node_id, sizeof(uint32_t), 1, writer->file);
    fwrite(&prop_count, sizeof(uint16_t), 1, writer->file);
    
    // Node A edges: -> B, -> C
    EdgeRef edges_a[2] = {
        {.target_node_id = 1, .edge_data_offset = 0},
        {.target_node_id = 2, .edge_data_offset = 0}
    };
    fwrite(edges_a, sizeof(EdgeRef), 2, writer->file);
    
    // Align to 8 bytes
    uint8_t padding[8] = {0};
    fwrite(padding, ALIGN_TO_8(ftell(writer->file)) - ftell(writer->file), 1, writer->file);
    
    // Node B (ID=1)
    node_id = 1;
    fwrite(&node_id, sizeof(uint32_t), 1, writer->file);
    fwrite(&prop_count, sizeof(uint16_t), 1, writer->file);
    
    // Node B edges: -> C
    EdgeRef edge_b = {.target_node_id = 2, .edge_data_offset = 0};
    fwrite(&edge_b, sizeof(EdgeRef), 1, writer->file);
    
    // Node B incoming: <- A
    uint32_t incoming_b = 0;
    fwrite(&incoming_b, sizeof(uint32_t), 1, writer->file);
    
    // Align
    fwrite(padding, ALIGN_TO_8(ftell(writer->file)) - ftell(writer->file), 1, writer->file);
    
    // Node C (ID=2)
    node_id = 2;
    fwrite(&node_id, sizeof(uint32_t), 1, writer->file);
    fwrite(&prop_count, sizeof(uint16_t), 1, writer->file);
    
    // Node C incoming: <- A, <- B
    uint32_t incoming_c[2] = {0, 1};
    fwrite(incoming_c, sizeof(uint32_t), 2, writer->file);
    
    // Update file size and checksum
    fseek(writer->file, 0, SEEK_END);
    writer->header.file_size = ftell(writer->file);
    
    // Write final header
    fseek(writer->file, 0, SEEK_SET);
    fwrite(&writer->header, sizeof(GraphHeader), 1, writer->file);
    
    fclose(writer->file);
    free(writer);
    
    printf("Graph written to %s\n", filename);
}

// Example: Reading the graph
typedef struct {
    FILE* file;
    GraphHeader header;
    GraphMetadata metadata;
    NodeIndexEntry* node_index;
} GraphReader;

GraphReader* graph_reader_open(const char* filename) {
    GraphReader* reader = malloc(sizeof(GraphReader));
    reader->file = fopen(filename, "rb");
    
    // Read header
    fread(&reader->header, sizeof(GraphHeader), 1, reader->file);
    
    // Validate magic number
    if (reader->header.magic != GRAPH_MAGIC) {
        fprintf(stderr, "Invalid graph file format\n");
        fclose(reader->file);
        free(reader);
        return NULL;
    }
    
    // Read metadata
    fseek(reader->file, reader->header.metadata_offset, SEEK_SET);
    fread(&reader->metadata, sizeof(GraphMetadata), 1, reader->file);
    
    // Load node index into memory for fast access
    reader->node_index = malloc(reader->metadata.node_count * sizeof(NodeIndexEntry));
    fseek(reader->file, reader->metadata.node_index_offset, SEEK_SET);
    fread(reader->node_index, sizeof(NodeIndexEntry), reader->metadata.node_count, reader->file);
    
    return reader;
}

// Get neighbors of a node
void print_node_neighbors(GraphReader* reader, uint32_t node_id) {
    if (node_id >= reader->metadata.node_count) {
        fprintf(stderr, "Invalid node ID\n");
        return;
    }
    
    NodeIndexEntry* entry = &reader->node_index[node_id];
    printf("Node %u has %u outgoing edges:\n", node_id, entry->out_degree);
    
    // Seek to node data
    fseek(reader->file, entry->data_offset, SEEK_SET);
    
    // Read node ID and property count
    uint32_t stored_id;
    uint16_t prop_count;
    fread(&stored_id, sizeof(uint32_t), 1, reader->file);
    fread(&prop_count, sizeof(uint16_t), 1, reader->file);
    
    // Skip properties if any
    fseek(reader->file, prop_count * sizeof(PropertyRef), SEEK_CUR);
    
    // Read outgoing edges
    for (uint32_t i = 0; i < entry->out_degree; i++) {
        EdgeRef edge;
        fread(&edge, sizeof(EdgeRef), 1, reader->file);
        printf("  -> Node %u\n", edge.target_node_id);
    }
}

// Example usage
int main() {
    const char* filename = "example_graph.bin";
    
    // Write example graph
    printf("Writing example graph...\n");
    write_example_graph(filename);
    
    // Read and display graph
    printf("\nReading graph...\n");
    GraphReader* reader = graph_reader_open(filename);
    if (!reader) return 1;
    
    printf("Graph has %llu nodes and %llu edges\n", 
           reader->metadata.node_count, reader->metadata.edge_count);
    
    // Print neighbors for each node
    for (uint32_t i = 0; i < reader->metadata.node_count; i++) {
        print_node_neighbors(reader, i);
    }
    
    // Cleanup
    free(reader->node_index);
    fclose(reader->file);
    free(reader);
    
    return 0;
}