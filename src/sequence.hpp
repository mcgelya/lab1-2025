#pragma once

#include <iostream>

#include "fwd.hpp"
#include "ienum.hpp"

template <typename T>
class Sequence : public IConstEnumerable<T> {
public:
    virtual ~Sequence() = default;

    virtual const T& GetFirst() = 0;
    virtual const T& GetLast() = 0;
    virtual const T& Get(size_t index) = 0;

    virtual SequencePtr<T> GetSubsequence(size_t startIndex, size_t endIndex) const = 0;
    virtual SequencePtr<T> GetFirst(size_t count) const = 0;
    virtual SequencePtr<T> GetLast(size_t count) const = 0;

    virtual size_t GetLength() const = 0;

    virtual size_t GetCapacity() const {
        return GetLength();
    }

    virtual void Append(const T& item) = 0;
    virtual void Prepend(const T& item) = 0;
    virtual void InsertAt(const T& item, size_t index) = 0;

    virtual void Clear() = 0;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const Sequence<T>& v) {
    os << "{";
    for (auto it = v.GetConstEnumerator(); !it->IsEnd(); it->MoveNext()) {
        os << it->ConstDereference();
        if (it->Index() + 1 < v.GetLength()) {
            os << ", ";
        }
    }
    os << "}";
    return os;
}
