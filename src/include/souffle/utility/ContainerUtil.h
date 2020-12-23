/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ContainerUtil.h
 *
 * @brief Datalog project utilities
 *
 ***********************************************************************/

#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace souffle {

// -------------------------------------------------------------------------------
//                           General Container Utilities
// -------------------------------------------------------------------------------

template <typename A>
using Own = std::unique_ptr<A>;

template <typename A>
using VecOwn = std::vector<Own<A>>;

template <typename A, typename B = A, typename... Args>
Own<A> mk(Args&&... xs) {
    return Own<A>(new B(std::forward<Args>(xs)...));
}

/**
 * Use to range-for iterate in reverse.
 * Assumes `std::rbegin` and `std::rend` are defined for type `A`.
 */
template <typename A>
struct reverse {
    reverse(A& iterable) : iterable(iterable) {}
    A& iterable;

    auto begin() {
        return std::rbegin(iterable);
    }

    auto end() {
        return std::rend(iterable);
    }
};

/**
 * A utility to check generically whether a given element is contained in a given
 * container.
 */
template <typename C>
bool contains(const C& container, const typename C::value_type& element) {
    return std::find(container.begin(), container.end(), element) != container.end();
}

// TODO: Detect and generalise to other set types?
template <typename A>
bool contains(const std::set<A>& container, const A& element) {
    return container.find(element) != container.end();
}

/**
 * Version of contains specialised for maps.
 *
 * This workaround is needed because of set container, for which value_type == key_type,
 * which is ambiguous in this context.
 */
template <typename C>
bool contains(const C& container, const typename C::value_type::first_type& element) {
    return container.find(element) != container.end();
}

/**
 * Returns the first element in a container that satisfies a given predicate,
 * nullptr otherwise.
 */
template <typename C>
typename C::value_type getIf(const C& container, std::function<bool(const typename C::value_type)> pred) {
    auto res = std::find_if(container.begin(), container.end(),
            [&](const typename C::value_type item) { return pred(item); });
    return res == container.end() ? nullptr : *res;
}

/**
 * Get value for a given key; if not found, return default value.
 */
template <typename C>
typename C::mapped_type const& getOr(
        const C& container, typename C::key_type key, const typename C::mapped_type& defaultValue) {
    auto it = container.find(key);

    if (it != container.end()) {
        return it->second;
    } else {
        return defaultValue;
    }
}

/**
 * A utility function enabling the creation of a vector with a fixed set of
 * elements within a single expression. This is the base case covering empty
 * vectors.
 */
template <typename T>
std::vector<T> toVector() {
    return std::vector<T>();
}

/**
 * A utility function enabling the creation of a vector with a fixed set of
 * elements within a single expression. This is the step case covering vectors
 * of arbitrary length.
 */
template <typename T, typename... R>
std::vector<T> toVector(const T& first, const R&... rest) {
    return {first, rest...};
}

/**
 * A utility function enabling the creation of a vector of pointers.
 */
template <typename T>
std::vector<T*> toPtrVector(const std::vector<std::unique_ptr<T>>& v) {
    std::vector<T*> res;
    for (auto& e : v) {
        res.push_back(e.get());
    }
    return res;
}

/**
 * Applies a function to each element of a vector and returns the results.
 */
template <typename A, typename F /* : A -> B */>
auto map(const std::vector<A>& xs, F&& f) {
    std::vector<decltype(f(xs[0]))> ys;
    ys.reserve(xs.size());
    for (auto&& x : xs) {
        ys.emplace_back(f(x));
    }
    return ys;
}

// -------------------------------------------------------------------------------
//                             Cloning Utilities
// -------------------------------------------------------------------------------

template <typename A>
auto clone(const std::vector<A*>& xs) {
    std::vector<std::unique_ptr<A>> ys;
    ys.reserve(xs.size());
    for (auto&& x : xs) {
        ys.emplace_back(x ? std::unique_ptr<A>(x->clone()) : nullptr);
    }
    return ys;
}

