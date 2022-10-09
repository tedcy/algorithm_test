#pragma once

#include "/root/env/snippets/cpp/cpp_test_common.h"
#include <type_traits>

#ifdef DebugCount
static int64_t zsetCount = 0;
#endif

template <typename KeyT>
class SkipList {
    static constexpr double _P = 0.25;
    static constexpr int MAX_LEVEL = 32;
    template <typename>
    friend class SkipListDebuger;
public:
    struct Node {
        KeyT key;
        struct Level {
            Node* next;
        } level[];
    };
    Node* newNode(const KeyT& key, int level) {
        auto node = newNode(level);
        node->key = key;
        return node;
    }
    Node* newNode(int level) {
        auto node = static_cast<Node*>(operator new(sizeof(Node) + level * sizeof(typename Node::Level)));
        if constexpr (!std::is_pod_v<KeyT>) {
            return new (node) Node;
        }
        return node;
    }
    SkipList() {
        head_ = newNode(MAX_LEVEL);
        for (auto i = 0;i < MAX_LEVEL;i++) {
            head_->level[i].next = nullptr;
        }
    }
    ~SkipList() {
        auto cur = head_->level[0].next;
        while(cur) {
            auto next = cur->level[0].next;
            delete cur;
            cur = next;
        }
        delete head_;
    }
    bool insert(const KeyT& key) {
        Node* update[MAX_LEVEL];

        auto cur = head_;
        for (auto level = level_ - 1;level >= 0;level--) {
            while(1) {
                if (auto next = cur->level[level].next; next && next->key < key) {
                    cur = next;
                }else {
                    if (next && next->key == key) {
                        return false;
                    }
                    break;
                }
            }
            update[level] = cur;
        }
        auto level = getRandomLevel();
        if (level > level_) {
            for (auto i = level_; i < level; i++) {
                update[i] = head_;
            }
            level_ = level;
        }
        cur = newNode(key, level);
        for (auto i = 0;i < level; i++) {
            cur->level[i].next = update[i]->level[i].next;
            update[i]->level[i].next = cur;
        }

        return true;
    }
    bool erase(const KeyT& key) {
        Node* update[MAX_LEVEL];

        auto cur = head_;
        for (auto level = level_ - 1;level >= 0;level--) {
            while(1) {
                if (auto next = cur->level[level].next; next && next->key < key) {
                    cur = next;
                }else {
                    break;
                }
            }
            update[level] = cur;
        }
        cur = cur->level[0].next;
        if (cur && cur->key == key) {
            for (auto level = 0;level < level_;level++) {
                if (auto& next = update[level]->level[level].next; next == cur) {
                    next = cur->level[level].next;
                }
            }
            while (level_ > 1 && head_->level[level_ - 1].next == nullptr) {
                level_--;
            }
            delete cur;
            return true;
        }
        return false;
    }
    Node* find(const KeyT& key) {
        auto cur = head_;
        for (int level = level_ - 1;level >= 0;level--) {
            while(1) {
                if (auto next = cur->level[level].next; next && next->key <= key) {
                    cur = next;
                }else {
                    break;
                }
            }
            if (cur && cur->key == key) {
                return cur;
            }
        }
        return nullptr;
    }
private:
    int getRandomLevel() {
        static constexpr int threshold = _P * RAND_MAX;
        int level = 1;
        while(rand() < threshold) {
            level++;
        }
        return min(level, MAX_LEVEL);
    }
    Node* head_;
    int level_ = 1;
};

template <typename KeyT>
class SkipListDebuger {
public:
    SkipList<KeyT> t_;
    using Node = typename SkipList<KeyT>::Node;
    bool debugCheck(set<int> &expectResult) {
        set<int> s;
        Node* cur = t_.head_->level[0].next;
        while(cur != nullptr) {
            s.insert(cur->key);
            cur = cur->level[0].next;
        }
        if (s == expectResult) {
            return true;
        }
        return false;
    }
};
