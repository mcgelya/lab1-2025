#pragma once

#include <cstddef>

template <typename T>
class ReadOnlyStream {
public:
    virtual ~ReadOnlyStream() = default;

    virtual bool IsEndOfStream() const = 0;

    virtual T Read() = 0;

    virtual size_t GetPosition() const = 0;

    virtual bool IsCanSeek() const = 0;

    virtual size_t Seek(size_t index) = 0;

    virtual bool IsCanGoBack() const = 0;

    virtual void Open() = 0;
    virtual void Close() = 0;
};

template <typename T>
class WriteOnlyStream {
public:
    virtual ~WriteOnlyStream() = default;

    virtual size_t GetPosition() const = 0;

    virtual size_t Write(const T& item) = 0;

    virtual void Open() = 0;
    virtual void Close() = 0;
};
