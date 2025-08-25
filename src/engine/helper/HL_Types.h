/* Copyright(C) 2007-2025 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_TYPES_H
#define __HL_TYPES_H

#ifdef WIN32
# define tstring wstring
# define to_tstring to_wstring
#else
# define tstring string
# define to_tstring to_string
#endif

#ifdef WIN32
#define ALLOCA(X) _alloca(X)
#else
#define ALLOCA(X) alloca(X)
#endif

enum SdpDirection
{
  Sdp_Answer,
  Sdp_Offer
};


#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <map>

template<
    class K, class V,
    class HashK = std::hash<K>, class EqK = std::equal_to<K>,
    class HashV = std::hash<V>, class EqV = std::equal_to<V>
>
class BiMap {
public:
    using key_type = K;
    using mapped_type = V;

    BiMap(const std::map<K,V>& initializers) {
        for (const auto& item: initializers) {
            insert(item.first, item.second);
        }
    }

    // Insert a new (key, value) pair. Returns false if either key or value already exists.
    bool insert(const K& k, const V& v) {
        if (contains_key(k) || contains_value(v)) return false;
        auto ok = forward_.emplace(k, v);
        try {
            auto ov = reverse_.emplace(v, k);
            if (!ov.second) { // shouldn't happen given the guard above
                forward_.erase(k);
                return false;
            }
        } catch (...) {
            forward_.erase(k);
            throw;
        }
        return ok.second;
    }

    bool insert(K&& k, V&& v) {
        if (contains_key(k) || contains_value(v)) return false;
        auto ok = forward_.emplace(std::move(k), std::move(v));
        try {
            auto ov = reverse_.emplace(ok.first->second, ok.first->first); // use stored refs
            if (!ov.second) {
                forward_.erase(ok.first);
                return false;
            }
        } catch (...) {
            forward_.erase(ok.first);
            throw;
        }
        return ok.second;
    }

    // Replace value for existing key (and update reverse map). Returns false if value is already bound elsewhere.
    bool replace_by_key(const K& k, const V& new_v) {
        auto it = forward_.find(k);
        if (it == forward_.end()) return false;
        if (contains_value(new_v)) return false;
        // remove old reverse, insert new reverse, then update forward
        reverse_.erase(it->second);
        reverse_.emplace(new_v, k);
        it->second = new_v;
        return true;
    }

    // Replace key for existing value (and update forward map). Returns false if key is already bound elsewhere.
    bool replace_by_value(const V& v, const K& new_k) {
        auto it = reverse_.find(v);
        if (it == reverse_.end()) return false;
        if (contains_key(new_k)) return false;
        forward_.erase(it->second);
        forward_.emplace(new_k, v);
        it->second = new_k;
        return true;
    }

    // Erase by key/value. Return number erased (0 or 1).
    size_t erase_key(const K& k) {
        auto it = forward_.find(k);
        if (it == forward_.end()) return 0;
        reverse_.erase(it->second);
        forward_.erase(it);
        return 1;
    }

    size_t erase_value(const V& v) {
        auto it = reverse_.find(v);
        if (it == reverse_.end()) return 0;
        forward_.erase(it->second);
        reverse_.erase(it);
        return 1;
    }

    // Lookup
    bool contains_key(const K& k) const { return forward_.find(k) != forward_.end(); }
    bool contains_value(const V& v) const { return reverse_.find(v) != reverse_.end(); }

    const V* find_by_key(const K& k) const {
        auto it = forward_.find(k);
        return (it == forward_.end()) ? nullptr : &it->second;
    }
    const K* find_by_value(const V& v) const {
        auto it = reverse_.find(v);
        return (it == reverse_.end()) ? nullptr : &it->second;
    }

    // at() variants throw std::out_of_range on missing entries
    const V& at_key(const K& k) const { return forward_.at(k); }
    const K& at_value(const V& v) const { return reverse_.at(v); }

    void clear() noexcept {
        forward_.clear();
        reverse_.clear();
    }

    bool empty() const noexcept { return forward_.empty(); }
    size_t size() const noexcept { return forward_.size(); }

    // Reserve buckets for performance (optional)
    void reserve(size_t n) {
        forward_.reserve(n);
        reverse_.reserve(n);
    }

private:
    std::unordered_map<K, V, HashK, EqK> forward_;
    std::unordered_map<V, K, HashV, EqV> reverse_;
};

#endif
