#include "planet/Quadtree.hpp"

void split_node(QuadNode* n) {
    if (!n || !n->is_leaf()) return;
    float hs = n->size * 0.5f;

    // 0=(0,0), 1=(1,0), 2=(0,1), 3=(1,1)
    n->child[0] = new QuadNode{n->level + 1, n->x,       n->y,       hs};
    n->child[1] = new QuadNode{n->level + 1, n->x + hs,  n->y,       hs};
    n->child[2] = new QuadNode{n->level + 1, n->x,       n->y + hs,  hs};
    n->child[3] = new QuadNode{n->level + 1, n->x + hs,  n->y + hs,  hs};
}

void delete_subtree(QuadNode* n) {
    if (!n) return;
    for (auto*& c : n->child) {
        delete_subtree(c);
        c = nullptr;
    }
    delete n;
}

void merge_node(QuadNode* n) {
    if (!n || n->is_leaf()) return;
    for (auto*& c : n->child) {
        delete_subtree(c);
        c = nullptr;
    }
}

void collect_leaves(QuadNode* n, std::vector<QuadNode*>& out) {
    if (!n) return;
    if (n->is_leaf()) {
        out.push_back(n);
        return;
    }
    for (auto* c : n->child) collect_leaves(c, out);
}