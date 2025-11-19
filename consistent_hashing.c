#include "consistent_hashing.h"

unsigned long long djb2_hash_64(const char *s){
    unsigned long long hash = 5381;
    int c;
    while ((c = *s++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

BSTNode* bst_min(BSTNode* n) {
    if (!n) return NULL;
    while (n->left) n = n->left;
    return n;
}

BSTNode* bst_insert(BSTNode* root, unsigned long long pos, int node_index){
    if (!root) {
        BSTNode *n = malloc(sizeof(BSTNode));
        if (!n) {
            fprintf(stderr, "Memory allocation failed in bst_insert()\n");
            exit(EXIT_FAILURE);
        }
        n->pos = pos;
        n->node_index = node_index;
        n->left = n->right = NULL;
        return n;
    }
    if (pos < root->pos) root->left = bst_insert(root->left, pos, node_index);
    else if (pos > root->pos) root->right = bst_insert(root->right, pos, node_index);
    else {
        if (node_index < root->node_index) root->left = bst_insert(root->left, pos, node_index);
        else root->right = bst_insert(root->right, pos, node_index);
    }
    return root;
}

BSTNode* bst_delete_one(BSTNode* root, unsigned long long pos, int node_index) {
    if (!root) return NULL;
    if (pos < root->pos) root->left = bst_delete_one(root->left, pos, node_index);
    else if (pos > root->pos) root->right = bst_delete_one(root->right, pos, node_index);
    else {
        if (root->node_index != node_index) {
            root->left = bst_delete_one(root->left, pos, node_index);
            root->right = bst_delete_one(root->right, pos, node_index);
            return root;
        }
        if (!root->left) {
            BSTNode* r = root->right;
            free(root);
            return r;
        } else if (!root->right) {
            BSTNode* l = root->left;
            free(root);
            return l;
        } else {
            BSTNode* succ = bst_min(root->right);
            root->pos = succ->pos;
            root->node_index = succ->node_index;
            root->right = bst_delete_one(root->right, succ->pos, succ->node_index);
        }
    }
    return root;
}

BSTNode* bst_find_successor_node(BSTNode* root, unsigned long long key){
    BSTNode *succ = NULL;
    while (root) {
        if (root->pos >= key) {
            succ = root;
            root = root->left;
        } else root = root->right;
    }
    return succ;
}

int node_index_by_id(SystemState *sys, const char *id) {
    for (int i = 0; i < sys->node_count; i++)
        if (sys->nodes[i].active && strcmp(sys->nodes[i].id, id) == 0)
            return i;
    return -1;
}

int plain_find_insert_index(SystemState *sys, const char *id) {
    int lo = 0, hi = sys->plain_count;
    while (lo < hi) {
        int mid = (lo + hi) >> 1;
        if (strcmp(sys->nodes[sys->plain_indices[mid]].id, id) < 0) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

void plain_insert_index(SystemState *sys, int node_index) {
    if (sys->plain_count == 0) {
        sys->plain_indices = malloc(sizeof(int));
        if (!sys->plain_indices) {
            fprintf(stderr, "Memory allocation failed in plain_insert_index()\n");
            exit(EXIT_FAILURE);
        }
        sys->plain_indices[0] = node_index;
        sys->plain_count = 1;
        return;
    }
    int idx = plain_find_insert_index(sys, sys->nodes[node_index].id);
    int *tmp = realloc(sys->plain_indices, sizeof(int) * (sys->plain_count + 1));
    if (!tmp) {
        fprintf(stderr, "Memory reallocation failed in plain_insert_index()\n");
        free(sys->plain_indices);
        exit(EXIT_FAILURE);
    }
    sys->plain_indices = tmp;
    memmove(&sys->plain_indices[idx + 1], &sys->plain_indices[idx], sizeof(int) * (sys->plain_count - idx));
    sys->plain_indices[idx] = node_index;
    sys->plain_count++;
}

void plain_remove_index(SystemState *sys, int node_index) {
    int idx = -1;
    for (int i = 0; i < sys->plain_count; i++)
        if (sys->plain_indices[i] == node_index) { idx = i; break; }
    if (idx == -1) return;
    memmove(&sys->plain_indices[idx], &sys->plain_indices[idx + 1], sizeof(int) * (sys->plain_count - idx - 1));
    sys->plain_count--;
    if (sys->plain_count == 0) { free(sys->plain_indices); sys->plain_indices = NULL; }
    else {
        int *tmp = realloc(sys->plain_indices, sizeof(int) * sys->plain_count);
        if (!tmp && sys->plain_count > 0) {
            fprintf(stderr, "Memory reallocation failed in plain_remove_index()\n");
            exit(EXIT_FAILURE);
        }
        sys->plain_indices = tmp;
    }
}

void add_node(SystemState *sys, const char *node_id, int vnodes) {
    if (vnodes <= 0) vnodes = 1;
    if (sys->node_count >= MAX_NODES) return;
    int existing = node_index_by_id(sys, node_id);
    if (existing != -1) return;
    int idx = sys->node_count++;
    strncpy(sys->nodes[idx].id, node_id, sizeof(sys->nodes[idx].id) - 1);
    sys->nodes[idx].id[63] = '\0';
    sys->nodes[idx].vnode_count = vnodes;
    sys->nodes[idx].vpos = malloc(sizeof(unsigned long long) * vnodes);
    if (!sys->nodes[idx].vpos) {
        fprintf(stderr, "Memory allocation failed for vpos in add_node()\n");
        exit(EXIT_FAILURE);
    }
    sys->nodes[idx].active = 1;
    for (int i =0; i <vnodes; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s#%d", node_id, i);
        unsigned long long pos = djb2_hash_64(buf);
        sys->nodes[idx].vpos[i] = pos;
        sys->ring_root = bst_insert(sys->ring_root, pos, idx);
    }
    plain_insert_index(sys, idx);
}

void remove_node(SystemState *sys, const char *node_id) {
    int idx = node_index_by_id(sys, node_id);
    if (idx == -1) return;
    for (int i = 0; i < sys->nodes[idx].vnode_count; i++)
        sys->ring_root = bst_delete_one(sys->ring_root, sys->nodes[idx].vpos[i], idx);
    free(sys->nodes[idx].vpos);
    sys->nodes[idx].vpos = NULL;
    sys->nodes[idx].active = 0;
    plain_remove_index(sys, idx);
}

int get_node_for_key_consistent(SystemState *sys, const char *key) {
    if (!sys->ring_root) return -1;
    unsigned long long h = djb2_hash_64(key);
    BSTNode *succ = bst_find_successor_node(sys->ring_root, h);
    if (!succ) succ = bst_min(sys->ring_root);
    return succ->node_index;
}

int get_node_for_key_plain(SystemState *sys, const char *key) {
    if (sys->plain_count == 0) return -1;
    unsigned long long h = djb2_hash_64(key);
    int idx = (int)(h % (unsigned long long)sys->plain_count);
    return sys->plain_indices[idx];
}

double compute_remap_fraction(int *before, int *after, int nkeys) {
    int changed = 0;
    for (int i = 0; i < nkeys; i++)
        if (before[i] != after[i]) changed++;
    return (double)changed / nkeys;
}

void free_bst(BSTNode* root) {
    if (!root) return;
    free_bst(root->left);
    free_bst(root->right);
    free(root);
}

void getLoadDistribution(SystemState *sys, char **keys, int nkeys) {
    if (sys->node_count == 0 || nkeys == 0) {
        printf("No nodes or keys to distribute.\n");
        return;
    }


    int *load = calloc(sys->node_count, sizeof(int));
    if (!load) {
        fprintf(stderr, "Memory allocation failed in getLoadDistribution()\n");
        return;
    }

    for (int i = 0; i < nkeys; i++) {
        int node_idx = get_node_for_key_consistent(sys, keys[i]);
        if (node_idx != -1) {
            load[node_idx]++;
        }
    }

    printf("## Load Distribution ##\n");
    for (int i = 0; i < sys->node_count; i++) {
        if (sys->nodes[i].active) {
            printf("  Node ID: %-10s | Key Count: %d\n", sys->nodes[i].id, load[i]);
        }
    }

    free(load);
}

void getKeysForNode(SystemState *sys, const char *nodeID, char **keys, int nkeys) {

    int target_node_idx = node_index_by_id(sys, nodeID);
    
    if (target_node_idx == -1) {
        printf("Node '%s' not found or is not active.\n", nodeID);
        return;
    }

    printf("## Keys for Node: %s ##\n", nodeID);
    int found_count = 0;

    // Iterate all keys and check if they map to the target node
    for (int i = 0; i < nkeys; i++) {
        int node_idx = get_node_for_key_consistent(sys, keys[i]);
        if (node_idx == target_node_idx) {
            printf("  %s\n", keys[i]);
            found_count++;
        }
    }

    if (found_count == 0) {
        printf("  (No keys mapped to this node)\n");
    }
}

void simulate(SystemState *sys, int nkeys) {
    if (nkeys <= 0) return;
    if (sys->plain_count < 2) {
        if (sys->ring_root) { free_bst(sys->ring_root); sys->ring_root = NULL; }
        for (int i = 0; i < sys->node_count; i++) {
            if (sys->nodes[i].vpos) { free(sys->nodes[i].vpos); sys->nodes[i].vpos = NULL; }
            sys->nodes[i].active = 0;
        }
        if (sys->plain_indices) { free(sys->plain_indices); sys->plain_indices = NULL; }
        sys->plain_count = 0;
        sys->node_count = 0;
        for (int i = 0; i < 5; i++) {
            char id[32];
            snprintf(id, sizeof(id), "Node-%d", i);
            add_node(sys, id, 100);
        }
    }

    // key generation

    char **keys = malloc(sizeof(char*) * nkeys);
    if (!keys) {
        fprintf(stderr, "Memory allocation failed for keys\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nkeys; i++) {
        keys[i] = malloc(MAX_KEY_LEN);
        if (!keys[i]) {
            fprintf(stderr, "Memory allocation failed for key strings\n");
            exit(EXIT_FAILURE);
        }
        snprintf(keys[i], MAX_KEY_LEN, "key-%d", i);
    }

    int *base_consistent = malloc(sizeof(int) * nkeys);
    int *base_plain = malloc(sizeof(int) * nkeys);
    if (!base_consistent || !base_plain) {
        fprintf(stderr, "Memory allocation failed in simulate()\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nkeys; i++) {
        base_consistent[i] = get_node_for_key_consistent(sys, keys[i]);
        base_plain[i] = get_node_for_key_plain(sys, keys[i]);
    }

    /////////////////////  extra problem statements

    // **** EXAMPLE: Call new functions here ****
    printf("\n--- Initial Load Distribution ---\n");
    getLoadDistribution(sys, keys, nkeys);
    printf("\n--- Keys for Node-0 ---\n");
    getKeysForNode(sys, "Node-0", keys, nkeys);
    printf("\n");

    int total_vnodes =0, vnode_counts=0;
    for (int i = 0; i <sys->node_count; i++)
        if (sys->nodes[i].active) { total_vnodes += sys->nodes[i].vnode_count; vnode_counts++; }

    int avg_vnodes = vnode_counts > 0 ? (total_vnodes / vnode_counts) : 100;
   
    add_node(sys, "Node-new", avg_vnodes);

    int *after_join_consistent = malloc(sizeof(int) * nkeys);
    int *after_join_plain = malloc(sizeof(int) * nkeys);
    if (!after_join_consistent || !after_join_plain) {
        fprintf(stderr, "Memory allocation failed after join\n");
        exit(EXIT_FAILURE);
    }

    for (int i =0;i <nkeys; i++) {
        after_join_consistent[i] = get_node_for_key_consistent(sys, keys[i]);
        after_join_plain[i] = get_node_for_key_plain(sys, keys[i]);
    }

    double remap_cons_join = compute_remap_fraction(base_consistent, after_join_consistent, nkeys);
    double remap_plain_join = compute_remap_fraction(base_plain, after_join_plain, nkeys);

    // node leave
    remove_node(sys,"Node-new");

    int *after_leave_consistent = malloc(sizeof(int) * nkeys);
    int *after_leave_plain = malloc(sizeof(int) * nkeys);
    if (!after_leave_consistent || !after_leave_plain) {
        fprintf(stderr, "Memory allocation failed after leave\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nkeys; i++) {
        after_leave_consistent[i] = get_node_for_key_consistent(sys, keys[i]);
        after_leave_plain[i] = get_node_for_key_plain(sys, keys[i]);
    }

    double remap_cons_leave = compute_remap_fraction(base_consistent, after_leave_consistent, nkeys);
    double remap_plain_leave = compute_remap_fraction(base_plain, after_leave_plain, nkeys);

    printf("{\n");
    printf("\"nNodes\": %d,\n", sys->plain_count);
    printf("\"nKeys\": %d,\n", nkeys);
    printf("\"join\": {\n");
    printf("\"remap_fraction\": { \"consistent\": %.6f, \"plain\": %.6f }\n", remap_cons_join, remap_plain_join);
    printf("},\n");
    printf("\"leave\": {\n");
    printf("\"remap_fraction\": { \"consistent\": %.6f, \"plain\": %.6f }\n", remap_cons_leave, remap_plain_leave);
    printf("}\n");
    printf("}\n");

    for (int i = 0; i < nkeys; i++) free(keys[i]);
    free(keys);
    free(base_consistent);
    free(base_plain);
    free(after_join_consistent);
    free(after_join_plain);
    free(after_leave_consistent);
    free(after_leave_plain);
}

