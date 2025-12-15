#pragma once

#include "array_sequence.hpp"
#include "fwd.hpp"
#include "stream.hpp"

class Base64EncodeStream : public ReadOnlyStream<char> {
public:
    explicit Base64EncodeStream(std::unique_ptr<ReadOnlyStream<uint8_t>> src)
        : src_(std::move(src)),
          in_(std::make_shared<ArraySequence<uint8_t>>()),
          out_(std::make_shared<ArraySequence<char>>()) {
        FillInputBuffer();
    }

    bool IsEndOfStream() const override {
        return finished_ && outPos_ >= out_->GetLength();
    }

    char Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream ");
        }

        if (outPos_ >= out_->GetLength()) {
            EncodeNextChunk();
            outPos_ = 0;
        }

        if (out_->GetLength() == 0) {
            finished_ = true;
            throw std::runtime_error("End of stream");
        }

        count_++;
        return out_->Get(outPos_++);
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

    SequencePtr<uint8_t> in_;
    SequencePtr<char> out_;
    size_t outPos_ = 0;

    size_t count_ = 0;
    bool finished_ = false;

    void FillInputBuffer() {
        in_->Clear();
        for (size_t i = 0; i < 3 && !src_->IsEndOfStream(); i++) {
            in_->Append(src_->Read());
        }

        if (in_->GetLength() == 0) {
            finished_ = true;
        }
    }

    void EncodeNextChunk() {
        out_->Clear();

        if (in_->GetLength() == 0) {
            FillInputBuffer();
            if (in_->GetLength() == 0) {
                return;
            }
        }

        const size_t inputLen = in_->GetLength();

        if (inputLen >= 1) {
            uint32_t triple = (static_cast<uint32_t>(in_->Get(0)) << 16);
            if (inputLen >= 2) {
                triple |= (static_cast<uint32_t>(in_->Get(1)) << 8);
            }
            if (inputLen >= 3) {
                triple |= static_cast<uint32_t>(in_->Get(2));
            }
            out_->Append(kTable[(triple >> 18) & 63]);
            out_->Append(kTable[(triple >> 12) & 63]);
            if (inputLen >= 2) {
                out_->Append(kTable[(triple >> 6) & 63]);
            } else {
                out_->Append('=');
            }
            if (inputLen >= 3) {
                out_->Append(kTable[triple & 63]);
            } else {
                out_->Append('=');
            }
        }
        in_->Clear();
        if (!src_->IsEndOfStream()) {
            FillInputBuffer();
        } else if (in_->GetLength() == 0) {
            finished_ = (outPos_ >= out_->GetLength());
        }
    }
};
