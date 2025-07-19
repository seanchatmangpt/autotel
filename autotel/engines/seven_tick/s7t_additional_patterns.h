#ifndef S7T_ADDITIONAL_PATTERNS_H
#define S7T_ADDITIONAL_PATTERNS_H

#include <stdint.h>
#include <stddef.h>

// ============================================================================
// ADDITIONAL GANG OF FOUR PATTERNS - 7-TICK OPTIMIZED
// ============================================================================

// 10. TEMPLATE METHOD PATTERN
typedef struct
{
  uint32_t algorithm_id;
  uint32_t data[4];
  uint32_t flags;
} S7T_TemplateContext;

typedef void (*S7T_TemplateStep)(S7T_TemplateContext *ctx);

typedef struct
{
  S7T_TemplateStep steps[8];
  uint32_t step_count;
} S7T_TemplateMethod;

static inline void s7t_execute_template_method(S7T_TemplateMethod *tmpl, S7T_TemplateContext *ctx)
{
  for (uint32_t i = 0; i < tmpl->step_count; i++)
  {
    tmpl->steps[i](ctx);
  }
}

// 11. PROTOTYPE PATTERN
typedef struct
{
  uint32_t prototype_id;
  uint32_t data[8];
  uint32_t attributes;
} S7T_Prototype;

typedef struct
{
  S7T_Prototype prototypes[16];
  uint32_t prototype_count;
} S7T_PrototypeRegistry;

static inline void s7t_prototype_clone(S7T_Prototype *dest, const S7T_Prototype *src)
{
  dest->prototype_id = src->prototype_id;
  for (int i = 0; i < 8; i++)
  {
    dest->data[i] = src->data[i];
  }
  dest->attributes = src->attributes;
}

static inline S7T_Prototype *s7t_prototype_get(S7T_PrototypeRegistry *reg, uint32_t id)
{
  for (uint32_t i = 0; i < reg->prototype_count; i++)
  {
    if (reg->prototypes[i].prototype_id == id)
    {
      return &reg->prototypes[i];
    }
  }
  return NULL;
}

// 12. BRIDGE PATTERN
typedef struct
{
  uint32_t abstraction_id;
  uint32_t implementation_id;
  uint32_t data;
} S7T_Bridge;

typedef uint32_t (*S7T_Implementation)(uint32_t data);

typedef struct
{
  S7T_Implementation implementations[8];
  uint32_t impl_count;
} S7T_ImplementationRegistry;

static inline uint32_t s7t_bridge_execute(S7T_Bridge *bridge, S7T_ImplementationRegistry *reg)
{
  if (bridge->implementation_id < reg->impl_count)
  {
    return reg->implementations[bridge->implementation_id](bridge->data);
  }
  return 0;
}

// 13. COMPOSITE PATTERN
typedef enum
{
  S7T_NODE_LEAF = 0,
  S7T_NODE_COMPOSITE = 1,
  S7T_NODE_TYPE_COUNT
} S7T_NodeType;

typedef struct
{
  S7T_NodeType type;
  uint32_t node_id;
  uint32_t data;
  uint32_t child_count;
  uint32_t children[4]; // Fixed-size for 7-tick performance
} S7T_CompositeNode;

typedef struct
{
  S7T_CompositeNode nodes[64];
  uint32_t node_count;
} S7T_CompositeTree;

static inline uint32_t s7t_composite_operation(S7T_CompositeTree *tree, uint32_t node_id)
{
  if (node_id >= tree->node_count)
    return 0;

  S7T_CompositeNode *node = &tree->nodes[node_id];
  uint32_t result = node->data;

  if (node->type == S7T_NODE_COMPOSITE)
  {
    for (uint32_t i = 0; i < node->child_count && i < 4; i++)
    {
      result += s7t_composite_operation(tree, node->children[i]);
    }
  }

  return result;
}

// 14. ADAPTER PATTERN
typedef struct
{
  uint32_t legacy_data[4];
  uint32_t legacy_flags;
} S7T_LegacyInterface;

typedef struct
{
  uint32_t modern_data[4];
  uint32_t modern_flags;
} S7T_ModernInterface;

typedef struct
{
  S7T_LegacyInterface *legacy;
  S7T_ModernInterface *modern;
} S7T_Adapter;

static inline void s7t_adapter_convert_legacy_to_modern(S7T_Adapter *adapter)
{
  for (int i = 0; i < 4; i++)
  {
    adapter->modern->modern_data[i] = adapter->legacy->legacy_data[i];
  }
  adapter->modern->modern_flags = adapter->legacy->legacy_flags;
}

