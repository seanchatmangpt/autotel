/*
 * CNS Binary Materializer - Simple Implementation
 * Aligned with existing header definitions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "cns/binary_materializer.h"

// Create a new graph
cns_graph_t* cns_graph_create(uint32_t initial_nodes, uint32_t initial_edges) {
    cns_graph_t* graph = calloc(1, sizeof(cns_graph_t));
    if (!graph) return NULL;
    
    graph->node_capacity = initial_nodes > 0 ? initial_nodes : 16;
    graph->edge_capacity = initial_edges > 0 ? initial_edges : 32;
    graph->data_capacity = 1024;
    
    graph->nodes = calloc(graph->node_capacity, sizeof(cns_node_t));
    graph->edges = calloc(graph->edge_capacity, sizeof(cns_edge_t));
    graph->data_pool = calloc(graph->data_capacity, 1);
    
    if (!graph->nodes || !graph->edges || !graph->data_pool) {
        cns_graph_destroy(graph);
        return NULL;
    }
    
    return graph;
}

// Destroy graph
void cns_graph_destroy(cns_graph_t* graph) {
    if (!graph) return;
    free(graph->nodes);
    free(graph->edges);
    free(graph->data_pool);
    free(graph->node_index);
    free(graph->type_index);
    free(graph);
}

// Create write buffer
cns_write_buffer_t* cns_write_buffer_create(size_t initial_size) {
    cns_write_buffer_t* buf = malloc(sizeof(cns_write_buffer_t));
    if (!buf) return NULL;
    
    buf->capacity = initial_size > 0 ? initial_size : 4096;
    buf->data = malloc(buf->capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->size = 0;
    buf->position = 0;
    buf->checksum = 0;
    return buf;
}

// Destroy write buffer
void cns_write_buffer_destroy(cns_write_buffer_t* buf) {
    if (!buf) return;
    free(buf->data);
    free(buf);
}

// Ensure buffer capacity
int cns_write_buffer_ensure_capacity(cns_write_buffer_t* buf, size_t required) {
    if (buf->capacity >= required) return CNS_SERIALIZE_OK;
    
    size_t new_capacity = buf->capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }
    
    uint8_t* new_data = realloc(buf->data, new_capacity);
    if (!new_data) return CNS_SERIALIZE_ERROR_MEMORY;
    
    buf->data = new_data;
    buf->capacity = new_capacity;
    return CNS_SERIALIZE_OK;
}

// Write bytes to buffer
int cns_write_buffer_write_bytes(cns_write_buffer_t* buf, const void* data, size_t size) {
    size_t required = buf->position + size;
    int ret = cns_write_buffer_ensure_capacity(buf, required);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    memcpy(buf->data + buf->position, data, size);
    buf->position += size;
    if (buf->position > buf->size) {
        buf->size = buf->position;
    }
    return CNS_SERIALIZE_OK;
}

// Write u32
int cns_write_buffer_write_u32(cns_write_buffer_t* buf, uint32_t value) {
    uint32_t le_value = CNS_TO_LE32(value);
    return cns_write_buffer_write_bytes(buf, &le_value, sizeof(le_value));
}

// Write u16
int cns_write_buffer_write_u16(cns_write_buffer_t* buf, uint16_t value) {
    uint16_t le_value = CNS_TO_LE16(value);
    return cns_write_buffer_write_bytes(buf, &le_value, sizeof(le_value));
}

// Create read buffer
cns_read_buffer_t* cns_read_buffer_create(const void* data, size_t size) {
    cns_read_buffer_t* buf = malloc(sizeof(cns_read_buffer_t));
    if (!buf) return NULL;
    
    buf->data = data;
    buf->size = size;
    buf->position = 0;
    buf->version = CNS_BINARY_VERSION_MAJOR;
    buf->is_big_endian = cns_is_big_endian();
    return buf;
}

// Destroy read buffer
void cns_read_buffer_destroy(cns_read_buffer_t* buf) {
    free(buf);
}

// Read bytes from buffer
int cns_read_buffer_read_bytes(cns_read_buffer_t* buf, void* data, size_t size) {
    if (buf->position + size > buf->size) {
        return CNS_SERIALIZE_ERROR_BOUNDS;
    }
    
    memcpy(data, buf->data + buf->position, size);
    buf->position += size;
    return CNS_SERIALIZE_OK;
}

// Read u32
int cns_read_buffer_read_u32(cns_read_buffer_t* buf, uint32_t* value) {
    uint32_t le_value;
    int ret = cns_read_buffer_read_bytes(buf, &le_value, sizeof(le_value));
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    *value = CNS_FROM_LE32(le_value);
    return CNS_SERIALIZE_OK;
}

// Read u16
int cns_read_buffer_read_u16(cns_read_buffer_t* buf, uint16_t* value) {
    uint16_t le_value;
    int ret = cns_read_buffer_read_bytes(buf, &le_value, sizeof(le_value));
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    *value = CNS_FROM_LE16(le_value);
    return CNS_SERIALIZE_OK;
}

// Write node
int cns_node_write(cns_write_buffer_t* buffer, const cns_node_t* node, const uint8_t* data_pool) {
    int ret;
    
    // Write base element
    ret = cns_write_buffer_write_u32(buffer, node->base.id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u16(buffer, node->base.type);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u16(buffer, node->base.flags);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, node->base.data_offset);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Write node-specific fields
    ret = cns_write_buffer_write_u32(buffer, node->first_out_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, node->first_in_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    return CNS_SERIALIZE_OK;
}

// Read node
int cns_node_read(cns_read_buffer_t* buffer, cns_node_t* node, cns_serialize_ctx_t* ctx) {
    int ret;
    
    // Read base element
    ret = cns_read_buffer_read_u32(buffer, &node->base.id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u16(buffer, &node->base.type);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u16(buffer, &node->base.flags);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &node->base.data_offset);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Read node-specific fields
    ret = cns_read_buffer_read_u32(buffer, &node->first_out_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &node->first_in_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    return CNS_SERIALIZE_OK;
}

// Write edge
int cns_edge_write(cns_write_buffer_t* buffer, const cns_edge_t* edge, const uint8_t* data_pool) {
    int ret;
    
    // Write base element
    ret = cns_write_buffer_write_u32(buffer, edge->base.id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u16(buffer, edge->base.type);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u16(buffer, edge->base.flags);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, edge->base.data_offset);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Write edge-specific fields
    ret = cns_write_buffer_write_u32(buffer, edge->source_id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, edge->target_id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, edge->next_out_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_write_buffer_write_u32(buffer, edge->next_in_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    return CNS_SERIALIZE_OK;
}

// Read edge
int cns_edge_read(cns_read_buffer_t* buffer, cns_edge_t* edge, cns_serialize_ctx_t* ctx) {
    int ret;
    
    // Read base element
    ret = cns_read_buffer_read_u32(buffer, &edge->base.id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u16(buffer, &edge->base.type);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u16(buffer, &edge->base.flags);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &edge->base.data_offset);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Read edge-specific fields
    ret = cns_read_buffer_read_u32(buffer, &edge->source_id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &edge->target_id);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &edge->next_out_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    ret = cns_read_buffer_read_u32(buffer, &edge->next_in_edge);
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    return CNS_SERIALIZE_OK;
}

// Simple CRC32 implementation
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t cns_checksum_scalar(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Simple memcpy
void cns_memcpy_scalar(void* dst, const void* src, size_t size) {
    memcpy(dst, src, size);
}

// Serialize graph
int cns_graph_serialize(const cns_graph_t* graph, cns_write_buffer_t* buffer, uint32_t flags) {
    if (!graph || !buffer) return CNS_SERIALIZE_ERROR_FORMAT;
    
    // Write header
    cns_binary_header_t header = {
        .magic = CNS_BINARY_MAGIC,
        .version_major = CNS_BINARY_VERSION_MAJOR,
        .version_minor = CNS_BINARY_VERSION_MINOR,
        .flags = flags,
        .header_size = sizeof(cns_binary_header_t),
        .node_count = graph->node_count,
        .edge_count = graph->edge_count,
        .data_size = graph->data_size,
        .node_offset = sizeof(cns_binary_header_t),
        .edge_offset = 0,  // Will be updated
        .data_offset = 0,  // Will be updated
        .index_offset = 0, // Will be updated
        .checksum = 0,     // Will be calculated
    };
    
    // Save header position
    size_t header_pos = buffer->position;
    int ret = cns_write_buffer_write_bytes(buffer, &header, sizeof(header));
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Write nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        ret = cns_node_write(buffer, &graph->nodes[i], graph->data_pool);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    // Update edge offset
    header.edge_offset = buffer->position;
    
    // Write edges
    for (uint32_t i = 0; i < graph->edge_count; i++) {
        ret = cns_edge_write(buffer, &graph->edges[i], graph->data_pool);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    // Update data offset
    header.data_offset = buffer->position;
    
    // Write data pool
    if (graph->data_size > 0) {
        ret = cns_write_buffer_write_bytes(buffer, graph->data_pool, graph->data_size);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    // Calculate checksum (skip header checksum field)
    if (flags & CNS_SERIALIZE_FLAG_CHECKSUM) {
        header.checksum = cns_checksum_scalar(buffer->data + sizeof(cns_binary_header_t),
                                             buffer->size - sizeof(cns_binary_header_t));
    }
    
    // Update header with final values
    size_t current_pos = buffer->position;
    buffer->position = header_pos;
    ret = cns_write_buffer_write_bytes(buffer, &header, sizeof(header));
    buffer->position = current_pos;
    
    return ret;
}

// Deserialize graph
int cns_graph_deserialize(cns_graph_t* graph, cns_read_buffer_t* buffer, uint32_t flags) {
    if (!graph || !buffer) return CNS_SERIALIZE_ERROR_FORMAT;
    
    // Read header
    cns_binary_header_t header;
    int ret = cns_read_buffer_read_bytes(buffer, &header, sizeof(header));
    if (ret != CNS_SERIALIZE_OK) return ret;
    
    // Validate header
    if (header.magic != CNS_BINARY_MAGIC) {
        return CNS_SERIALIZE_ERROR_FORMAT;
    }
    
    if (header.version_major > CNS_BINARY_VERSION_MAJOR) {
        return CNS_SERIALIZE_ERROR_VERSION;
    }
    
    // Verify checksum if requested
    if ((flags & CNS_SERIALIZE_FLAG_CHECKSUM) && header.checksum != 0) {
        uint32_t calculated = cns_checksum_scalar(buffer->data + sizeof(header),
                                                 buffer->size - sizeof(header));
        if (calculated != header.checksum) {
            return CNS_SERIALIZE_ERROR_CHECKSUM;
        }
    }
    
    // Initialize graph
    graph->node_count = header.node_count;
    graph->edge_count = header.edge_count;
    graph->data_size = header.data_size;
    graph->flags = header.flags;
    
    // Allocate arrays
    graph->nodes = calloc(header.node_count, sizeof(cns_node_t));
    graph->edges = calloc(header.edge_count, sizeof(cns_edge_t));
    graph->data_pool = malloc(header.data_size);
    
    if (!graph->nodes || !graph->edges || !graph->data_pool) {
        return CNS_SERIALIZE_ERROR_MEMORY;
    }
    
    // Read nodes
    buffer->position = header.node_offset;
    cns_serialize_ctx_t ctx = {.graph = graph};
    
    for (uint32_t i = 0; i < header.node_count; i++) {
        ret = cns_node_read(buffer, &graph->nodes[i], &ctx);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    // Read edges
    buffer->position = header.edge_offset;
    for (uint32_t i = 0; i < header.edge_count; i++) {
        ret = cns_edge_read(buffer, &graph->edges[i], &ctx);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    // Read data pool
    if (header.data_size > 0) {
        buffer->position = header.data_offset;
        ret = cns_read_buffer_read_bytes(buffer, graph->data_pool, header.data_size);
        if (ret != CNS_SERIALIZE_OK) return ret;
    }
    
    return CNS_SERIALIZE_OK;
}

// Error string lookup
const char* cns_serialize_error_string(int error_code) {
    switch (error_code) {
        case CNS_SERIALIZE_OK: return "Success";
        case CNS_SERIALIZE_ERROR_MEMORY: return "Memory allocation failed";
        case CNS_SERIALIZE_ERROR_IO: return "I/O error";
        case CNS_SERIALIZE_ERROR_FORMAT: return "Invalid format";
        case CNS_SERIALIZE_ERROR_VERSION: return "Unsupported version";
        case CNS_SERIALIZE_ERROR_CHECKSUM: return "Checksum mismatch";
        case CNS_SERIALIZE_ERROR_BOUNDS: return "Buffer bounds exceeded";
        case CNS_SERIALIZE_ERROR_COMPRESS: return "Compression error";
        default: return "Unknown error";
    }
}