template <typename A>
auto clone(const std::vector<std::unique_ptr<A>>& xs) {
    std::vector<std::unique_ptr<A>> ys;
    ys.reserve(xs.size());
    for (auto&& x : xs) {
        ys.emplace_back(x ? std::unique_ptr<A>(x->clone()) : nullptr);
    }
    return ys;
}

// -------------------------------------------------------------
//                            Iterators
// -------------------------------------------------------------
/**
 * A wrapper for an iterator that transforms values returned by
 * the underlying iter.
 *
 * @tparam Iter ... the type of wrapped iterator
 * @tparam F    ... the function to apply
 *
 */
template <typename Iter, typename F>
class TransformIterator {
    using iter_t = std::iterator_traits<Iter>;
    using difference_type = typename iter_t::difference_type;
    using result_type = decltype(std::declval<F&>()(*std::declval<Iter>()));

public:
    // some constructors
    TransformIterator() = default;
    TransformIterator(Iter iter, F f) : iter(std::move(iter)), fun(std::move(f)) {}

    // defaulted copy and move constructors
    TransformIterator(const TransformIterator&) = default;
    TransformIterator(TransformIterator&&) = default;

    // default assignment operators
    TransformIterator& operator=(const TransformIterator&) = default;
    TransformIterator& operator=(TransformIterator&&) = default;

    /* The equality operator as required by the iterator concept. */
    bool operator==(const TransformIterator& other) const {
        return iter == other.iter;
    }

    /* The not-equality operator as required by the iterator concept. */
    bool operator!=(const TransformIterator& other) const {
        return iter != other.iter;
    }

    /* The deref operator as required by the iterator concept. */
    auto operator*() const -> result_type {
        return fun(*iter);
    }

    /* Support for the pointer operator. */
    auto operator->() const {
        return &**this;
    }

    /* The increment operator as required by the iterator concept. */
    TransformIterator& operator++() {
        ++iter;
        return *this;
    }

    TransformIterator operator++(int) {
        auto res = *this;
        ++iter;
        return res;
    }

    TransformIterator& operator--() {
        --iter;
        return *this;
    }

    TransformIterator operator--(int) {
        auto res = *this;
        --iter;
        return res;
    }

    auto operator[](difference_type ii) const {
        return f(iter[ii]);
    }

private:
    /* The nested iterator. */
    Iter iter;
    F fun;
};

template <typename Iter, typename F>
auto makeTransformIter(Iter&& iter, F&& f) {
    return TransformIterator<std::remove_reference_t<Iter>, std::remove_reference_t<F>>(
            std::forward<Iter>(iter), std::forward<F>(f));
}

/**
 * A wrapper for an iterator obtaining pointers of a certain type,
 * dereferencing values before forwarding them to the consumer.
 */
namespace detail {
inline auto iterDeref = [](auto& p) -> decltype(*p) { return *p; };
}

template <typename Iter>
using IterDerefWrapper = TransformIterator<Iter, decltype(detail::iterDeref)>;

/**
 * A factory function enabling the construction of a dereferencing
 * iterator utilizing the automated deduction of template parameters.
 */
template <typename Iter>
auto derefIter(Iter&& iter) {
    return makeTransformIter(std::forward<Iter>(iter), detail::iterDeref);
}

// -------------------------------------------------------------
//                             Ranges
// -------------------------------------------------------------

/**
 * A utility class enabling representation of ranges by pairing
 * two iterator instances marking lower and upper boundaries.
 */
template <typename Iter>
struct range {
    // the lower and upper boundary
    Iter a, b;

    // a constructor accepting a lower and upper boundary
    range(Iter a, Iter b) : a(std::move(a)), b(std::move(b)) {}

    // default copy / move and assignment support
    range(const range&) = default;
    range(range&&) = default;
    range& operator=(const range&) = default;

    // get the lower boundary (for for-all loop)
    Iter& begin() {
        return a;
    }
    const Iter& begin() const {
        return a;
    }

    // get the upper boundary (for for-all loop)
    Iter& end() {
        return b;
    }
    const Iter& end() const {
        return b;
    }

    // emptiness check
    bool empty() const {
        return a == b;
    }

