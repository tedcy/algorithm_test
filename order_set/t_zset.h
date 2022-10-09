#pragma once

#include "stdlib.h"
#include "assert.h"
#include <type_traits>

#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^32 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */

/* ZSETs use a specialized version of Skiplists */
template <typename T>
struct zskiplistNode {
    T score;
    struct zskiplistNode *backward;
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        unsigned int span;
    } level[];
};

template <typename T>
struct zskiplist {
    struct zskiplistNode<T> *header, *tail;
    unsigned long length;
    int level;
};

template <typename T>
inline zskiplistNode<T> *zslCreateNode(int level) {
    zskiplistNode<T> *zn = (zskiplistNode<T>*)malloc(sizeof(*zn)+level*sizeof(typename zskiplistNode<T>::zskiplistLevel));
    if constexpr (!std::is_pod_v<T>) {
        return new (zn) zskiplistNode<T>;
    }
    return zn;
}
template <typename T>
inline zskiplistNode<T> *zslCreateNode(int level, T score) {
    zskiplistNode<T> *zn = zslCreateNode<T>(level);
    zn->score = score;
    return zn;
}

template <typename T>
inline zskiplist<T> *zslCreate(void) {
    int j;
    zskiplist<T> *zsl;

    zsl = (zskiplist<T>*)malloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode<T>(ZSKIPLIST_MAXLEVEL);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
        zsl->header->level[j].span = 0;
    }
    zsl->header->backward = NULL;
    zsl->tail = NULL;
    return zsl;
}

template <typename T>
inline void zslFreeNode(zskiplistNode<T> *node) {
    free(node);
}

template <typename T>
inline void zslFree(zskiplist<T> *zsl) {
    zskiplistNode<T> *node = zsl->header->level[0].forward, *next;

    free(zsl->header);
    while(node) {
        next = node->level[0].forward;
        zslFreeNode(node);
        node = next;
    }
    free(zsl);
}

/* Returns a random level for the new skiplist node we are going to create.
 * The return value of this function is between 1 and ZSKIPLIST_MAXLEVEL
 * (both inclusive), with a powerlaw-alike distribution where higher
 * levels are less likely to be returned. */
inline  int zslRandomLevel(void) {
    int level = 1;
    while ((random()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

template <typename T>
inline zskiplistNode<T> *zslInsert(zskiplist<T> *zsl, T score) {
    zskiplistNode<T> *update[ZSKIPLIST_MAXLEVEL], *x;
    unsigned int rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward &&
            (x->level[i].forward->score < score)) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the key is not already inside, since we allow duplicated
     * scores, and the re-insertion of score and redis object should never
     * happen since the caller of zslInsert() should test in the hash table
     * if the element is already inside or not. */
    level = zslRandomLevel();
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = zsl->header;
            update[i]->level[i].span = zsl->length;
        }
        zsl->level = level;
    }
    x = zslCreateNode(level,score);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < zsl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == zsl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        zsl->tail = x;
    zsl->length++;
    return x;
}

/* Internal function used by zslDelete, zslDeleteByScore and zslDeleteByRank */
template <typename T>
inline void zslDeleteNode(zskiplist<T> *zsl, zskiplistNode<T> *x, zskiplistNode<T> **update) {
    int i;
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        zsl->tail = x->backward;
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

/* Delete an element with matching score/object from the skiplist. */
template <typename T>
inline int zslDelete(zskiplist<T> *zsl, T score) {
    zskiplistNode<T> *update[ZSKIPLIST_MAXLEVEL], *x;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score))
            x = x->level[i].forward;
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score) {
        zslDeleteNode(zsl, x, update);
        zslFreeNode(x);
        return 1;
    }
    return 0; /* not found */
}

/* Find the rank for an element by both score and key.
 * Returns 0 when the element cannot be found, rank otherwise.
 * Note that the rank is 1-based due to the span of zsl->header to the
 * first element. */
template <typename T>
inline unsigned long zslGetRank(zskiplist<T> *zsl, T score) {
    zskiplistNode<T> *x;
    unsigned long rank = 0;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score))) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        if (x->score == score) {
            return rank;
        }
    }
    return 0;
}

template <typename T>
inline zskiplistNode<T>* zslfind(zskiplist<T> *zsl, T score) {
    zskiplistNode<T> *x;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score))) {
            x = x->level[i].forward;
        }

        if (x->score == score) {
            return x;
        }
    }
    return nullptr;
}

template <typename KeyT>
class ZSet {
public:
    ZSet() :list_(zslCreate<KeyT>()){
    }
    ~ZSet() {
        if (list_) {
            zslFree(list_);
        }
    }
    void insert(KeyT score) {
        zslInsert(list_, score);
    }
    void erase(KeyT score) {
        zslDelete(list_, score);
    }
    auto find(KeyT score) {
        return zslfind(list_, score);
    }
    auto getRank(KeyT score) {
        return zslGetRank(list_, score);
    }
private:
    zskiplist<KeyT> *list_ = nullptr;
};
