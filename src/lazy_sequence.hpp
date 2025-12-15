#pragma once

#include <optional>

#include "array_sequence.hpp"
#include "cardinal.hpp"

template <typename T>
class LazySequenceIterator : public IConstEnumerator<T> {
public:
    explicit LazySequenceIterator(LazySequencePtr<T> owner) : owner_(std::move(owner)), index_(0) {
    }

    bool IsEnd() const override {
        return Cardinal(index_) == owner_->GetLength() ||
               (index_ == owner_->GetMaterializedCount() && !owner_->HasNext());
    }

    void MoveNext() override {
        ++index_;
    }

    const T& ConstDereference() const override {
        return owner_->GetIndex(index_);
    }

    size_t Index() const override {
        return index_;
    }

private:
    const LazySequencePtr<T> owner_;
    size_t index_ = 0;
};

template <typename T>
class LazySequence : public std::enable_shared_from_this<LazySequence<T>> {
private:
    class IGenerator;

    class DefaultGenerator;

    class SequenceGenerator;

    template <typename Func>
    class FunctionGenerator;

    struct SubSequenceTag {};
    class SubsequenceGenerator;

    struct SkipTag {};
    class SkipGenerator;

    struct AppendTag {};
    class AppendGenerator;

    struct InsertTag {};
    class InsertGenerator;

    struct ConcatTag {};
    class ConcatGenerator;

    struct MapTag {};
    template <typename T2, typename Func>
    class MapGenerator;

    struct WhereTag {};
    template <typename Func>
    class WhereGenerator;

    struct ZipTag {};
    template <typename T1, typename T2>
    class ZipGenerator;

private:
    class IGenerator {
    public:
        virtual ~IGenerator() = default;

        virtual T GetNext() = 0;

        virtual std::optional<T> TryGetNext() = 0;

        virtual bool HasNext() const = 0;
    };

    class DefaultGenerator : public IGenerator {
    public:
        DefaultGenerator(LazySequencePtr<T> seq) : seq_(std::move(seq), it_(seq_->GetConstEnumerator())) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            T res = it_->ConstDereference();
            it_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return !it_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
    };

    // Noop, sequence is already in the owner
    class SequenceGenerator : public IGenerator {
    public:
        SequenceGenerator() = default;

        T GetNext() override {
            throw std::out_of_range("GetNext: no next element");
        }

        bool HasNext() const override {
            return false;
        }

        std::optional<T> TryGetNext() override {
            return std::nullopt;
        }
    };

    template <typename Func>
    class FunctionGenerator : public IGenerator {
    public:
        FunctionGenerator(LazySequence<T>* owner, Func func, size_t arity)
            : owner_(owner), func_(std::move(func)), arity_(arity) {
        }

        T GetNext() override {
            SequencePtr<T> suf = owner_->items_->GetLast(arity_);
            return func_(suf);
        }

        bool HasNext() const override {
            return true;
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequence<T>* owner_;
        Func func_;
        size_t arity_;
    };

    class SubsequenceGenerator : public IGenerator {
    public:
        SubsequenceGenerator(LazySequencePtr<T> seq, size_t startIndex, size_t endIndex)
            : seq_(std::move(seq)), it_(seq_->GetConstEnumerator()), startIndex_(startIndex), endIndex_(endIndex) {
            for (size_t i = 0; i < startIndex; ++i) {
                it_->MoveNext();
            }
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            T res = it_->ConstDereference();
            it_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return it_->Index() != endIndex_ + 1;
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
        size_t startIndex_;
        size_t endIndex_;
    };

    class SkipGenerator : public IGenerator {
    public:
        SkipGenerator(LazySequencePtr<T> seq, size_t startIndex, size_t endIndex)
            : seq_(std::move(seq)), it_(seq_->GetConstEnumerator()), startIndex_(startIndex), endIndex_(endIndex) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            if (it_->Index() >= startIndex_) {
                while (it_->Index() <= endIndex_) {
                    it_->MoveNext();
                }
            }
            T res = it_->ConstDereference();
            it_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return !it_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
        size_t startIndex_;
        size_t endIndex_;
    };

    class AppendGenerator : public IGenerator {
    public:
        AppendGenerator(LazySequencePtr<T> seq, const T& item) : seq_(std::move(seq)), item_(item), added_(false) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            if (!it_->IsEnd()) {
                T res = it_->ConstDereference();
                it_->MoveNext();
                return res;
            }
            added_ = true;
            return std::move(item_);
        }

        bool HasNext() const override {
            return !it_->IsEnd() || !added_;
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
        T item_;
        bool added_;
    };

    class InsertGenerator : public IGenerator {
    public:
        InsertGenerator(LazySequencePtr<T> seq, const T& item, size_t index)
            : seq_(std::move(seq)),
              it_(seq_->GetConstEnumerator()),
              item_(item),
              index_(index),
              cur_(0),
              added_(false) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            if (cur_ == index_) {
                added_ = true;
                ++cur_;
                return std::move(item_);
            }
            ++cur_;
            T res = it_->ConstDereference();
            it_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return !it_->IsEnd() || !added_;
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
        T item_;
        size_t index_;
        size_t cur_;
        bool added_;
    };

