#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>

#include "stream.hpp"

// A finite read-only stream producing pseudorandom bytes.
// Intended for large-scale testing without materializing data in memory.
class RandomByteStream final : public ReadOnlyStream<uint8_t> {
public:
    RandomByteStream(size_t totalBytes, uint32_t seed = 0)
        : total_(totalBytes), pos_(0), rng_(seed == 0 ? std::random_device{}() : seed), dist_(0, 255) {
    }

    bool IsEndOfStream() const override {
        return pos_ >= total_;
    }

    uint8_t Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream");
        }
        ++pos_;
        return static_cast<uint8_t>(dist_(rng_));
    }

    size_t GetPosition() const override {
        return pos_;
    }

    bool IsCanSeek() const override {
        return false;
    }

    size_t Seek(size_t) override {
        throw std::logic_error("Cannot seek in RandomByteStream");
    }

    bool IsCanGoBack() const override {
        return false;
    }

private:
    size_t total_;
    size_t pos_;
    std::mt19937 rng_;
    std::uniform_int_distribution<int> dist_;
};
