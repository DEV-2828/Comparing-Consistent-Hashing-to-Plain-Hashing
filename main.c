#include "consistent_hashing.h"
int main(int argc, char **argv){
    SystemState sys = {0};
    int nkeys = 10000;
    if (argc > 1) nkeys = atoi(argv[1]);
    simulate(&sys, nkeys);
    if (sys.ring_root) free_bst(sys.ring_root);
    if (sys.plain_indices) free(sys.plain_indices);
    for (int i = 0; i < sys.node_count; i++) {
        if (sys.nodes[i].vpos) free(sys.nodes[i].vpos);
    }
    return 0;
}
