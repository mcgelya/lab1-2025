#pragma once

#include <memory>

template <typename T>
class Sequence;

template <typename T>
using SequencePtr = std::shared_ptr<Sequence<T>>;

template <typename T>
class LazySequence;

template <typename T>
using LazySequencePtr = std::shared_ptr<LazySequence<T>>;

template <typename T>
class ReadOnlyStream;

template <typename T>
using ReadOnlyStreamPtr = std::shared_ptr<ReadOnlyStream<T>>;

template <typename T>
class WriteOnlyStream;

template <typename T>
using WriteOnlyStreamPtr = std::shared_ptr<WriteOnlyStream<T>>;
