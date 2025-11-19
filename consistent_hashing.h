#ifndef CONSISTENT_HASHING_H
#define CONSISTENT_HASHING_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_NODES 1024
#define MAX_KEY_LEN 64

typedef struct BSTNode{
    unsigned long long pos;
    int node_index;
    struct BSTNode *left;
    struct BSTNode *right;
}BSTNode;

typedef struct{
    char id[64];
    int vnode_count;
    unsigned long long *vpos;
    int active;
}node_t;

typedef struct {
    BSTNode *ring_root;
    node_t nodes[MAX_NODES];
    int node_count;
    int *plain_indices;
    int plain_count;
} SystemState;

unsigned long long djb2_hash_64(const char *s);
BSTNode* bst_min(BSTNode* n);
BSTNode* bst_insert(BSTNode* root, unsigned long long pos, int node_index);
BSTNode* bst_delete_one(BSTNode* root, unsigned long long pos, int node_index);
BSTNode* bst_find_successor_node(BSTNode* root, unsigned long long key);
int node_index_by_id(SystemState *sys, const char *id);
int plain_find_insert_index(SystemState *sys, const char *id);
void plain_insert_index(SystemState *sys, int node_index);
void plain_remove_index(SystemState *sys, int node_index);
void add_node(SystemState *sys, const char *node_id, int vnodes);
void remove_node(SystemState *sys, const char *node_id);
int get_node_for_key_consistent(SystemState *sys, const char *key);
int get_node_for_key_plain(SystemState *sys, const char *key);
double compute_remap_fraction(int *before, int *after, int nkeys);
void free_bst(BSTNode* root);
void simulate(SystemState *sys, int nkeys);

void getLoadDistribution(SystemState *sys, char **keys, int nkeys);
void getKeysForNode(SystemState *sys, const char *nodeID, char **keys, int nkeys);

#endif
