# Pattern: Network Services and API Integration

## Description
This pattern demonstrates how to expose the 7T Engine as network services, including HTTP APIs, gRPC services, and WebSocket connections for distributed applications and microservices architectures.

## Code Example
```c
#include "c_src/sparql7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <json-c/json.h>

// HTTP response structure
typedef struct {
    int status_code;
    char* content_type;
    char* body;
    size_t body_length;
} HttpResponse;

// HTTP request structure
typedef struct {
    char* method;
    char* path;
    char* query_string;
    char* body;
    size_t body_length;
} HttpRequest;

// Engine service context
typedef struct {
    S7TEngine* engine;
    int server_socket;
    int port;
    int max_clients;
    pthread_mutex_t engine_mutex;
} EngineService;

// Initialize HTTP response
HttpResponse* http_response_create() {
    HttpResponse* response = malloc(sizeof(HttpResponse));
    if (response) {
        response->status_code = 200;
        response->content_type = "application/json";
        response->body = NULL;
        response->body_length = 0;
    }
    return response;
}

// Set HTTP response body
void http_response_set_body(HttpResponse* response, const char* body) {
    if (response->body) {
        free(response->body);
    }
    response->body_length = strlen(body);
    response->body = malloc(response->body_length + 1);
    strcpy(response->body, body);
}

// Destroy HTTP response
void http_response_destroy(HttpResponse* response) {
    if (response) {
        if (response->body) {
            free(response->body);
        }
        free(response);
    }
}

// Parse HTTP request
HttpRequest* parse_http_request(const char* raw_request) {
    HttpRequest* request = malloc(sizeof(HttpRequest));
    if (!request) return NULL;
    
    // Simple HTTP parsing (in production, use a proper HTTP library)
    char* method_end = strchr(raw_request, ' ');
    if (!method_end) {
        free(request);
        return NULL;
    }
    
    size_t method_len = method_end - raw_request;
    request->method = malloc(method_len + 1);
    strncpy(request->method, raw_request, method_len);
    request->method[method_len] = '\0';
    
    char* path_start = method_end + 1;
    char* path_end = strchr(path_start, ' ');
    if (!path_end) {
        free(request->method);
        free(request);
        return NULL;
    }
    
    size_t path_len = path_end - path_start;
    request->path = malloc(path_len + 1);
    strncpy(request->path, path_start, path_len);
    request->path[path_len] = '\0';
    
    // Parse query string
    char* query_start = strchr(request->path, '?');
    if (query_start) {
        *query_start = '\0';
        request->query_string = query_start + 1;
    } else {
        request->query_string = NULL;
    }
    
    request->body = NULL;
    request->body_length = 0;
    
    return request;
}

// Destroy HTTP request
void http_request_destroy(HttpRequest* request) {
    if (request) {
        if (request->method) free(request->method);
        if (request->path) free(request->path);
        if (request->body) free(request->body);
        free(request);
    }
}

// Create JSON response
char* create_json_response(int success, const char* message, json_object* data) {
    json_object* response = json_object_new_object();
    json_object* success_obj = json_object_new_boolean(success);
    json_object* message_obj = json_object_new_string(message);
    
    json_object_object_add(response, "success", success_obj);
    json_object_object_add(response, "message", message_obj);
    
    if (data) {
        json_object_object_add(response, "data", data);
    }
    
    const char* json_string = json_object_to_json_string(response);
    char* result = malloc(strlen(json_string) + 1);
    strcpy(result, json_string);
    
    json_object_put(response);
    return result;
}

// Handle triple addition request
HttpResponse* handle_add_triple(EngineService* service, HttpRequest* request) {
    HttpResponse* response = http_response_create();
    
    if (!request->body) {
        http_response_set_body(response, create_json_response(0, "No request body", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* request_json = json_tokener_parse(request->body);
    if (!request_json) {
        http_response_set_body(response, create_json_response(0, "Invalid JSON", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* s_obj, *p_obj, *o_obj;
    if (!json_object_object_get_ex(request_json, "subject", &s_obj) ||
        !json_object_object_get_ex(request_json, "predicate", &p_obj) ||
        !json_object_object_get_ex(request_json, "object", &o_obj)) {
        http_response_set_body(response, create_json_response(0, "Missing subject, predicate, or object", NULL));
        response->status_code = 400;
        json_object_put(request_json);
        return response;
    }
    
    uint32_t s = json_object_get_int(s_obj);
    uint32_t p = json_object_get_int(p_obj);
    uint32_t o = json_object_get_int(o_obj);
    
    pthread_mutex_lock(&service->engine_mutex);
    s7t_add_triple(service->engine, s, p, o);
    pthread_mutex_unlock(&service->engine_mutex);
    
    json_object* data = json_object_new_object();
    json_object* triple = json_object_new_object();
    json_object_object_add(triple, "subject", json_object_new_int(s));
    json_object_object_add(triple, "predicate", json_object_new_int(p));
    json_object_object_add(triple, "object", json_object_new_int(o));
    json_object_object_add(data, "triple", triple);
    
    http_response_set_body(response, create_json_response(1, "Triple added successfully", data));
    json_object_put(request_json);
    
    return response;
}

// Handle pattern matching request
HttpResponse* handle_ask_pattern(EngineService* service, HttpRequest* request) {
    HttpResponse* response = http_response_create();
    
    if (!request->body) {
        http_response_set_body(response, create_json_response(0, "No request body", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* request_json = json_tokener_parse(request->body);
    if (!request_json) {
        http_response_set_body(response, create_json_response(0, "Invalid JSON", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* s_obj, *p_obj, *o_obj;
    if (!json_object_object_get_ex(request_json, "subject", &s_obj) ||
        !json_object_object_get_ex(request_json, "predicate", &p_obj) ||
        !json_object_object_get_ex(request_json, "object", &o_obj)) {
        http_response_set_body(response, create_json_response(0, "Missing subject, predicate, or object", NULL));
        response->status_code = 400;
        json_object_put(request_json);
        return response;
    }
    
    uint32_t s = json_object_get_int(s_obj);
    uint32_t p = json_object_get_int(p_obj);
    uint32_t o = json_object_get_int(o_obj);
    
    pthread_mutex_lock(&service->engine_mutex);
    int result = s7t_ask_pattern(service->engine, s, p, o);
    pthread_mutex_unlock(&service->engine_mutex);
    
    json_object* data = json_object_new_object();
    json_object_object_add(data, "found", json_object_new_boolean(result));
    json_object_object_add(data, "pattern", request_json);
    
    http_response_set_body(response, create_json_response(1, "Pattern query completed", data));
    
    return response;
}

// Handle batch operations request
HttpResponse* handle_batch_operations(EngineService* service, HttpRequest* request) {
    HttpResponse* response = http_response_create();
    
    if (!request->body) {
        http_response_set_body(response, create_json_response(0, "No request body", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* request_json = json_tokener_parse(request->body);
    if (!request_json) {
        http_response_set_body(response, create_json_response(0, "Invalid JSON", NULL));
        response->status_code = 400;
        return response;
    }
    
    json_object* patterns_array;
    if (!json_object_object_get_ex(request_json, "patterns", &patterns_array) ||
        !json_object_is_type(patterns_array, json_type_array)) {
        http_response_set_body(response, create_json_response(0, "Missing or invalid patterns array", NULL));
        response->status_code = 400;
        json_object_put(request_json);
        return response;
    }
    
    int array_length = json_object_array_length(patterns_array);
    if (array_length == 0) {
        http_response_set_body(response, create_json_response(0, "Empty patterns array", NULL));
        response->status_code = 400;
        json_object_put(request_json);
        return response;
    }
    
    TriplePattern* patterns = malloc(array_length * sizeof(TriplePattern));
    int* results = malloc(array_length * sizeof(int));
    
    if (!patterns || !results) {
        free(patterns);
        free(results);
        http_response_set_body(response, create_json_response(0, "Memory allocation failed", NULL));
        response->status_code = 500;
        json_object_put(request_json);
        return response;
    }
    
    // Parse patterns
    for (int i = 0; i < array_length; i++) {
        json_object* pattern_obj = json_object_array_get_idx(patterns_array, i);
        json_object* s_obj, *p_obj, *o_obj;
        
        if (!json_object_object_get_ex(pattern_obj, "subject", &s_obj) ||
            !json_object_object_get_ex(pattern_obj, "predicate", &p_obj) ||
            !json_object_object_get_ex(pattern_obj, "object", &o_obj)) {
            free(patterns);
            free(results);
            http_response_set_body(response, create_json_response(0, "Invalid pattern format", NULL));
            response->status_code = 400;
            json_object_put(request_json);
            return response;
        }
        
        patterns[i].s = json_object_get_int(s_obj);
        patterns[i].p = json_object_get_int(p_obj);
        patterns[i].o = json_object_get_int(o_obj);
    }
    
    // Execute batch operation
    pthread_mutex_lock(&service->engine_mutex);
    s7t_ask_batch(service->engine, patterns, results, array_length);
    pthread_mutex_unlock(&service->engine_mutex);
    
    // Create response data
    json_object* data = json_object_new_object();
    json_object* results_array = json_object_new_array();
    
    for (int i = 0; i < array_length; i++) {
        json_object* result_obj = json_object_new_object();
        json_object* pattern_obj = json_object_new_object();
        json_object_object_add(pattern_obj, "subject", json_object_new_int(patterns[i].s));
        json_object_object_add(pattern_obj, "predicate", json_object_new_int(patterns[i].p));
        json_object_object_add(pattern_obj, "object", json_object_new_int(patterns[i].o));
        json_object_object_add(result_obj, "pattern", pattern_obj);
        json_object_object_add(result_obj, "found", json_object_new_boolean(results[i]));
        json_object_array_add(results_array, result_obj);
    }
    
    json_object_object_add(data, "results", results_array);
    json_object_object_add(data, "count", json_object_new_int(array_length));
    
    http_response_set_body(response, create_json_response(1, "Batch operation completed", data));
    
    free(patterns);
    free(results);
    json_object_put(request_json);
    
    return response;
}

// Route HTTP requests
HttpResponse* route_request(EngineService* service, HttpRequest* request) {
    if (strcmp(request->method, "POST") == 0) {
        if (strcmp(request->path, "/api/triple") == 0) {
            return handle_add_triple(service, request);
        } else if (strcmp(request->path, "/api/pattern") == 0) {
            return handle_ask_pattern(service, request);
        } else if (strcmp(request->path, "/api/batch") == 0) {
            return handle_batch_operations(service, request);
        }
    } else if (strcmp(request->method, "GET") == 0) {
        if (strcmp(request->path, "/api/health") == 0) {
            HttpResponse* response = http_response_create();
            http_response_set_body(response, create_json_response(1, "Service healthy", NULL));
            return response;
        }
    }
    
    // 404 Not Found
    HttpResponse* response = http_response_create();
    response->status_code = 404;
    http_response_set_body(response, create_json_response(0, "Endpoint not found", NULL));
    return response;
}

// Send HTTP response
void send_http_response(int client_socket, HttpResponse* response) {
    char status_line[256];
    char headers[512];
    
    const char* status_text = (response->status_code == 200) ? "OK" :
                             (response->status_code == 400) ? "Bad Request" :
                             (response->status_code == 404) ? "Not Found" :
                             (response->status_code == 500) ? "Internal Server Error" : "Unknown";
    
    snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", response->status_code, status_text);
    snprintf(headers, sizeof(headers), 
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "\r\n",
             response->content_type, response->body_length);
    
    send(client_socket, status_line, strlen(status_line), 0);
    send(client_socket, headers, strlen(headers), 0);
    
    if (response->body && response->body_length > 0) {
        send(client_socket, response->body, response->body_length, 0);
    }
}

// Client connection handler
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    EngineService* service = (EngineService*)((char*)arg - offsetof(EngineService, server_socket));
    
    char buffer[4096];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        HttpRequest* request = parse_http_request(buffer);
        if (request) {
            HttpResponse* response = route_request(service, request);
            send_http_response(client_socket, response);
            http_response_destroy(response);
            http_request_destroy(request);
        }
    }
    
    close(client_socket);
    free(arg);
    return NULL;
}

// Start HTTP server
int start_http_server(EngineService* service) {
    service->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (service->server_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    int opt = 1;
    setsockopt(service->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(service->port);
    
    if (bind(service->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return -1;
    }
    
    if (listen(service->server_socket, service->max_clients) < 0) {
        perror("Listen failed");
        return -1;
    }
    
    printf("HTTP server started on port %d\n", service->port);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int* client_socket = malloc(sizeof(int));
        *client_socket = accept(service->server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (*client_socket < 0) {
            perror("Accept failed");
            free(client_socket);
            continue;
        }
        
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_socket) != 0) {
            perror("Thread creation failed");
            close(*client_socket);
            free(client_socket);
        } else {
            pthread_detach(thread);
        }
    }
    
    return 0;
}

// Initialize engine service
EngineService* engine_service_create(int port, size_t max_s, size_t max_p, size_t max_o) {
    EngineService* service = malloc(sizeof(EngineService));
    if (!service) return NULL;
    
    service->engine = s7t_create(max_s, max_p, max_o);
    if (!service->engine) {
        free(service);
        return NULL;
    }
    
    service->port = port;
    service->max_clients = 10;
    pthread_mutex_init(&service->engine_mutex, NULL);
    
    return service;
}

// Destroy engine service
void engine_service_destroy(EngineService* service) {
    if (service) {
        if (service->engine) {
            s7t_destroy(service->engine);
        }
        pthread_mutex_destroy(&service->engine_mutex);
        free(service);
    }
}

int main() {
    EngineService* service = engine_service_create(8080, 100000, 1000, 100000);
    if (!service) {
        fprintf(stderr, "Failed to create engine service\n");
        return 1;
    }
    
    printf("7T Engine HTTP API Server\n");
    printf("Available endpoints:\n");
    printf("  POST /api/triple - Add a triple\n");
    printf("  POST /api/pattern - Query a pattern\n");
    printf("  POST /api/batch - Batch operations\n");
    printf("  GET /api/health - Health check\n");
    
    int result = start_http_server(service);
    
    engine_service_destroy(service);
    return result;
}
```

## Tips
- Use proper HTTP libraries like libcurl or libmicrohttpd for production.
- Implement authentication and rate limiting for security.
- Use connection pooling for better performance.
- Consider gRPC for high-performance RPC communication.
- Implement proper error handling and logging. 