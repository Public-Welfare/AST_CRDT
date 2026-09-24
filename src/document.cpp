#include "tree_crdt/document.hpp"

namespace tree_crdt {

Document::Document(uint64_t client)
    : client_id(client) {
}

void Document::insert(size_t visible_index, std::string_view text) {
    if (text.empty()) return;

    const size_t doc_len = engine.tree.visible_length();

    // 1. Empty Document Case
    if (doc_len == 0) {
        ClientID item_id{client_id, current_clock++};
        Item item(item_id, ClientID::null_id(), ClientID::null_id(), text);
        engine.integrate(std::move(item));
        state_vector.update(client_id, item_id.clock + 1);
        return;
    }

    // 2. Prepend at Head
    if (visible_index == 0) {
        auto first_query = engine.tree.find_by_order(0);
        ClientID origin_right = first_query ? first_query->node->item.id : ClientID::null_id();

        ClientID item_id{client_id, current_clock++};
        Item item(item_id, ClientID::null_id(), origin_right, text);
        engine.integrate(std::move(item));
        state_vector.update(client_id, item_id.clock + 1);
        return;
    }

    // 3. Append at Tail
    if (visible_index >= doc_len) {
        auto last_query = engine.tree.find_by_order(doc_len - 1);
        ClientID origin_left = last_query ? last_query->node->item.id : ClientID::null_id();

        ClientID item_id{client_id, current_clock++};
        Item item(item_id, origin_left, ClientID::null_id(), text);
        engine.integrate(std::move(item));
        state_vector.update(client_id, item_id.clock + 1);
        return;
    }

    // 4. Insert in the Middle
    auto left_query = engine.tree.find_by_order(visible_index - 1);
    auto right_query = engine.tree.find_by_order(visible_index);

    if (left_query && right_query && left_query->node == right_query->node) {
        // Insertion falls inside a single node chunk -> Split the chunk
        TreeNode* target_node = left_query->node;
        const size_t split_offset = left_query->local_offset + 1;

        Item right_chunk = target_node->item.split(split_offset);
        auto* right_node = new TreeNode(std::move(right_chunk));
        engine.item_map[right_node->item.id] = right_node;
        engine.tree.insert_after(target_node, right_node);

        ClientID item_id{client_id, current_clock++};
        Item new_item(item_id, target_node->item.id, right_node->item.id, text);
        engine.integrate(std::move(new_item));
        state_vector.update(client_id, item_id.clock + 1);
    } else {
        ClientID origin_left = left_query ? left_query->node->item.id : ClientID::null_id();
        ClientID origin_right = right_query ? right_query->node->item.id : ClientID::null_id();

        ClientID item_id{client_id, current_clock++};
        Item new_item(item_id, origin_left, origin_right, text);
        engine.integrate(std::move(new_item));
        state_vector.update(client_id, item_id.clock + 1);
    }
}

void Document::delete_range(size_t visible_index, size_t length) {
    if (length == 0 || visible_index >= engine.tree.visible_length()) return;

    size_t remaining = length;

    while (remaining > 0 && visible_index < engine.tree.visible_length()) {
        auto query = engine.tree.find_by_order(visible_index);
        if (!query) break;

        const size_t avail_in_node = query->node->item.length() - query->local_offset;
        const size_t to_delete = std::min(remaining, avail_in_node);

        engine.delete_in_node(query->node, query->local_offset, to_delete);
        remaining -= to_delete;
    }
}

std::string Document::to_string() const {
    return engine.tree.to_string();
}

size_t Document::length() const noexcept {
    return engine.tree.visible_length();
}

size_t Document::line_count() const noexcept {
    return engine.tree.line_count();
}

int32_t Document::tree_height() const noexcept {
    return engine.tree.height();
}

EditorPosition Document::index_to_position(size_t visible_index) const noexcept {
    return engine.tree.index_to_position(visible_index);
}

size_t Document::position_to_index(uint32_t line, uint32_t column) const noexcept {
    return engine.tree.position_to_index(line, column);
}

StateVector Document::get_state_vector() const noexcept {
    return state_vector;
}

std::vector<uint8_t> Document::encode_update(const StateVector& remote_vector) const {
    return DeltaEncoder::encode_diff(engine, remote_vector);
}

size_t Document::apply_update(const std::vector<uint8_t>& update_data) {
    if (update_data.empty()) return 0;
    const size_t count = DeltaEncoder::apply_update(engine, update_data.data(), update_data.size());

    // Update state vector to reflect new items
    engine.tree.for_each_item([this](const Item& item) {
        state_vector.update(item.id.client, item.id.clock + 1);
    });

    return count;
}

} // namespace tree_crdt
