#pragma once
#include <mutex>
#include <memory>
#include <list>
#include <algorithm>
#include <map>
#include <shared_mutex>
#include <vector>

template<typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_tabel {
private:
    class bucket_type {
        friend class threadsafe_lookup_tabel;
        typedef std::pair<Key, Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;
        public:
            Value value_for(Key const& key, Value const& default_value) {
                std::shared_lock<std::shared_mutex> lock(mutex);
                const bucket_iterator it = find_entry_for(key);
                return (it == data.end()) ? default_value : it->second;
            }

            void add_or_update_mapping(Key const& key, Value const& value) {
                std::shared_lock<std::shared_mutex> lock(mutex);
                const bucket_iterator it = find_entry_for(key);
                if (it == data.end()) {
                    data.push_back(bucket_value(key, value));
                } else {
                    it->second = value; // 赋值操作可能抛出异常
                }     
            }

            void remove_mapping(Key const& key) {
                std::shared_lock<std::shared_mutex> lock(mutex);
                const bucket_iterator it = find_entry_for(key);
                if (it != data.end()) {
                    data.erase(it);
                }
            }
        private:
            bucket_iterator find_entry_for(Key const& key) {
                return std::find_if(data.begin(), data.end(), 
                    [&](bucket_value const& item) {
                        return item.first == key;
                });
            }
        private:
            bucket_data data;
            mutable std::shared_mutex mutex;  // 支持多个读，一个写
    };
    
public:
    // 桶的数量最好是质数
    threadsafe_lookup_tabel(size_t num_buckets = 19, Hash const& hasher_ = Hash())
        : hasher(hasher_) {
        buckets.reserve(num_buckets);
        for (size_t i = 0; i < num_buckets; ++i) {
            buckets.push_back(std::make_unique<bucket_type>());
        }
    }

    threadsafe_lookup_tabel(threadsafe_lookup_tabel const&) = delete;
    threadsafe_lookup_tabel& operator=(threadsafe_lookup_tabel const&) = delete;

    Value value_for(Key const& key, Value const& default_value) {
        return get_bucket(key).value_for(key, default_value);
    } 

    void add_or_update_mapping(Key const& key, Value const& value) {
        return get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(Key const& key) {
        return get_bucket(key).remove_mapping(key);
    }
    // 数据快照
    std::map<Key, Value> get_map() const {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (auto& bucket : buckets) {
            locks.push_back(std::unique_lock<std::shared_mutex>(bucket->mutex));
        }
        std::map<Key, Value> res;
        for (auto& bucket : buckets) {
            for (auto& kv : bucket->data) {
                res[kv.first] = kv.second;
            }
        }
        return res;
    }
private:
   bucket_type& get_bucket(Key const& key) const {
        size_t const bucket_n = hasher(key) % buckets.size();
        return *buckets[bucket_n];
   }
private:
    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;
};
