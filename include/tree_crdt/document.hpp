#pragma once

#include "types.hpp"
#include "yata_engine.hpp"
#include "state_vector.hpp"
#include "delta_encoder.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace tree_crdt {

/**
 * Document: The public interface for TreeCRDT.
 * Encapsulates the local client identity, the YATA consensus engine,
 * the augmented AVL tree, and synchronization protocols.
 */
class Document {
public:
    uint64_t client_id;
    uint32_t current_clock{0};
    YataEngine engine;
    StateVector state_vector;

    explicit Document(uint64_t client);

    // Text Editing API
    void insert(size_t visible_index, std::string_view text);
    void delete_range(size_t visible_index, size_t length);

    // Content & Metrics
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] size_t length() const noexcept;
    [[nodiscard]] size_t line_count() const noexcept;
    [[nodiscard]] int32_t tree_height() const noexcept;

    // Monaco 2D Coordinate Translation (O(log N))
    [[nodiscard]] EditorPosition index_to_position(size_t visible_index) const noexcept;
    [[nodiscard]] size_t position_to_index(uint32_t line, uint32_t column) const noexcept;

    // Differential Synchronization API
    [[nodiscard]] StateVector get_state_vector() const noexcept;
    [[nodiscard]] std::vector<uint8_t> encode_update(const StateVector& remote_vector) const;
    size_t apply_update(const std::vector<uint8_t>& update_data);
};

} // namespace tree_crdt
