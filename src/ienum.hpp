#pragma once

#include <memory>

template <typename T>
class IEnumerator {
public:
    virtual bool IsEnd() const = 0;

    virtual void MoveNext() = 0;

    virtual T& Dereference() = 0;

    virtual size_t Index() const = 0;
};

template <typename T>
using IEnumeratorPtr = std::shared_ptr<IEnumerator<T>>;

template <typename T>
class IConstEnumerator {
public:
    virtual bool IsEnd() const = 0;

    virtual void MoveNext() = 0;

    virtual const T& ConstDereference() const = 0;

    virtual size_t Index() const = 0;
};

template <typename T>
using IConstEnumeratorPtr = std::shared_ptr<IConstEnumerator<T>>;

template <typename T>
class IEnumerable {
public:
    virtual IEnumeratorPtr<T> GetEnumerator() = 0;
};

template <typename T>
class IConstEnumerable {
public:
    virtual IConstEnumeratorPtr<T> GetConstEnumerator() const = 0;
};
