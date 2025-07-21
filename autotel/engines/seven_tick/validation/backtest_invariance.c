#include "cns/cns_weaver.h"
#include "cns/bitactor.h"

cns_weave_op_t backtest_sequence[] = {
    CNS_OP(OP_TRINITY_INIT, NULL),
    CNS_OP(OP_8M_ALLOC, NULL, 8192),
    CNS_OP(OP_8T_EXECUTE, NULL, 0xAABBCCDD, 0x11223344),
    CNS_OP(OP_8H_COGNITIVE_CYCLE, NULL, 0xCAFEBABE),
    CNS_OP(OP_TRINITY_EXECUTE, NULL, 0xDEADBEEF),
    CNS_OP(OP_GRAPH_INIT, NULL),
    CNS_OP(OP_GRAPH_ADD_TRIPLE, NULL, 0x1, 0x2, 0x3),
    CNS_OP(OP_SHACL_VALIDATE, NULL, 0x4),
    CNS_OP(OP_SPARQL_QUERY, NULL, 0x5)};

CNS_PERMUTATION_DEFINE(backtest_invariance, backtest_sequence);

int main()
{
  cns_weaver_state_t state;
  cns_weaver_init(&state);
  // Register all BitActor operations as in main system...
  cns_weaver_run(&backtest_invariance_weave);
  cns_weaver_cleanup(&state);
  return 0;
}