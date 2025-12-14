#pragma once

#include <cstddef>

enum class Cardinals {
    n,
    N0,
};

class Cardinal {
public:
    Cardinal(size_t n);
    Cardinal(Cardinals cardinal);

    bool IsFinite() const;

    bool IsN0() const;

    size_t GetFinite() const;

    bool operator==(const Cardinal& other) const;

    bool operator<(const Cardinal& other) const;

    Cardinal operator+(const Cardinal& m) const;
    Cardinal operator-(size_t m) const;

private:
    size_t n_;
    Cardinals cardinal_;
};
