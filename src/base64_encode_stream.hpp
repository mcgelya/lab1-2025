#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "fwd.hpp"
#include "stream.hpp"

class Base64EncodeStream : public ReadOnlyStream<char> {
public:
    explicit Base64EncodeStream(std::unique_ptr<ReadOnlyStream<uint8_t>> src, size_t bufferSizeBytes = 3)
        : src_(std::move(src)), bufferSize_(std::max<size_t>(1, bufferSizeBytes)) {
    }

    bool IsEndOfStream() const override {
        return inputDone_ && outPos_ >= out_.size();
    }

    char Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream ");
        }

        if (outPos_ >= out_.size()) {
            ProduceOutput();
        }

        if (out_.empty()) {
            inputDone_ = true;
            throw std::runtime_error("End of stream");
        }

        count_++;
        return out_[outPos_++];
    }

    size_t GetPosition() const override {
        return count_;
    }

    bool IsCanSeek() const override {
        return false;
    }

    size_t Seek(size_t index) override {
        throw std::logic_error("Cannot seek in base64 encode stream");
    }

    bool IsCanGoBack() const override {
        return false;
    }

private:
    static constexpr std::string_view kTable =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

private:
    std::unique_ptr<ReadOnlyStream<uint8_t>> src_;

    const size_t bufferSize_;

    std::vector<char> out_;
    size_t outPos_ = 0;

    std::array<uint8_t, 2> carry_{};
    size_t carryLen_ = 0;

    std::vector<uint8_t> in_;

    size_t count_ = 0;
    bool inputDone_ = false;

    void RefillInput() {
        in_.clear();
        in_.reserve(carryLen_ + bufferSize_);
        for (size_t i = 0; i < carryLen_; ++i) {
            in_.push_back(carry_[i]);
        }
        carryLen_ = 0;

        for (size_t i = 0; i < bufferSize_ && !src_->IsEndOfStream(); ++i) {
            in_.push_back(src_->Read());
        }
    }

    void EncodeTriplet(uint8_t b0, uint8_t b1, uint8_t b2) {
        const uint32_t triple =
            (static_cast<uint32_t>(b0) << 16) | (static_cast<uint32_t>(b1) << 8) | static_cast<uint32_t>(b2);
        out_.push_back(kTable[(triple >> 18) & 63]);
        out_.push_back(kTable[(triple >> 12) & 63]);
        out_.push_back(kTable[(triple >> 6) & 63]);
        out_.push_back(kTable[triple & 63]);
    }

    void EncodeFinal(uint8_t b0, uint8_t b1, size_t rem) {
        // rem is 1 or 2
        uint32_t triple = (static_cast<uint32_t>(b0) << 16);
        if (rem == 2) {
            triple |= (static_cast<uint32_t>(b1) << 8);
        }
        out_.push_back(kTable[(triple >> 18) & 63]);
        out_.push_back(kTable[(triple >> 12) & 63]);
        if (rem == 2) {
            out_.push_back(kTable[(triple >> 6) & 63]);
        } else {
            out_.push_back('=');
        }
        out_.push_back('=');
    }

    void ProduceOutput() {
        out_.clear();
        outPos_ = 0;

        while (out_.empty() && !inputDone_) {
            RefillInput();

            if (in_.empty()) {
                inputDone_ = true;
                return;
            }

            const bool srcEndedNow = src_->IsEndOfStream();
            const size_t n = in_.size();
            const size_t fullTriples = n / 3;
            const size_t rem = n % 3;

            out_.reserve(fullTriples * 4 + (rem ? 4 : 0));

            for (size_t i = 0; i < fullTriples; ++i) {
                const size_t off = i * 3;
                EncodeTriplet(in_[off], in_[off + 1], in_[off + 2]);
            }

            if (rem != 0) {
                const size_t off = fullTriples * 3;
                if (srcEndedNow) {
                    EncodeFinal(in_[off], (rem == 2 ? in_[off + 1] : 0), rem);
                    inputDone_ = true;
                } else {
                    carryLen_ = rem;
                    carry_[0] = in_[off];
                    if (rem == 2) {
                        carry_[1] = in_[off + 1];
                    }
                }
            } else if (srcEndedNow) {
                inputDone_ = true;
            }
        }
    }
};
