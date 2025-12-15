#pragma once

#include <stdexcept>

#include "dynamic_array.hpp"
#include "ienum.hpp"
#include "sequence.hpp"

template <typename T>
class ArraySequenceIterator : public IEnumerator<T> {
public:
    ArraySequenceIterator(T* it, size_t size) : it_(it), size_(size) {
    }

    bool IsEnd() const override {
        return index_ == size_;
    }

    void MoveNext() override {
        ++it_;
        ++index_;
    }

    T& Dereference() override {
        return *it_;
    }

    size_t Index() const override {
        return index_;
    }

private:
    T* it_;
    const size_t size_;
    size_t index_ = 0;
};

template <typename T>
class ArraySequenceConstIterator : public IConstEnumerator<T> {
public:
    ArraySequenceConstIterator(const T* it, size_t size) : it_(it), size_(size) {
    }

    bool IsEnd() const override {
        return index_ == size_;
    }

    void MoveNext() override {
        ++it_;
        ++index_;
    }

    const T& ConstDereference() const override {
        return *it_;
    }

    size_t Index() const override {
        return index_;
    }

private:
    const T* it_;
    const size_t size_;
    size_t index_ = 0;
};

template <typename T>
class ArraySequence : public Sequence<T>, public IEnumerable<T> {
public:
    ArraySequence(const T* items, int count) {
        if (count == 0) {
            capacity_ = 1;
            size_ = 0;
            data_ = DynamicArray<T>(capacity_);
        } else {
            capacity_ = count;
            size_ = count;
            data_ = DynamicArray<T>(items, count);
        }
    }

    ArraySequence(DynamicArray<T> a) : capacity_(a.GetSize()), size_(a.GetSize()), data_(std::move(a)) {
    }

    ArraySequence(const Sequence<T>& a) : capacity_(a.GetCapacity()), size_(0), data_(capacity_) {
        for (IConstEnumeratorPtr<T> it = a.GetConstEnumerator(); !it->IsEnd(); it->MoveNext()) {
            Append(it->ConstDereference());
        }
    }

    ArraySequence(SequencePtr<T> a) : ArraySequence(*a) {
    }

    ArraySequence() : capacity_(1), size_(0), data_(capacity_) {
    }

    const T& GetFirst() override {
        if (size_ == 0) {
            throw std::out_of_range("Sequence is empty");
        }
        return data_.Get(0);
    }

    const T& GetLast() override {
        if (size_ == 0) {
            throw std::out_of_range("Sequence is empty");
        }
        return data_.Get(size_ - 1);
    }

    const T& Get(size_t index) override {
        if (index >= size_) {
            throw std::out_of_range("Index is out of range: " + std::to_string(index) + " " + std::to_string(size_));
        }
        return data_.Get(index);
    }

    SequencePtr<T> GetSubsequence(size_t startIndex, size_t endIndex) const override {
        if (startIndex >= size_ || endIndex >= size_) {
            throw std::out_of_range("Index is out of range: " + std::to_string(startIndex) + " " +
                                    std::to_string(endIndex) + " " + std::to_string(size_));
        }
        if (startIndex > endIndex) {
            throw std::out_of_range("startIndex is greater than endIndex");
        }
        const T* start = data_.GetConstBegin() + startIndex;
        size_t sz = endIndex - startIndex + 1;
        return std::make_shared<ArraySequence<T>>(start, sz);
    }

    SequencePtr<T> GetFirst(size_t count) const override {
        if (count == 0) {
            return std::make_shared<ArraySequence>();
        }
        if (count > size_) {
            throw std::out_of_range("Requested elements count is greater than size");
        }
        return GetSubsequence(0, count - 1);
    }

    SequencePtr<T> GetLast(size_t count) const override {
        if (count == 0) {
            return std::make_shared<ArraySequence>();
        }
        if (count > size_) {
            throw std::out_of_range("Requested elements count is greater than size");
        }
        return GetSubsequence(size_ - count, size_ - 1);
    }

    size_t GetLength() const override {
        return size_;
    }

    size_t GetCapacity() const override {
        return capacity_;
    }

    void Append(const T& item) override {
        PushBack(item);
    }

    void Prepend(const T& item) override {
        Insert(item, 0);
    }

    void InsertAt(const T& item, size_t index) override {
        Insert(item, index);
    }

    void Clear() override {
        size_ = 0;
    }

    IEnumeratorPtr<T> GetEnumerator() override {
        return std::make_shared<ArraySequenceIterator<T>>(data_.GetBegin(), size_);
    }

    IConstEnumeratorPtr<T> GetConstEnumerator() const override {
        return std::make_shared<ArraySequenceConstIterator<T>>(data_.GetConstBegin(), size_);
    }

private:
    size_t capacity_;
    size_t size_;
    DynamicArray<T> data_;

    void PushBack(const T& item) {
        if (size_ == capacity_) {
            capacity_ *= 2;
            data_.Resize(capacity_);
        }
        data_.Set(size_, item);
        ++size_;
    }

    void Insert(const T& item, int index) {
        if (size_ == capacity_) {
            data_.Resize(capacity_ * 2);
            capacity_ *= 2;
        }
        for (int i = size_ - 1; i >= index; --i) {
            T cur = data_.Get(i);
            data_.Set(i + 1, cur);
        }
        data_.Set(index, item);
        ++size_;
    }
};
