#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <compare>
#include <ostream>

namespace tree_crdt {

/**
 * ClientID uniquely identifies a participant and their monotonic clock.
 * Formally: { client_id: uint64_t, clock: uint32_t }
 */
struct ClientID {
    uint64_t client{0};
    uint32_t clock{0};

    constexpr auto operator<=>(const ClientID& other) const noexcept {
        if (auto cmp = client <=> other.client; cmp != 0) return cmp;
        return clock <=> other.clock;
    }

    constexpr bool operator==(const ClientID& other) const noexcept {
        return client == other.client && clock == other.clock;
    }

    constexpr bool is_null() const noexcept {
        return client == 0 && clock == 0;
    }

    static constexpr ClientID null_id() noexcept {
        return {0, 0};
    }
};

inline std::ostream& operator<<(std::ostream& os, const ClientID& id) {
    return os << "{" << id.client << ":" << id.clock << "}";
}

/**
 * 2D Monaco / VS Code editor coordinate representation.
 * 1-indexed for line and column to match standard IDE conventions.
 */
struct EditorPosition {
    uint32_t line{1};
    uint32_t column{1};

    constexpr bool operator==(const EditorPosition& other) const noexcept {
        return line == other.line && column == other.column;
    }
};

inline std::ostream& operator<<(std::ostream& os, const EditorPosition& pos) {
    return os << "Ln " << pos.line << ", Col " << pos.column;
}

} // namespace tree_crdt