    class ConcatGenerator : public IGenerator {
    public:
        ConcatGenerator(LazySequencePtr<T> seq1, LazySequencePtr<T> seq2)
            : seq1_(std::move(seq1)),
              it1_(seq1_->GetConstEnumerator()),
              seq2_(std::move(seq2)),
              it2_(seq2_->GetConstEnumerator()) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            if (it1_->IsEnd()) {
                T res = it2_->ConstDereference();
                it2_->MoveNext();
                return res;
            }
            T res = it1_->ConstDereference();
            it1_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return !it1_->IsEnd() || !it2_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq1_;
        IConstEnumeratorPtr<T> it1_;
        LazySequencePtr<T> seq2_;
        IConstEnumeratorPtr<T> it2_;
    };

    template <typename T2, typename Func>
    class MapGenerator : public IGenerator {
    public:
        MapGenerator(LazySequencePtr<T2> seq, Func func)
            : seq_(std::move(seq)), it_(seq_->GetConstEnumerator()), func_(std::move(func)) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            T2 res = it_->ConstDereference();
            it_->MoveNext();
            return func_(res);
        }

        bool HasNext() const override {
            return !it_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T2> seq_;
        IConstEnumeratorPtr<T2> it_;
        Func func_;
    };

    template <typename T1, typename T2>
    class ZipGenerator : public IGenerator {
    public:
        ZipGenerator(LazySequencePtr<T1> seq1, LazySequencePtr<T2> seq2)
            : seq1_(std::move(seq1)),
              it1_(seq1_->GetConstEnumerator()),
              seq2_(std::move(seq2)),
              it2_(seq2_->GetConstEnumerator()) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            T1 res1 = it1_->ConstDereference();
            it1_->MoveNext();
            T2 res2 = it2_->ConstDereference();
            it2_->MoveNext();
            return T{res1, res2};
        }

        bool HasNext() const override {
            return !it1_->IsEnd() || !it2_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T1> seq1_;
        IConstEnumeratorPtr<T1> it1_;
        LazySequencePtr<T2> seq2_;
        IConstEnumeratorPtr<T2> it2_;
    };

    template <typename Func>
    class WhereGenerator : public IGenerator {
    public:
        WhereGenerator(LazySequencePtr<T> seq, Func func)
            : seq_(std::move(seq)), it_(seq_->GetConstEnumerator()), func_(std::move(func)) {
        }

        T GetNext() override {
            if (!HasNext()) {
                throw std::out_of_range("GetNext: no next element");
            }
            while (!it_->IsEnd() && !func_(it_->ConstDereference())) {
                it_->MoveNext();
            }
            if (it_->IsEnd()) {
                throw std::out_of_range("GetNext: no next element");
            }
            T res = it_->ConstDereference();
            it_->MoveNext();
            return res;
        }

        bool HasNext() const override {
            return !it_->IsEnd();
        }

        std::optional<T> TryGetNext() override {
            try {
                return GetNext();
            } catch (const std::exception& ex) {
                return std::nullopt;
            }
        }

    private:
        LazySequencePtr<T> seq_;
        IConstEnumeratorPtr<T> it_;
        Func func_;
    };

public:
    virtual ~LazySequence() = default;

    LazySequence()
        : items_(std::make_unique<ArraySequence<T>>()), generator_(std::make_unique<SequenceGenerator>()), length_(0) {
    }

    LazySequence(const T* items, int count)
        : length_(count),
          items_(std::make_unique<ArraySequence<T>>(items, count)),
          generator_(std::make_unique<SequenceGenerator>()) {
    }

    LazySequence(SequencePtr<T> seq)
        : length_(seq->GetLength()),
          items_(std::make_unique<ArraySequence<T>>(*seq)),
          generator_(std::make_unique<SequenceGenerator>()) {
    }

    LazySequence(LazySequencePtr<T> seq)
        : length_(seq->GetLength()),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<DefaultGenerator>(std::move(seq))) {
    }

    template <typename Func>
    LazySequence(Func func, SequencePtr<T> seq, size_t arity)
        : length_(Cardinals::N0),
          items_(std::make_unique<ArraySequence<T>>(std::move(seq))),
          generator_(std::make_unique<FunctionGenerator<Func>>(this, std::move(func), arity)) {
        if (items_->GetLength() < arity) {
            throw std::runtime_error("Given less starting elements than arity");
        }
    }

    // Subsequence
    LazySequence(LazySequencePtr<T> seq, size_t startIndex, size_t endIndex, SubSequenceTag)
        : length_(endIndex - startIndex + 1),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<SubsequenceGenerator>(std::move(seq), startIndex, endIndex)) {
    }

