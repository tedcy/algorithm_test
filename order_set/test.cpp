#include "rb_tree.h"
//#include "t_zset_without_span.h"
#include "t_zset.h"
#include "skiplist.h"

int performanceElementSize = 0;
int randTestElementSize = 0;
int randTestRepeatCount = 0;
int randMax = 0;

struct SeqGenerator {
    static inline bool isUnique = false;
    static void setUnique() {
        isUnique = true;
    }
    explicit SeqGenerator(uint count) {
        set<int> uniqueSet;
        while(insertSeq_.size() < count) {
            auto v = rand() % randMax;
            if (isUnique) {
                if (uniqueSet.count(v) != 0) {
                    continue;
                }
                uniqueSet.insert(v);
            }
            insertSeq_.push_back(v);
        }
        eraseSeq_ = insertSeq_;
        random_shuffle(eraseSeq_.begin(), eraseSeq_.end());
    }
    SeqGenerator(const vector<int> &insertSeq,
        const vector<int> &eraseSeq) : 
        insertSeq_(insertSeq), eraseSeq_(eraseSeq) {
    }
    vector<int> insertSeq_;
    vector<int> eraseSeq_;
};

template <typename T, typename ...Types>
void testInOrderBase() {
    T t;
    set<int> expectResult;

    SeqGenerator seq({1,4,0,2,5,3,6}, 
        {1,0,3,4,6,5,2});
    for (auto& v : seq.insertSeq_) {
        cout << "insert" << LOGV(v) << endl;
        t.insert(v);
        expectResult.insert(v);
        t.template debugPrint<true>();
        if (!t.debugCheck(expectResult)) {
            break;
        }
        cout << "----------" << endl;
    }

    for (auto& v : seq.eraseSeq_) {
        cout << "erase" << LOGV(v) << endl;
        t.erase(v);
        expectResult.erase(v);
        t.template debugPrint<true>();
        if (!t.debugCheck(expectResult)) {
            break;
        }
        cout << "----------" << endl;
    }

#ifdef CHECKMEMLEAK
    if (!t.debugNodes_.empty()) {
        cout << "failed in erase" << endl;
    }
#endif
    if constexpr (sizeof...(Types) != 0) {
        testInOrderBase<Types...>();
    }

    //cout << "----------" << endl;
    //auto t1 = t;
    //t1.debugPrint<true>();
    //t1.debugCheck();
}
template <typename ...Type>
void testInOrder() {
    testInOrderBase<Type...>();
}

template <typename T, typename ...Types>
void testRandBase() {
    set<int> expectResult;
    for (int i = 0;i < randTestRepeatCount ;i++) {
        T t;
        SeqGenerator seq(randTestElementSize);
        bool failed = false;
        for (auto &v : seq.insertSeq_) {
            t.t_.insert(v);
            expectResult.insert(v);
            if (!t.debugCheck(expectResult)) {
                failed = true;
                cout << "failed in insert" << endl;
                break;
            }
        }
        if (!failed) {
            for (auto &v : seq.eraseSeq_) {
                auto node = t.t_.find(v);
                if (node->key != v) {
                    failed = true;
                    cout << "failed in find" << endl;
                    break;
                }
            }
        }
        if (!failed) {
            for (auto &v : seq.eraseSeq_) {
                t.t_.erase(v);
                expectResult.erase(v);
                if (!t.debugCheck(expectResult)) {
                    failed = true;
                    cout << "failed in erase" << endl;
                    break;
                }
            }
#ifdef CHECKMEMLEAK
            if (!t.debugNodes_.empty()) {
                failed = true;
                cout << "failed in erase" << endl;
            }
#endif
        }
        if (failed) {
            for (auto &v : seq.insertSeq_) {
                cout << v << ",";
            }
            cout << endl;
            for (auto &v : seq.eraseSeq_) {
                cout << v << ",";
            }
            cout << endl;
            return;
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testRandBase<Types...>();
    }
}
template <typename ...Types>
void testRand() {
    testRandBase<Types...>();
}

template <typename T, typename ...Types>
void testInsertPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        profile::MemoryHolder h;
        T t;
        {
            //防止T析构也被计算
            Timer timer(getType<T>() + " insert");
            for (auto &v : seq.insertSeq_) {
                t.insert(v);
            }
        }
#ifdef MEM_TAG
        h.print();
#endif
    }
    if constexpr (sizeof...(Types) != 0) {
        testInsertPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testInsertPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testInsertPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testFindPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
        {
            Timer timer(getType<T>() + " find");
            for (auto &v : seq.eraseSeq_) {
                auto result = t.find(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testFindPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testFindPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testFindPerformanceNest<Types...>();
#ifdef DebugTreeCount
    cout << LOGV(seq.eraseSeq_.size()) << 
        LOGV(treeCount / seq.eraseSeq_.size()) << 
        LOGV(zsetCount / seq.eraseSeq_.size()) << endl;
#endif
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testErasePerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
        {
            Timer timer(getType<T>() + " erase");
            for (auto &v : seq.eraseSeq_) {
                t.erase(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testErasePerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testErasePerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testErasePerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testRankPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
        {
            Timer timer(getType<T>() + " getRank");
            for (auto &v : seq.eraseSeq_) {
                t.getRank(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testRankPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testRankPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testRankPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

int main() {
    Timer::setW(120);
    SeqGenerator::setUnique();
    srand(time(0));
    performanceElementSize = 1000000;
    randTestElementSize = 1000;
    randTestRepeatCount = 1000;
    randMax = INT_MAX;

    //testInOrder<RBTree, RankRBTree>();
    //testRand<RBTreeDebuger<RBTree>, RBTreeDebuger<RankRBTree>, SkipListDebuger>();
    testInsertPerformance<SkipList>();
    //testInsertPerformance<RBTree, RankRBTree, ZSet, SkipList, set<int>, unordered_set<int>>();
    //testFindPerformance<RBTree, RankRBTree, ZSet, SkipList, set<int>, unordered_set<int>>();
    //testErasePerformance<RBTree, RankRBTree, ZSet, SkipList, set<int>, unordered_set<int>>();
    //testRankPerformance<RankRBTree, ZSet>();
}
