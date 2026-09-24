#include "tree_crdt/order_statistic_tree.hpp"
#include <stack>
#include <sstream>

namespace tree_crdt {

OrderStatisticTree::~OrderStatisticTree() {
    _free_subtree(root);
    root = nullptr;
}

void OrderStatisticTree::_free_subtree(TreeNode* node) noexcept {
    if (!node) return;
    _free_subtree(node->left);
    _free_subtree(node->right);
    delete node;
}

OrderStatisticTree::OrderStatisticTree(OrderStatisticTree&& other) noexcept
    : root(other.root) {
    other.root = nullptr;
}

OrderStatisticTree& OrderStatisticTree::operator=(OrderStatisticTree&& other) noexcept {
    if (this != &other) {
        _free_subtree(root);
        root = other.root;
        other.root = nullptr;
    }
    return *this;
}

size_t OrderStatisticTree::visible_length() const noexcept {
    return root ? root->subtree_visible_length : 0;
}

size_t OrderStatisticTree::total_length() const noexcept {
    return root ? root->subtree_total_length : 0;
}

size_t OrderStatisticTree::line_count() const noexcept {
    return root ? root->subtree_visible_line_breaks + 1 : 1;
}

int32_t OrderStatisticTree::height() const noexcept {
    return root ? root->height : 0;
}

// ---------------- ROTATIONS & BALANCING ----------------

TreeNode* OrderStatisticTree::_rotate_right(TreeNode* y) noexcept {
    TreeNode* x = y->left;
    TreeNode* B = x->right;

    x->right = y;
    x->parent = y->parent;
    y->parent = x;

    y->left = B;
    if (B) B->parent = y;

    if (x->parent) {
        if (x->parent->left == y) {
            x->parent->left = x;
        } else {
            x->parent->right = x;
        }
    } else {
        root = x;
    }

    y->update();
    x->update();
    return x;
}

TreeNode* OrderStatisticTree::_rotate_left(TreeNode* x) noexcept {
    TreeNode* y = x->right;
    TreeNode* B = y->left;

    y->left = x;
    y->parent = x->parent;
    x->parent = y;

    x->right = B;
    if (B) B->parent = x;

    if (y->parent) {
        if (y->parent->left == x) {
            y->parent->left = y;
        } else {
            y->parent->right = y;
        }
    } else {
        root = y;
    }

    x->update();
    y->update();
    return y;
}

void OrderStatisticTree::rebalance_from(TreeNode* start_node) {
    TreeNode* curr = start_node;

    while (curr) {
        curr->update();
        const int32_t bf = curr->balance_factor();

        if (bf > 1) {
            // Left heavy
            if (curr->left && curr->left->balance_factor() < 0) {
                // Left-Right case
                _rotate_left(curr->left);
            }
            curr = _rotate_right(curr);
        } else if (bf < -1) {
            // Right heavy
            if (curr->right && curr->right->balance_factor() > 0) {
                // Right-Left case
                _rotate_right(curr->right);
            }
            curr = _rotate_left(curr);
        }

        curr = curr->parent;
    }
}

// ---------------- ORDER STATISTIC SEARCH ----------------

std::optional<OrderQueryResult> OrderStatisticTree::find_by_order(size_t visible_k) const noexcept {
    if (!root || visible_k >= visible_length()) {
        return std::nullopt;
    }

    TreeNode* curr = root;
    size_t accumulated = 0;

    while (curr) {
        const size_t left_v_len = curr->left ? curr->left->subtree_visible_length : 0;
        const size_t curr_v_len = curr->item.visible_length();

        if (visible_k < left_v_len) {
            curr = curr->left;
        } else if (curr_v_len > 0 && visible_k < left_v_len + curr_v_len) {
            const size_t local_offset = visible_k - left_v_len;
            return OrderQueryResult{
                curr,
                local_offset,
                accumulated + left_v_len
            };
        } else {
            visible_k -= (left_v_len + curr_v_len);
            accumulated += (left_v_len + curr_v_len);
            curr = curr->right;
        }
    }

    return std::nullopt;
}

size_t OrderStatisticTree::order_of_key(const TreeNode* node) const noexcept {
    if (!node) return 0;

    size_t index = node->left ? node->left->subtree_visible_length : 0;
    const TreeNode* curr = node;

    while (curr->parent) {
        if (curr->parent->right == curr) {
            const size_t parent_left_v = curr->parent->left ? curr->parent->left->subtree_visible_length : 0;
            index += parent_left_v + curr->parent->item.visible_length();
        }
        curr = curr->parent;
    }

    return index;
}

// ---------------- MONACO 2D COORDINATES ----------------

EditorPosition OrderStatisticTree::index_to_position(size_t visible_k) const noexcept {
    if (!root || visible_k == 0) {
        return EditorPosition{1, 1};
    }

    if (visible_k >= visible_length()) {
        visible_k = visible_length();
    }

    uint32_t current_line = 1;
    size_t last_newline_pos = 0;
    size_t current_pos = 0;

    TreeNode* curr = root;

    while (curr) {
        const size_t left_v_len = curr->left ? curr->left->subtree_visible_length : 0;
        const size_t left_v_lb = curr->left ? curr->left->subtree_visible_line_breaks : 0;

        if (visible_k < left_v_len) {
            curr = curr->left;
        } else {
            // Count newlines from left subtree
            current_line += static_cast<uint32_t>(left_v_lb);
            current_pos += left_v_len;
            visible_k -= left_v_len;

            const size_t curr_v_len = curr->item.visible_length();
            if (visible_k < curr_v_len || (curr_v_len > 0 && visible_k == curr_v_len && !curr->right)) {
                // Character falls inside this node
                const std::string& text = curr->item.content;
                for (size_t i = 0; i < visible_k; ++i) {
                    if (text[i] == '\n') {
                        current_line++;
                        last_newline_pos = current_pos + i + 1;
                    }
                }
                current_pos += visible_k;
                break;
            } else {
                // Traverse right
                if (!curr->item.deleted) {
                    current_line += static_cast<uint32_t>(curr->item.line_breaks());
                }
                current_pos += curr_v_len;
                visible_k -= curr_v_len;
                curr = curr->right;
            }
        }
    }

    const uint32_t column = static_cast<uint32_t>(current_pos - last_newline_pos + 1);
    return EditorPosition{current_line, column};
}

size_t OrderStatisticTree::position_to_index(uint32_t line, uint32_t column) const noexcept {
    if (!root || (line <= 1 && column <= 1)) {
        return 0;
    }

    size_t target_index = 0;
    uint32_t current_line = 1;
    uint32_t current_col = 1;

    TreeNode* curr = root;
    std::stack<TreeNode*> traversal_stack;

    // In-order iteration
    while (curr || !traversal_stack.empty()) {
        while (curr) {
            traversal_stack.push(curr);
            curr = curr->left;
        }

        curr = traversal_stack.top();
        traversal_stack.pop();

        if (!curr->item.deleted) {
            const std::string& text = curr->item.content;
            for (char ch : text) {
                if (current_line == line && current_col == column) {
                    return target_index;
                }

                target_index++;
                if (ch == '\n') {
                    current_line++;
                    current_col = 1;
                } else {
                    current_col++;
                }
            }
        }

        curr = curr->right;
    }

    return target_index;
}

// ---------------- TOPOLOGY MODIFICATIONS ----------------

void OrderStatisticTree::insert_at_head(TreeNode* new_node) {
    if (!new_node) return;

    if (!root) {
        root = new_node;
        return;
    }

    TreeNode* curr = root;
    while (curr->left) curr = curr->left;

    curr->left = new_node;
    new_node->parent = curr;
    rebalance_from(new_node);
}

void OrderStatisticTree::insert_at_tail(TreeNode* new_node) {
    if (!new_node) return;

    if (!root) {
        root = new_node;
        return;
    }

    TreeNode* curr = root;
    while (curr->right) curr = curr->right;

    curr->right = new_node;
    new_node->parent = curr;
    rebalance_from(new_node);
}

void OrderStatisticTree::insert_after(TreeNode* target, TreeNode* new_node) {
    if (!target || !new_node) return;

    if (!target->right) {
        target->right = new_node;
        new_node->parent = target;
    } else {
        // Place at leftmost leaf of right subtree (in-order successor position)
        TreeNode* succ = target->right;
        while (succ->left) succ = succ->left;
        succ->left = new_node;
        new_node->parent = succ;
    }

    rebalance_from(new_node);
}

void OrderStatisticTree::insert_before(TreeNode* target, TreeNode* new_node) {
    if (!target || !new_node) return;

    if (!target->left) {
        target->left = new_node;
        new_node->parent = target;
    } else {
        // Place at rightmost leaf of left subtree (in-order predecessor position)
        TreeNode* pred = target->left;
        while (pred->right) pred = pred->right;
        pred->right = new_node;
        new_node->parent = pred;
    }

    rebalance_from(new_node);
}

TreeNode* OrderStatisticTree::get_successor(TreeNode* node) const noexcept {
    if (!node) return nullptr;
    if (node->right) {
        TreeNode* curr = node->right;
        while (curr->left) curr = curr->left;
        return curr;
    }
    TreeNode* curr = node;
    while (curr->parent && curr->parent->right == curr) {
        curr = curr->parent;
    }
    return curr->parent;
}

TreeNode* OrderStatisticTree::get_predecessor(TreeNode* node) const noexcept {
    if (!node) return nullptr;
    if (node->left) {
        TreeNode* curr = node->left;
        while (curr->right) curr = curr->right;
        return curr;
    }
    TreeNode* curr = node;
    while (curr->parent && curr->parent->left == curr) {
        curr = curr->parent;
    }
    return curr->parent;
}

// ---------------- SERIALIZATION ----------------

std::string OrderStatisticTree::to_string() const {
    if (!root) return "";

    std::string result;
    result.reserve(visible_length());

    std::stack<TreeNode*> s;
    TreeNode* curr = root;

    while (curr || !s.empty()) {
        while (curr) {
            s.push(curr);
            curr = curr->left;
        }

        curr = s.top();
        s.pop();

        if (!curr->item.deleted) {
            result.append(curr->item.content);
        }

        curr = curr->right;
    }

    return result;
}

void OrderStatisticTree::for_each_item(const std::function<void(const Item&)>& callback) const {
    std::stack<TreeNode*> s;
    TreeNode* curr = root;

    while (curr || !s.empty()) {
        while (curr) {
            s.push(curr);
            curr = curr->left;
        }

        curr = s.top();
        s.pop();

        callback(curr->item);

        curr = curr->right;
    }
}

} // namespace tree_crdt
