#pragma once

#include "/root/env/snippets/cpp/cpp_test_common.h"

#ifdef DebugCount
static int64_t zsetCount = 0;
#endif

/* ZSETs use a specialized version of Skiplists */
#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.5     /* Skiplist P = 1/4 */

typedef struct zskiplistNode zskiplistNode;
typedef void (*handler_pt) (zskiplistNode *node);
struct zskiplistNode {
    // sds ele;
    // double score;
    int score; // 时间戳
    /* struct zskiplistNode *backward; 从后向前遍历时使用*/
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        /* int span; 这个存储的level间节点的个数，在定时器中并不需要*/ 
    } level[];
};

typedef struct zskiplist {
    // 添加一个free的函数
    struct zskiplistNode *header/*, *tail 并不需要知道最后一个节点*/;
    int length;
    int level;
} zskiplist;

#include <stdlib.h>
#include <stdio.h>

/* Create a skiplist node with the specified number of levels. */
inline zskiplistNode *zslCreateNode(int level, int score) {
    zskiplistNode *zn =
        (zskiplistNode*)malloc(sizeof(*zn)+level*sizeof(zskiplistNode::zskiplistLevel));
    zn->score = score;
    return zn;
}

// 创建跳表
inline zskiplist *zslCreate(void) {
    int j;
    zskiplist *zsl;

    zsl = (zskiplist*)malloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
    }
    return zsl;
}

/* Free a whole skiplist. */
inline void zslFree(zskiplist *zsl) {
    zskiplistNode *node = zsl->header->level[0].forward, *next;

    free(zsl->header);
    while(node) {
        next = node->level[0].forward;
        free(node);
        node = next;
    }
    free(zsl);
}

// 获取随机的层数
inline int zslRandomLevel(void) {
    int level = 1;
    while ((random()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

// 向跳表插入节点
inline zskiplistNode *zslInsert(zskiplist *zsl, int score){
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i, level;

    x = zsl->header;
	// update记录路径,方便建立节点时候的的前指针指向他
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                x->level[i].forward->score < score)
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    x = x->level[0].forward;
    if (x && score == x->score) {
        return nullptr;
    }
    level = zslRandomLevel();
    //printf("zskiplist add node level = %d\n", level);
	// 高度超过了原先最大高度,补充头指向新建的路径
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            update[i] = zsl->header;
        }
        zsl->level = level;
    }
    x = zslCreateNode(level,score);
    for (i = 0; i < level; i++) {
		// 新建节点接续他的前节点的后续指针
        x->level[i].forward = update[i]->level[i].forward;
		// 前指针指向新建节点
        update[i]->level[i].forward = x;
    }

    zsl->length++;
    return x;
}

inline void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) {
    int i;
	// 将行走路径上原先指向x的节点指向x的后续节点
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].forward = x->level[i].forward;
        } 
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

/* Delete an element with matching score/object from the skiplist. */
inline int zslDelete(zskiplist *zsl, int score) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
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
        free(x);
        return 1;
    }
    return 0; /* not found */
}

inline zskiplistNode* zslGetRank(zskiplist *zsl, double score) {
    zskiplistNode *x;

#ifdef DebugCount
    int count = 0;
#endif
    LDEBUG(LOGVT(score));
    x = zsl->header;
    for (int level = zsl->level-1; level >= 0; level--) {
        LDEBUG("next", LOGVT(score));
#ifdef DebugCount
        while (++zsetCount && ++count && x->level[level].forward &&
#endif
        while (x->level[level].forward &&
            (x->level[level].forward->score < score ||
                (x->level[level].forward->score == score))) {
            LDEBUG("forward", LOGVT(x->level[level].forward->score));
            x = x->level[level].forward;
        }
        if (x->score == score) {
            LDEBUG("finish", LOGVT(score), LOGVT(count));
            return x;
        }
    }
    return nullptr;
}

inline void zslPrint(zskiplist *zsl) {
    zskiplistNode *x;
    cout << LOGV(zsl->level) << endl;
    for (int level = zsl->level-1; level >= 0; level--) {
        cout << LOGV(level) << endl;
        x = zsl->header;
        while (x) {
            cout << LOGV(x->score) << endl;
            x = x->level[level].forward;
        }
    }
}

class ZSet {
public:
    ZSet() :list_(zslCreate()){
    }
    ~ZSet() {
        if (list_) {
#ifdef LOG_TAG
            zslPrint(list_);
#endif
            zslFree(list_);
        }
    }
    void insert(int score) {
        zslInsert(list_, score);
    }
    void erase(int score) {
        zslDelete(list_, score);
    }
    auto find(int score) {
        return zslGetRank(list_, score);
    }
private:
    zskiplist *list_ = nullptr;
};
