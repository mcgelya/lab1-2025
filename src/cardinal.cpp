#include "cardinal.hpp"

Cardinal::Cardinal(size_t n) : n_(n), cardinal_(Cardinals::n) {
}

Cardinal::Cardinal(Cardinals cardinal) : n_(0), cardinal_(cardinal) {
}

bool Cardinal::IsFinite() const {
    return cardinal_ == Cardinals::n;
}

bool Cardinal::IsN0() const {
    return cardinal_ == Cardinals::N0;
}

size_t Cardinal::GetFinite() const {
    return n_;
}

bool Cardinal::operator==(const Cardinal& other) const {
    if (cardinal_ != other.cardinal_) {
        return false;
    }
    return cardinal_ == Cardinals::N0 || n_ == other.n_;
}

bool Cardinal::operator<(const Cardinal& other) const {
    if (cardinal_ == other.cardinal_) {
        return n_ < other.n_;
    }
    return cardinal_ < other.cardinal_;
}

Cardinal Cardinal::operator+(const Cardinal& m) const {
    return cardinal_ == Cardinals::N0 ? Cardinal(cardinal_) : Cardinal(n_ + m.n_);
}

Cardinal Cardinal::operator-(size_t m) const {
    return cardinal_ == Cardinals::N0 ? Cardinal(cardinal_) : Cardinal(n_ - m);
}