    // Skip
    LazySequence(LazySequencePtr<T> seq, size_t startIndex, size_t endIndex, SkipTag)
        : length_(seq->length_ - (endIndex - startIndex + 1)),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<SkipGenerator>(std::move(seq), startIndex, endIndex)) {
    }

    // Append
    LazySequence(LazySequencePtr<T> seq, const T& item, AppendTag)
        : length_(seq->length_ + 1),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<AppendGenerator>(std::move(seq), item)) {
    }

    // InsertAt
    LazySequence(LazySequencePtr<T> seq, const T& item, size_t index, InsertTag)
        : length_(seq->length_ + 1),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<InsertGenerator>(std::move(seq), item, index)) {
    }

    // Concat
    LazySequence(LazySequencePtr<T> seq1, LazySequencePtr<T> seq2, ConcatTag)
        : length_(seq1->length_ + seq2->length_),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<ConcatGenerator>(std::move(seq1), std::move(seq2))) {
    }

    // Map
    template <typename T2, typename Func>
    LazySequence(LazySequencePtr<T2> seq, Func func, MapTag)
        : length_(seq->length_),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<MapGenerator<T2, Func>>(std::move(seq), std::move(func))) {
    }

    // Where
    template <typename Func>
    LazySequence(LazySequencePtr<T> seq, Func func, WhereTag)
        : length_(seq->length_),  // not bounding, generating till done
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<WhereGenerator<Func>>(std::move(seq), std::move(func))) {
    }

    // Zip
    template <typename T1, typename T2>
    LazySequence(LazySequencePtr<T1> seq1, LazySequencePtr<T2> seq2, ZipTag)
        : length_(std::min(seq1->length_, seq2->length_)),
          items_(std::make_unique<ArraySequence<T>>()),
          generator_(std::make_unique<ZipGenerator<T1, T2>>(std::move(seq1), std::move(seq2))) {
    }

public:
    const T& GetFirst() const {
        return GetIndex(0);
    }

    const T& GetLast() const {
        while (generator_->HasNext()) {
            items_->Append(generator_->GetNext());
        }
        return items_->GetLast();
    }

    const T& GetIndex(size_t index) const {
        if (index >= items_->GetLength()) {
            for (size_t i = items_->GetLength(); i <= index; ++i) {
                items_->Append(generator_->GetNext());
            }
        }
        return items_->Get(index);
    }

    LazySequencePtr<T> GetSubsequence(size_t startIndex, size_t endIndex) {
        if (startIndex > endIndex) {
            throw std::out_of_range("GetSubsequence: startIndex is greater than endIndex");
        }
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), startIndex, endIndex, SubSequenceTag{});
    }

    LazySequencePtr<T> Skip(size_t startIndex, size_t endIndex) {
        if (startIndex > endIndex) {
            throw std::out_of_range("Skip: startIndex is greater than endIndex");
        }
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), startIndex, endIndex, SkipTag{});
    }

    Cardinal GetLength() const {
        return length_;
    }

    size_t GetMaterializedCount() const {
        return items_->GetLength();
    }

    bool HasNext() const {
        return generator_->HasNext();
    }

    LazySequencePtr<T> Append(const T& item) {
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), item, AppendTag{});
    }

    LazySequencePtr<T> Prepend(const T& item) {
        return InsertAt(item, 0);
    }

    LazySequencePtr<T> InsertAt(const T& item, size_t index) {
        if (length_.IsFinite() && index > length_.GetFinite()) {
            throw std::out_of_range("InsertAt: index is greater than length");
        }
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), item, index, InsertTag{});
    }

    LazySequencePtr<T> Concat(LazySequencePtr<T> seq) {
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), std::move(seq), ConcatTag{});
    }

    template <typename Func>
    auto Map(Func func) {
        using T2 = typename std::invoke_result_t<Func, T>;
        return std::make_shared<LazySequence<T2>>(this->shared_from_this(), std::move(func), MapTag{});
    }

    template <typename T2, typename Func>
    auto Reduce(const T2& start, Func func) {
        T2 res = start;
        for (auto it = GetConstEnumerator(); !it->IsEnd(); it->MoveNext()) {
            res = func(res, it->ConstDereference());
        }
        return res;
    }

    template <typename Func>
    auto Where(Func func) {
        return std::make_shared<LazySequence<T>>(this->shared_from_this(), std::move(func), WhereTag{});
    }

    template <typename T2>
    auto Zip(LazySequencePtr<T2> seq) {
        return std::make_shared<LazySequencePtr<std::pair<T, T2>>>(this->shared_from_this(), std::move(seq), ZipTag{});
    }

    IConstEnumeratorPtr<T> GetConstEnumerator() {
        return std::make_shared<LazySequenceIterator<T>>(this->shared_from_this());
    }

private:
    const Cardinal length_;
    const std::unique_ptr<Sequence<T>> items_;
    const std::unique_ptr<IGenerator> generator_;
};
