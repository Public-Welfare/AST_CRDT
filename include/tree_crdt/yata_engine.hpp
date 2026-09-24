#pragma once

#include "types.hpp"
#include "item.hpp"
#include "order_statistic_tree.hpp"
#include <unordered_map>
#include <vector>

namespace tree_crdt {

struct ClientIDHash {
    std::size_t operator()(const ClientID& id) const noexcept {
        const std::size_t h1 = std::hash<uint64_t>{}(id.client);
        const std::size_t h2 = std::hash<uint32_t>{}(id.clock);
        return h1 ^ (h2 << 1);
    }
};

/**
 * YataEngine: Implements the decentralized YATA consensus algorithm.
 * Guarantees mathematical convergence across any number of distributed peers
 * without a central coordinator.
 */
class YataEngine {
public:
    OrderStatisticTree tree;
    std::unordered_map<ClientID, TreeNode*, ClientIDHash> item_map;

    YataEngine() = default;

    /**
     * Integrates a new item into the document sequence using the deterministic YATA rules.
     * Returns a pointer to the newly inserted TreeNode.
     */
    TreeNode* integrate(Item item);

    /**
     * Finds a TreeNode by its ClientID in O(1) time.
     */
    [[nodiscard]] TreeNode* get_by_id(const ClientID& id) const noexcept;

    /**
     * Marks an offset range within a TreeNode as deleted (tombstone),
     * splitting prefix and suffix as needed.
     */
    void delete_in_node(TreeNode* node, size_t local_offset, size_t length);

private:
    /**
     * Deterministic comparison between two concurrent items with identical origin bounds.
     */
    [[nodiscard]] static bool should_insert_before(const Item& left_candidate, const Item& current);
};

} // namespace tree_crdt
