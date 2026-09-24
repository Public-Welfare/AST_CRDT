#pragma once

#include "types.hpp"
#include <string>
#include <string_view>
#include <algorithm>

namespace tree_crdt {

/**
 * Item: Fundamental atomic piece in a YATA-based CRDT sequence.
 * Represents a contiguous chunk of characters authored by a single client.
 * 
 * Invariants:
 *  - Immutable ID: { client, clock }
 *  - Immutable origins: origin_left and origin_right record the surrounding
 *    context at the time of creation, enabling decentralized deterministic insertion.
 *  - Deletions are soft-tombstones: deleted flag is set, visible length becomes 0.
 */
class Item {
public:
    ClientID id;
    ClientID origin_left;
    ClientID origin_right;
    std::string content;
    bool deleted{false};

    // Cached metric for newlines to accelerate 2D coordinate calculations
    size_t line_breaks_count{0};

    Item(ClientID item_id, 
         ClientID orig_left, 
         ClientID orig_right, 
         std::string_view text, 
         bool is_deleted = false)
        : id(item_id),
          origin_left(orig_left),
          origin_right(orig_right),
          content(text),
          deleted(is_deleted) {
        line_breaks_count = std::count(content.begin(), content.end(), '\n');
    }

    [[nodiscard]] size_t length() const noexcept {
        return content.size();
    }

    [[nodiscard]] size_t visible_length() const noexcept {
        return deleted ? 0 : content.size();
    }

    [[nodiscard]] size_t line_breaks() const noexcept {
        return line_breaks_count;
    }

    [[nodiscard]] size_t visible_line_breaks() const noexcept {
        return deleted ? 0 : line_breaks_count;
    }

    void mark_deleted() noexcept {
        deleted = true;
    }

    /**
     * Splits this item at a local offset (0 < offset < content.size()).
     * This item retains content[0..offset).
     * Returns a new Item containing content[offset..end) with clock adjusted.
     */
    Item split(size_t local_offset) {
        if (local_offset == 0 || local_offset >= content.size()) {
            throw std::out_of_range("Invalid split offset");
        }

        std::string right_content = content.substr(local_offset);
        content = content.substr(0, local_offset);
        line_breaks_count = std::count(content.begin(), content.end(), '\n');

        ClientID right_id = { id.client, static_cast<uint32_t>(id.clock + local_offset) };
        ClientID right_origin_left = { id.client, static_cast<uint32_t>(id.clock + local_offset - 1) };

        Item right_item(right_id, right_origin_left, origin_right, right_content, deleted);
        
        // This item's right origin conceptually becomes the start of the right item
        origin_right = right_id;

        return right_item;
    }
};

} // namespace tree_crdt
