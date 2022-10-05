#include "rb_tree.h"
#include "t_zset_without_span.h"

int performanceElementSize = 0;
int randTestElementSize = 0;
int randTestRepeatCount = 0;
int randMax = 0;

struct SeqGenerator {
    explicit SeqGenerator(int count) {
        for (int i = 0;i < count;i++) {
            auto v = rand() % randMax;
            insertSeq_.push_back(v);
        }
        auto temp = insertSeq_;
        for (int i = 0;i < count;i++) {
            auto index = rand() % temp.size();
            eraseSeq_.push_back(temp[index]);
            temp[index] = temp[temp.size() - 1];
            temp.pop_back();
        }
        assert(temp.empty());
    }
    SeqGenerator(const vector<int> &insertSeq,
        const vector<int> &eraseSeq) : 
        insertSeq_(insertSeq), eraseSeq_(eraseSeq) {
    }
    vector<int> insertSeq_;
    vector<int> eraseSeq_;
};

void testBase() {
    RankRBTree t;

    SeqGenerator seq({1,4,0,2,5,3,6}, 
        {1,0,3,4,6,5,2});
    for (auto& v : seq.insertSeq_) {
        cout << "insert" << LOGV(v) << endl;
        t.insert(v);
        t.debugPrint<true>();
        if (!t.debugCheck()) {
            break;
        }
        cout << "----------" << endl;
    }

    for (auto& v : seq.eraseSeq_) {
        cout << "erase" << LOGV(v) << endl;
        t.erase(v);
        t.debugPrint<true>();
        if (!t.debugCheck()) {
            break;
        }
        cout << "----------" << endl;
    }

#ifdef CHECKMEMLEAK
    if (!t.debugNodes_.empty()) {
        cout << "failed in erase" << endl;
    }
#endif

    //cout << "----------" << endl;
    //auto t1 = t;
    //t1.debugPrint<true>();
    //t1.debugCheck();
}

//#include "t_zset.h"
#include "t_zset_without_span.h"

void testRand() {
    for (int i = 0;i < randTestRepeatCount ;i++) {
        RankRBTree t;
        SeqGenerator seq(randTestElementSize);
        bool failed = false;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
            if (!t.debugCheck()) {
                failed = true;
                cout << "failed in insert" << endl;
                break;
            }
        }
        if (!failed) {
            for (auto &v : seq.eraseSeq_) {
                t.erase(v);
                if (!t.debugCheck()) {
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
}
template <typename T, typename ...Types>
void testInsertPerformance(const SeqGenerator& seq) {
    {
        Timer timer(getType<T>() + " insert");
        profile::MemoryHolder h;
        T t;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
#ifdef LOG_TAG
        h.print();
#endif
    }
    if constexpr (sizeof...(Types) != 0) {
        testInsertPerformance<Types...>(seq);
    }
}
template <typename ...Types>
void testInsertPerformance() {
    SeqGenerator seq(performanceElementSize);
    testInsertPerformance<Types...>(seq);
}

template <typename T, typename ...Types>
void testFindPerformance(const SeqGenerator& seq) {
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
    if constexpr (sizeof...(Types) != 0) {
        testFindPerformance<Types...>(seq);
    }
}
template <typename ...Types>
void testFindPerformance() {
    SeqGenerator seq(performanceElementSize);
    //SeqGenerator seq({1,2,3,4}, {1,2,3,4});
    testFindPerformance<Types...>(seq);
#ifdef DebugTreeCount
    cout << LOGV(seq.eraseSeq_.size()) << 
        LOGV(treeCount / seq.eraseSeq_.size()) << 
        LOGV(zsetCount / seq.eraseSeq_.size()) << endl;
#endif

}

template <typename T, typename ...Types>
void testErasePerformance(const SeqGenerator& seq) {
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
    if constexpr (sizeof...(Types) != 0) {
        testErasePerformance<Types...>(seq);
    }
}
template <typename ...Types>
void testErasePerformance() {
    SeqGenerator seq(performanceElementSize);
    testErasePerformance<Types...>(seq);
}

int main() {
    srand(time(0));
    performanceElementSize = 1000000;
    randTestElementSize = 1000;
    randTestRepeatCount = 1000;
    randMax = INT_MAX;

    //testBase();
    //testRand();
    testInsertPerformance<RBTree, RankRBTree, ZSet, set<int>, unordered_set<int>>();
    testFindPerformance<RBTree, RankRBTree, ZSet, set<int>, unordered_set<int>>();
    testErasePerformance<RBTree, RankRBTree, ZSet, set<int>, unordered_set<int>>();
}
