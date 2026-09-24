#include "tree_crdt/yata_engine.hpp"

namespace tree_crdt {

bool YataEngine::should_insert_before(const Item& candidate, const Item& current) {
    // YATA total-ordering rule:
    // If both items share the same origin_left, the one with lower ClientID (or higher client priority) wins.
    // This deterministically guarantees identical ordering on all distributed peers.
    if (candidate.origin_left == current.origin_left) {
        return candidate.id < current.id;
    }

    // If candidate's origin_left is current's origin_left, compare IDs
    return candidate.id.client < current.id.client;
}

TreeNode* YataEngine::get_by_id(const ClientID& id) const noexcept {
    auto it = item_map.find(id);
    if (it != item_map.end()) {
        return it->second;
    }
    return nullptr;
}

TreeNode* YataEngine::integrate(Item item) {
    // 1. Idempotency: Skip if already integrated
    if (item_map.contains(item.id)) {
        return item_map[item.id];
    }

    auto* new_node = new TreeNode(std::move(item));
    item_map[new_node->item.id] = new_node;

    // 2. Empty Tree Case
    if (!tree.root) {
        tree.root = new_node;
        return new_node;
    }

    TreeNode* left_origin_node = nullptr;
    if (!new_node->item.origin_left.is_null()) {
        left_origin_node = get_by_id(new_node->item.origin_left);
    }

    TreeNode* right_origin_node = nullptr;
    if (!new_node->item.origin_right.is_null()) {
        right_origin_node = get_by_id(new_node->item.origin_right);
    }

    // 3. Insert at Head (origin_left is null)
    if (!left_origin_node) {
        TreeNode* curr = tree.root;
        while (curr->left) curr = curr->left; // Find leftmost (first) node

        // Check if concurrent items already inserted at the head
        if (right_origin_node && curr == right_origin_node) {
            tree.insert_before(right_origin_node, new_node);
            return new_node;
        }

        // Compare against existing head item
        if (should_insert_before(new_node->item, curr->item)) {
            tree.insert_before(curr, new_node);
        } else {
            tree.insert_after(curr, new_node);
        }
        return new_node;
    }

    // 4. Standard Insertion after left origin
    // Check if concurrent items already exist between left_origin and right_origin
    TreeNode* curr = tree.get_successor(left_origin_node);
    while (curr && curr != right_origin_node) {
        if (should_insert_before(new_node->item, curr->item)) {
            tree.insert_before(curr, new_node);
            return new_node;
        }
        curr = tree.get_successor(curr);
    }

    if (right_origin_node) {
        tree.insert_before(right_origin_node, new_node);
    } else {
        tree.insert_after(left_origin_node, new_node);
    }
    return new_node;
}

void YataEngine::delete_in_node(TreeNode* node, size_t local_offset, size_t length) {
    if (!node || node->item.deleted || length == 0) return;

    TreeNode* target = node;

    // Step 1: If deletion starts after offset 0, split off the prefix
    if (local_offset > 0) {
        Item right_chunk = target->item.split(local_offset);
        auto* right_node = new TreeNode(std::move(right_chunk));
        item_map[right_node->item.id] = right_node;
        tree.insert_after(target, right_node);
        target = right_node;
    }

    // Step 2: If deletion does not cover the entire remaining target, split off the suffix
    if (length < target->item.length()) {
        Item suffix_chunk = target->item.split(length);
        auto* suffix_node = new TreeNode(std::move(suffix_chunk));
        item_map[suffix_node->item.id] = suffix_node;
        tree.insert_after(target, suffix_node);
    }

    // Step 3: Mark target as deleted tombstone
    target->item.mark_deleted();
    tree.rebalance_from(target);
}

} // namespace tree_crdt