static inline void s7t_adapter_convert_modern_to_legacy(S7T_Adapter *adapter)
{
  for (int i = 0; i < 4; i++)
  {
    adapter->legacy->legacy_data[i] = adapter->modern->modern_data[i];
  }
  adapter->legacy->legacy_flags = adapter->modern->modern_flags;
}

// 15. FACADE PATTERN
typedef struct
{
  uint32_t subsystem_a_data;
  uint32_t subsystem_b_data;
  uint32_t subsystem_c_data;
  uint32_t flags;
} S7T_Facade;

static inline void s7t_facade_operation(S7T_Facade *facade)
{
  // Simplified subsystem operations for 7-tick performance
  facade->subsystem_a_data = facade->subsystem_a_data * 2;
  facade->subsystem_b_data = facade->subsystem_b_data + 1;
  facade->subsystem_c_data = facade->subsystem_c_data << 1;
  facade->flags = 0xABCD;
}

// 16. PROXY PATTERN
typedef struct
{
  uint32_t real_object_id;
  uint32_t cached_data[4];
  uint32_t cache_valid;
  uint32_t access_count;
} S7T_Proxy;

typedef struct
{
  uint32_t object_id;
  uint32_t data[4];
} S7T_RealObject;

typedef struct
{
  S7T_RealObject objects[16];
  uint32_t object_count;
} S7T_RealObjectRegistry;

static inline uint32_t *s7t_proxy_get_data(S7T_Proxy *proxy, S7T_RealObjectRegistry *registry)
{
  proxy->access_count++;

  if (!proxy->cache_valid)
  {
    // Load from real object
    for (uint32_t i = 0; i < registry->object_count; i++)
    {
      if (registry->objects[i].object_id == proxy->real_object_id)
      {
        for (int j = 0; j < 4; j++)
        {
          proxy->cached_data[j] = registry->objects[i].data[j];
        }
        proxy->cache_valid = 1;
        break;
      }
    }
  }

  return proxy->cached_data;
}

// 17. CHAIN OF RESPONSIBILITY PATTERN
typedef enum
{
  S7T_HANDLER_LOW = 0,
  S7T_HANDLER_MEDIUM = 1,
  S7T_HANDLER_HIGH = 2,
  S7T_HANDLER_CRITICAL = 3,
  S7T_HANDLER_COUNT
} S7T_HandlerLevel;

typedef struct
{
  S7T_HandlerLevel level;
  uint32_t handler_id;
  uint32_t data;
  uint32_t processed;
} S7T_Request;

typedef uint32_t (*S7T_Handler)(S7T_Request *req);

typedef struct
{
  S7T_Handler handlers[S7T_HANDLER_COUNT];
  uint32_t handler_count;
} S7T_ChainOfResponsibility;

static inline uint32_t s7t_chain_process(S7T_ChainOfResponsibility *chain, S7T_Request *req)
{
  for (uint32_t i = req->level; i < chain->handler_count; i++)
  {
    uint32_t result = chain->handlers[i](req);
    if (result != 0)
    {
      req->processed = 1;
      return result;
    }
  }
  return 0;
}

// 18. INTERPRETER PATTERN
typedef enum
{
  S7T_EXPR_LITERAL = 0,
  S7T_EXPR_ADD = 1,
  S7T_EXPR_SUB = 2,
  S7T_EXPR_MUL = 3,
  S7T_EXPR_TYPE_COUNT
} S7T_ExpressionType;

typedef struct
{
  S7T_ExpressionType type;
  uint32_t value;
  uint32_t left_child;
  uint32_t right_child;
} S7T_Expression;

typedef struct
{
  S7T_Expression expressions[32];
  uint32_t expr_count;
} S7T_Interpreter;

static inline uint32_t s7t_interpreter_evaluate(S7T_Interpreter *interp, uint32_t expr_id)
{
  if (expr_id >= interp->expr_count)
    return 0;

  S7T_Expression *expr = &interp->expressions[expr_id];

  switch (expr->type)
  {
  case S7T_EXPR_LITERAL:
    return expr->value;
  case S7T_EXPR_ADD:
    return s7t_interpreter_evaluate(interp, expr->left_child) +
           s7t_interpreter_evaluate(interp, expr->right_child);
  case S7T_EXPR_SUB:
    return s7t_interpreter_evaluate(interp, expr->left_child) -
           s7t_interpreter_evaluate(interp, expr->right_child);
  case S7T_EXPR_MUL:
    return s7t_interpreter_evaluate(interp, expr->left_child) *
           s7t_interpreter_evaluate(interp, expr->right_child);
  default:
    return 0;
  }
}