    // splits up this range into the given number of partitions
    std::vector<range> partition(int np = 100) {
        // obtain the size
        int n = 0;
        for (auto i = a; i != b; ++i) {
            n++;
        }

        // split it up
        auto s = n / np;
        auto r = n % np;
        std::vector<range> res;
        res.reserve(np);
        auto cur = a;
        auto last = cur;
        int i = 0;
        int p = 0;
        while (cur != b) {
            ++cur;
            i++;
            if (i >= (s + (p < r ? 1 : 0))) {
                res.push_back({last, cur});
                last = cur;
                p++;
                i = 0;
            }
        }
        if (cur != last) {
            res.push_back({last, cur});
        }
        return res;
    }
};

/**
 * A utility function enabling the construction of ranges
 * without explicitly specifying the iterator type.
 *
 * @tparam Iter .. the iterator type
 * @param a .. the lower boundary
 * @param b .. the upper boundary
 */
template <typename Iter>
range<Iter> make_range(const Iter& a, const Iter& b) {
    return range<Iter>(a, b);
}

// -------------------------------------------------------------------------------
//                             Equality Utilities
// -------------------------------------------------------------------------------

/**
 * Cast the values, from baseType to toType and compare using ==. (if casting fails -> return false.)
 *
 * @tparam baseType, initial Type of values
 * @tparam toType, type where equality comparison takes place.
 */
template <typename toType, typename baseType>
bool castEq(const baseType* left, const baseType* right) {
    if (auto castedLeft = dynamic_cast<const toType*>(left)) {
        if (auto castedRight = dynamic_cast<const toType*>(right)) {
            return castedLeft == castedRight;
        }
    }
    return false;
}

/**
 * A functor class supporting the values pointers are pointing to.
 */
template <typename T>
struct comp_deref {
    bool operator()(const T& a, const T& b) const {
        if (a == nullptr) {
            return false;
        }
        if (b == nullptr) {
            return false;
        }
        return *a == *b;
    }
};

/**
 * A function testing whether two containers are equal with the given Comparator.
 */
template <typename Container, typename Comparator>
bool equal_targets(const Container& a, const Container& b, const Comparator& comp) {
    // check reference
    if (&a == &b) {
        return true;
    }

    // check size
    if (a.size() != b.size()) {
        return false;
    }

    // check content
    return std::equal(a.begin(), a.end(), b.begin(), comp);
}

/**
 * A function testing whether two containers of pointers are referencing equivalent
 * targets.
 */
template <typename T, template <typename...> class Container>
bool equal_targets(const Container<T*>& a, const Container<T*>& b) {
    return equal_targets(a, b, comp_deref<T*>());
}

/**
 * A function testing whether two containers of unique pointers are referencing equivalent
 * targets.
 */
template <typename T, template <typename...> class Container>
bool equal_targets(const Container<std::unique_ptr<T>>& a, const Container<std::unique_ptr<T>>& b) {
    return equal_targets(a, b, comp_deref<std::unique_ptr<T>>());
}

/**
 * A function testing whether two maps of unique pointers are referencing to equivalent
 * targets.
 */
template <typename Key, typename Value>
bool equal_targets(
        const std::map<Key, std::unique_ptr<Value>>& a, const std::map<Key, std::unique_ptr<Value>>& b) {
    auto comp = comp_deref<std::unique_ptr<Value>>();
    return equal_targets(
            a, b, [&comp](auto& a, auto& b) { return a.first == b.first && comp(a.second, b.second); });
}

}  // namespace souffle

namespace std {
template <typename Iter, typename F>
struct iterator_traits<souffle::TransformIterator<Iter, F>> {
    using iter_t = std::iterator_traits<Iter>;
    using iter_tag = typename iter_t::iterator_category;
    using difference_type = typename iter_t::difference_type;
    using result_type = decltype(std::declval<F&>()(*std::declval<Iter>()));
    using value_type = std::remove_cv_t<std::remove_reference_t<result_type>>;
    using iterator_category = std::conditional_t<std::is_base_of_v<std::random_access_iterator_tag, iter_tag>,
            std::random_access_iterator_tag, iter_tag>;
};
}  // namespace std
