#pragma once

#include <fstream>

#include "sequence.hpp"
#include "stream.hpp"

template <typename T>
class SequenceWriteStream : public WriteOnlyStream<T> {
public:
    SequenceWriteStream(SequencePtr<T> seq) : seq_(std::move(seq)) {
    }

    size_t GetPosition() const override {
        return seq_->GetLength();
    }

    size_t Write(const T& item) override {
        seq_->Append(item);
        return seq_->GetLength();
    }

private:
    SequencePtr<T> seq_;
};

template <typename T, typename Serialize>
class FileWriteStream : public WriteOnlyStream<T> {
public:
    FileWriteStream(const std::string& file, Serialize serialize)
        : of_(file, std::ios::binary), index_(0), serialize_(std::move(serialize)) {
        if (!of_.is_open()) {
            throw std::runtime_error("Cannot open file");
        }
    }

    size_t GetPosition() const override {
        return index_;
    }

    size_t Write(const T& item) override {
        serialize_(of_, item);
        ++index_;
        return index_;
    }

private:
    std::ofstream of_;
    size_t index_ = 0;
    Serialize serialize_;
};
