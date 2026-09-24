#pragma once

#include "yata_engine.hpp"
#include "state_vector.hpp"
#include <vector>

namespace tree_crdt {

/**
 * DeltaEncoder: Compact binary protocol for network synchronization.
 * Computes exact diffs based on StateVectors and applies updates idempotently.
 */
class DeltaEncoder {
public:
    /**
     * Serializes all operations present in the local engine that are unknown
     * to the remote peer according to remote_sv.
     */
    static std::vector<uint8_t> encode_diff(const YataEngine& engine, const StateVector& remote_sv);

    /**
     * Applies a binary delta update to the local YataEngine.
     * Returns the count of integrated items.
     */
    static size_t apply_update(YataEngine& engine, const uint8_t* data, size_t size);
};

} // namespace tree_crdt
