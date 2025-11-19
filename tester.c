#include "consistent_hashing.h"
#include <stdio.h>

int main(int argc, char **argv) {
    // Define 10 different key counts to test the simulation against.
    int key_counts[] = {100, 500, 1000, 5000, 10000, 25000, 50000, 100000, 250000, 500000};
    int num_cases = sizeof(key_counts) / sizeof(key_counts[0]);

    // We will output a JSON array containing the results of each simulation run.
    printf("[\n");

    for (int i = 0; i < num_cases; i++) {
        int nkeys = key_counts[i];
        SystemState sys = {0};

        simulate(&sys, nkeys);

        // Clean up all memory allocated for the SystemState.
        if (sys.ring_root) free_bst(sys.ring_root);
        if (sys.plain_indices) free(sys.plain_indices);
        for (int j = 0; j < sys.node_count; j++) {
            if (sys.nodes[j].vpos) free(sys.nodes[j].vpos);
        }

        if (i < num_cases - 1) {
            printf(",\n");
        }
    }

    printf("\n]\n"); // End of the JSON array
    return 0;
}