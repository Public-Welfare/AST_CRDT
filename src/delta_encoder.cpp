#include "tree_crdt/delta_encoder.hpp"
#include <cstring>

namespace tree_crdt {

std::vector<uint8_t> DeltaEncoder::encode_diff(const YataEngine& engine, const StateVector& remote_sv) {
    std::vector<Item> missing_items;

    engine.tree.for_each_item([&](const Item& item) {
        if (!remote_sv.has(item.id)) {
            missing_items.push_back(item);
        }
    });

    std::vector<uint8_t> buffer;
    const uint32_t count = static_cast<uint32_t>(missing_items.size());

    // Approximate sizing
    buffer.reserve(sizeof(uint32_t) + count * 64);

    // Write count
    const uint8_t* count_ptr = reinterpret_cast<const uint8_t*>(&count);
    buffer.insert(buffer.end(), count_ptr, count_ptr + sizeof(uint32_t));

    for (const auto& item : missing_items) {
        // ID
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&item.id.client);
        buffer.insert(buffer.end(), p, p + sizeof(uint64_t));
        p = reinterpret_cast<const uint8_t*>(&item.id.clock);
        buffer.insert(buffer.end(), p, p + sizeof(uint32_t));

        // Origin Left
        p = reinterpret_cast<const uint8_t*>(&item.origin_left.client);
        buffer.insert(buffer.end(), p, p + sizeof(uint64_t));
        p = reinterpret_cast<const uint8_t*>(&item.origin_left.clock);
        buffer.insert(buffer.end(), p, p + sizeof(uint32_t));

        // Origin Right
        p = reinterpret_cast<const uint8_t*>(&item.origin_right.client);
        buffer.insert(buffer.end(), p, p + sizeof(uint64_t));
        p = reinterpret_cast<const uint8_t*>(&item.origin_right.clock);
        buffer.insert(buffer.end(), p, p + sizeof(uint32_t));

        // Deleted flag
        const uint8_t del = item.deleted ? 1 : 0;
        buffer.push_back(del);

        // Content
        const uint32_t len = static_cast<uint32_t>(item.content.size());
        p = reinterpret_cast<const uint8_t*>(&len);
        buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
        buffer.insert(buffer.end(), item.content.begin(), item.content.end());
    }

    return buffer;
}

size_t DeltaEncoder::apply_update(YataEngine& engine, const uint8_t* data, size_t size) {
    if (!data || size < sizeof(uint32_t)) return 0;

    uint32_t count = 0;
    std::memcpy(&count, data, sizeof(uint32_t));
    const uint8_t* ptr = data + sizeof(uint32_t);
    const uint8_t* end = data + size;

    size_t applied_count = 0;

    for (uint32_t i = 0; i < count; ++i) {
        if (ptr + sizeof(uint64_t) * 3 + sizeof(uint32_t) * 3 + 1 > end) break;

        ClientID id;
        std::memcpy(&id.client, ptr, sizeof(uint64_t));
        ptr += sizeof(uint64_t);
        std::memcpy(&id.clock, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        ClientID origin_left;
        std::memcpy(&origin_left.client, ptr, sizeof(uint64_t));
        ptr += sizeof(uint64_t);
        std::memcpy(&origin_left.clock, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        ClientID origin_right;
        std::memcpy(&origin_right.client, ptr, sizeof(uint64_t));
        ptr += sizeof(uint64_t);
        std::memcpy(&origin_right.clock, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        const bool deleted = (*ptr != 0);
        ptr += 1;

        if (ptr + sizeof(uint32_t) > end) break;
        uint32_t content_len = 0;
        std::memcpy(&content_len, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        if (ptr + content_len > end) break;
        std::string content(reinterpret_cast<const char*>(ptr), content_len);
        ptr += content_len;

        Item item(id, origin_left, origin_right, content, deleted);
        engine.integrate(std::move(item));
        applied_count++;
    }

    return applied_count;
}

} // namespace tree_crdt
