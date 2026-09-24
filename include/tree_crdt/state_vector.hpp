#pragma once

#include "types.hpp"
#include <unordered_map>
#include <vector>
#include <cstring>

namespace tree_crdt {

/**
 * StateVector: Maps each client to their highest observed clock sequence.
 * Formally: { ClientID.client -> uint32_t max_clock }
 * 
 * Used for differential synchronization: Client A sends its StateVector to Client B,
 * and Client B generates a minimal update containing only operations that Client A has not seen.
 */
class StateVector {
public:
    std::unordered_map<uint64_t, uint32_t> clocks;

    StateVector() = default;

    void update(uint64_t client, uint32_t clock_count) noexcept {
        auto it = clocks.find(client);
        if (it == clocks.end()) {
            clocks[client] = clock_count;
        } else {
            it->second = std::max(it->second, clock_count);
        }
    }

    [[nodiscard]] uint32_t get_clock(uint64_t client) const noexcept {
        auto it = clocks.find(client);
        return it != clocks.end() ? it->second : 0;
    }

    [[nodiscard]] bool has(const ClientID& id) const noexcept {
        auto it = clocks.find(id.client);
        if (it == clocks.end()) return false;
        return id.clock < it->second;
    }

    // Binary serialization
    [[nodiscard]] std::vector<uint8_t> encode() const {
        std::vector<uint8_t> buffer;
        const uint32_t count = static_cast<uint32_t>(clocks.size());

        buffer.resize(sizeof(uint32_t) + count * (sizeof(uint64_t) + sizeof(uint32_t)));
        uint8_t* ptr = buffer.data();

        std::memcpy(ptr, &count, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        for (const auto& [client, clock] : clocks) {
            std::memcpy(ptr, &client, sizeof(uint64_t));
            ptr += sizeof(uint64_t);
            std::memcpy(ptr, &clock, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
        }

        return buffer;
    }

    static StateVector decode(const uint8_t* data, size_t size) {
        StateVector sv;
        if (size < sizeof(uint32_t)) return sv;

        uint32_t count = 0;
        std::memcpy(&count, data, sizeof(uint32_t));
        const uint8_t* ptr = data + sizeof(uint32_t);

        const size_t expected_size = sizeof(uint32_t) + count * (sizeof(uint64_t) + sizeof(uint32_t));
        if (size < expected_size) return sv;

        for (uint32_t i = 0; i < count; ++i) {
            uint64_t client = 0;
            uint32_t clock = 0;
            std::memcpy(&client, ptr, sizeof(uint64_t));
            ptr += sizeof(uint64_t);
            std::memcpy(&clock, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            sv.update(client, clock);
        }

        return sv;
    }
};

} // namespace tree_crdt
