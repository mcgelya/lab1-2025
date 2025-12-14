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

    size_t Write(const T& item) {
        seq_->Append(item);
        return seq_->GetLength();
    }

    void Open() override {
    }

    void Close() override {
    }

private:
    SequencePtr<T> seq_;
};

template <typename T, typename Serialize>
class FileWriteStream : public WriteOnlyStream<T> {
public:
    FileWriteStream(std::string file, Serialize serialize)
        : of_(std::move(file)), index_(0), serialize_(std::move(serialize)) {
    }

    size_t GetPosition() const override {
        return index_;
    }

    size_t Write(const T& item) {
        of_ << serialize_(item);
        ++index_;
        return index_;
    }

    void Open() override {
    }

    void Close() override {
        of_.close();
    }

private:
    std::ofstream of_;
    size_t index_ = 0;
    Serialize serialize_;
};
