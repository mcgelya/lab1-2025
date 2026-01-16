#pragma once

#include <fstream>
#include <stdexcept>

#include "lazy_sequence.hpp"
#include "sequence.hpp"
#include "stream.hpp"

template <typename T>
class SequenceReadStream : public ReadOnlyStream<T> {
public:
    SequenceReadStream(SequencePtr<T> seq) : seq_(std::move(seq)), index_(0) {
    }

    bool IsEndOfStream() const override {
        return index_ == seq_->GetLength();
    }

    T Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream");
        }
        ++index_;
        return seq_->Get(index_ - 1);
    }

    size_t GetPosition() const override {
        return index_;
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        if (seq_->GetLength() < index) {
            throw std::out_of_range("index is greater than length");
        }
        index_ = index;
        return index_;
    }

    bool IsCanGoBack() const override {
        return true;
    }

private:
    SequencePtr<T> seq_;
    size_t index_ = 0;
};

template <typename T>
class LazySequenceReadStream : public ReadOnlyStream<T> {
public:
    LazySequenceReadStream(LazySequencePtr<T> seq) : seq_(std::move(seq)), index_(0) {
    }

    bool IsEndOfStream() const override {
        return index_ == seq_->GetLength();
    }

    T Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream");
        }
        ++index_;
        return seq_->GetIndex(index_ - 1);
    }

    size_t GetPosition() const override {
        return index_;
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        if (seq_->GetLength() < index) {
            throw std::out_of_range("index is greater than length");
        }
        index_ = index;
        return index_;
    }

    bool IsCanGoBack() const override {
        return true;
    }

private:
    LazySequencePtr<T> seq_;
    size_t index_ = 0;
};

template <typename T, typename Parse>
class StringReadStream : public ReadOnlyStream<T> {
public:
    StringReadStream(std::string in, Parse parse) : in_(std::move(in)), index_(0), count_(0), parse_(std::move(parse)) {
    }

    bool IsEndOfStream() const override {
        return index_ == in_.size();
    }

    T Read() override {
        while (index_ < in_.size() && std::isspace(in_[index_])) {
            ++index_;
        }
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream");
        }
        size_t next = index_;
        while (next < in_.size() && !std::isspace(in_[next])) {
            ++next;
        }
        index_ = next;
        ++count_;
        return parse_(in_.substr(index_, next - index_));
    }

    size_t GetPosition() const override {
        return count_;
    }

    bool IsCanSeek() const override {
        return false;
    }

    size_t Seek(size_t index) override {
        throw std::logic_error("Cannot seek in string read stream");
    }

    bool IsCanGoBack() const override {
        return false;
    }

private:
    std::string in_;
    size_t index_ = 0;
    size_t count_ = 0;
    Parse parse_;
};

template <typename T, typename Parse>
class FileReadStream : public ReadOnlyStream<T> {
public:
    FileReadStream(const std::string& file, Parse parse)
        : if_(file, std::ios::binary), count_(0), parse_(std::move(parse)) {
        if (!if_.is_open()) {
            throw std::runtime_error("Cannot open file");
        }
    }

    bool IsEndOfStream() const override {
        return if_.eof();
    }

    T Read() override {
        if (IsEndOfStream()) {
            throw std::runtime_error("End of stream");
        }
        ++count_;
        return parse_(if_);
    }

    size_t GetPosition() const override {
        return count_;
    }

    bool IsCanSeek() const override {
        return false;
    }

    size_t Seek(size_t index) override {
        throw std::logic_error("Cannot seek in file read stream");
    }

    bool IsCanGoBack() const override {
        return false;
    }

private:
    std::ifstream if_;
    size_t count_ = 0;
    Parse parse_;
};
