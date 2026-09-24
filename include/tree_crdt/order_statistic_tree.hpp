#pragma once

#include "types.hpp"
#include "item.hpp"
#include <memory>
#include <string>
#include <optional>
#include <functional>

namespace tree_crdt {

/**
 * TreeNode: A node in the augmented self-balancing AVL Order-Statistic Tree.
 * Each node wraps an atomic CRDT Item and maintains augmented subtree invariants.
 */
class TreeNode {
public:
    Item item;

    // Augmented Subtree Fields (O(1) update, O(log N) global query)
    size_t subtree_visible_length{0};
    size_t subtree_total_length{0};
    size_t subtree_visible_line_breaks{0};
    int32_t height{1};

    // Tree Pointers
    TreeNode* left{nullptr};
    TreeNode* right{nullptr};
    TreeNode* parent{nullptr};

    explicit TreeNode(Item crdt_item)
        : item(std::move(crdt_item)),
          subtree_visible_length(item.visible_length()),
          subtree_total_length(item.length()),
          subtree_visible_line_breaks(item.visible_line_breaks()),
          height(1) {}

    // Recalculates augmented values from immediate left and right children
    inline void update() noexcept {
        const size_t left_v_len = left ? left->subtree_visible_length : 0;
        const size_t right_v_len = right ? right->subtree_visible_length : 0;
        subtree_visible_length = item.visible_length() + left_v_len + right_v_len;

        const size_t left_t_len = left ? left->subtree_total_length : 0;
        const size_t right_t_len = right ? right->subtree_total_length : 0;
        subtree_total_length = item.length() + left_t_len + right_t_len;

        const size_t left_v_lb = left ? left->subtree_visible_line_breaks : 0;
        const size_t right_v_lb = right ? right->subtree_visible_line_breaks : 0;
        subtree_visible_line_breaks = item.visible_line_breaks() + left_v_lb + right_v_lb;

        const int32_t left_h = left ? left->height : 0;
        const int32_t right_h = right ? right->height : 0;
        height = 1 + std::max(left_h, right_h);
    }

    [[nodiscard]] inline int32_t balance_factor() const noexcept {
        const int32_t left_h = left ? left->height : 0;
        const int32_t right_h = right ? right->height : 0;
        return left_h - right_h;
    }
};

/**
 * Result structure returned by find_by_order.
 */
struct OrderQueryResult {
    TreeNode* node{nullptr};
    size_t local_offset{0};
    size_t absolute_visible_start{0};
};

/**
 * OrderStatisticTree: Production-grade Augmented AVL Binary Search Tree.
 * Enables O(log N) positional queries, Monaco 2D coordinate translation,
 * and seamless CRDT sequence indexing.
 */
class OrderStatisticTree {
public:
    TreeNode* root{nullptr};

    OrderStatisticTree() = default;
    ~OrderStatisticTree();

    // Prevent direct copying for clear RAII ownership
    OrderStatisticTree(const OrderStatisticTree&) = delete;
    OrderStatisticTree& operator=(const OrderStatisticTree&) = delete;

    // Allow move semantics
    OrderStatisticTree(OrderStatisticTree&& other) noexcept;
    OrderStatisticTree& operator=(OrderStatisticTree&& other) noexcept;

    // Metrics
    [[nodiscard]] size_t visible_length() const noexcept;
    [[nodiscard]] size_t total_length() const noexcept;
    [[nodiscard]] size_t line_count() const noexcept;
    [[nodiscard]] int32_t height() const noexcept;

    // Core O(log N) Order-Statistic Operations
    [[nodiscard]] std::optional<OrderQueryResult> find_by_order(size_t visible_k) const noexcept;
    [[nodiscard]] size_t order_of_key(const TreeNode* node) const noexcept;

    // Monaco / VS Code O(log N) 2D Coordinate Conversions
    [[nodiscard]] EditorPosition index_to_position(size_t visible_k) const noexcept;
    [[nodiscard]] size_t position_to_index(uint32_t line, uint32_t column) const noexcept;

    // Tree Topology Manipulation (preserves In-Order Sequence)
    void insert_at_head(TreeNode* new_node);
    void insert_at_tail(TreeNode* new_node);
    void insert_after(TreeNode* target, TreeNode* new_node);
    void insert_before(TreeNode* target, TreeNode* new_node);

    // Rebalancing & Updates
    void rebalance_from(TreeNode* start_node);

    // Traversal Helpers
    [[nodiscard]] TreeNode* get_successor(TreeNode* node) const noexcept;
    [[nodiscard]] TreeNode* get_predecessor(TreeNode* node) const noexcept;

    // Serialization & Traversal
    [[nodiscard]] std::string to_string() const;
    void for_each_item(const std::function<void(const Item&)>& callback) const;

private:
    void _free_subtree(TreeNode* node) noexcept;
    TreeNode* _rotate_right(TreeNode* y) noexcept;
    TreeNode* _rotate_left(TreeNode* x) noexcept;
};

} // namespace tree_crdt