// 19. MEDIATOR PATTERN
typedef struct
{
  uint32_t colleague_id;
  uint32_t data;
  uint32_t flags;
} S7T_Colleague;

typedef struct
{
  S7T_Colleague colleagues[8];
  uint32_t colleague_count;
  uint32_t mediator_data;
} S7T_Mediator;

static inline void s7t_mediator_notify(S7T_Mediator *mediator, uint32_t colleague_id, uint32_t data)
{
  for (uint32_t i = 0; i < mediator->colleague_count; i++)
  {
    if (mediator->colleagues[i].colleague_id != colleague_id)
    {
      mediator->colleagues[i].data = data;
      mediator->colleagues[i].flags |= 0x01; // Mark as updated
    }
  }
  mediator->mediator_data = data;
}

// 20. MEMENTO PATTERN
typedef struct
{
  uint32_t state_data[4];
  uint32_t timestamp;
} S7T_Memento;

typedef struct
{
  S7T_Memento mementos[16];
  uint32_t memento_count;
  uint32_t current_index;
} S7T_Caretaker;

static inline void s7t_caretaker_save_state(S7T_Caretaker *caretaker, const uint32_t *state_data)
{
  if (caretaker->current_index < 16)
  {
    S7T_Memento *memento = &caretaker->mementos[caretaker->current_index];
    for (int i = 0; i < 4; i++)
    {
      memento->state_data[i] = state_data[i];
    }
    memento->timestamp = caretaker->current_index;
    caretaker->current_index++;
    caretaker->memento_count++;
  }
}

static inline const uint32_t *s7t_caretaker_restore_state(S7T_Caretaker *caretaker, uint32_t index)
{
  if (index < caretaker->memento_count)
  {
    return caretaker->mementos[index].state_data;
  }
  return NULL;
}

// ============================================================================
// PATTERN CONSTANTS AND MACROS
// ============================================================================

#define S7T_TEMPLATE_FLAG_VALIDATED (1 << 0)
#define S7T_TEMPLATE_FLAG_PROCESSED (1 << 1)
#define S7T_TEMPLATE_FLAG_COMPLETED (1 << 2)

#define S7T_PROTOTYPE_FLAG_ACTIVE (1 << 0)
#define S7T_PROTOTYPE_FLAG_MODIFIED (1 << 1)

#define S7T_BRIDGE_FLAG_CONNECTED (1 << 0)
#define S7T_BRIDGE_FLAG_ACTIVE (1 << 1)

#define S7T_COMPOSITE_FLAG_VISITED (1 << 0)
#define S7T_COMPOSITE_FLAG_PROCESSED (1 << 1)

#define S7T_ADAPTER_FLAG_CONVERTED (1 << 0)
#define S7T_ADAPTER_FLAG_DIRTY (1 << 1)

#define S7T_FACADE_FLAG_INITIALIZED (1 << 0)
#define S7T_FACADE_FLAG_OPERATIONAL (1 << 1)

#define S7T_PROXY_FLAG_CACHED (1 << 0)
#define S7T_PROXY_FLAG_DIRTY (1 << 1)

#define S7T_CHAIN_FLAG_PROCESSED (1 << 0)
#define S7T_CHAIN_FLAG_HANDLED (1 << 1)

#define S7T_INTERPRETER_FLAG_EVALUATED (1 << 0)
#define S7T_INTERPRETER_FLAG_CACHED (1 << 1)

#define S7T_MEDIATOR_FLAG_UPDATED (1 << 0)
#define S7T_MEDIATOR_FLAG_NOTIFIED (1 << 1)

#define S7T_MEMENTO_FLAG_VALID (1 << 0)
#define S7T_MEMENTO_FLAG_ACTIVE (1 << 1)

// ============================================================================
// PATTERN INITIALIZATION HELPERS
// ============================================================================

static inline void s7t_template_method_init(S7T_TemplateMethod *tmpl)
{
  tmpl->step_count = 0;
  for (int i = 0; i < 8; i++)
  {
    tmpl->steps[i] = NULL;
  }
}

static inline void s7t_prototype_registry_init(S7T_PrototypeRegistry *reg)
{
  reg->prototype_count = 0;
}

static inline void s7t_implementation_registry_init(S7T_ImplementationRegistry *reg)
{
  reg->impl_count = 0;
}

static inline void s7t_composite_tree_init(S7T_CompositeTree *tree)
{
  tree->node_count = 0;
}

static inline void s7t_caretaker_init(S7T_Caretaker *caretaker)
{
  caretaker->memento_count = 0;
  caretaker->current_index = 0;
}

#endif // S7T_ADDITIONAL_PATTERNS_H