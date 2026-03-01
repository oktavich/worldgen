#pragma once

#include <array>
#include <vector>

struct QuadNode {
    int level = 0;
    float x = 0.0f;
    float y = 0.0f;
    float size = 1.0f;
    std::array<QuadNode*, 4> child{nullptr, nullptr, nullptr, nullptr};

    bool is_leaf() const { return child[0] == nullptr; }
};

void split_node(QuadNode* n);
void merge_node(QuadNode* n);

void delete_subtree(QuadNode* n);

void collect_leaves(QuadNode* n, std::vector<QuadNode*>& out);