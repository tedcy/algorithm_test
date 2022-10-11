#include "rb_tree.h"
//#include "t_zset_without_span.h"
#include "t_zset.h"
#include "skiplist.h"
#include <type_traits>

int performanceElementSize = 0;
int randTestElementSize = 0;
int randTestRepeatCount = 0;
int randMax = 0;

using testType = int;

template <typename T>
struct SeqGeneratorBase {
    static inline bool isUnique = false;
    static void setUnique() {
        isUnique = true;
    }
    explicit SeqGeneratorBase(uint count) {
        set<T> uniqueSet;
        while(insertSeq_.size() < count) {
            T v;
            int temp = rand() % randMax;
            if constexpr(is_same_v<T, int>) {
                v = temp;
            }else {
                v = to_string(temp);
            }
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
    SeqGeneratorBase(const vector<int> &insertSeq,
        const vector<int> &eraseSeq) {
        if constexpr(is_same_v<T, int>) {
            insertSeq_ = insertSeq;
            eraseSeq_ = eraseSeq;
        }else {
            for (auto &v : insertSeq) {
                insertSeq_.push_back(to_string(v));
            }
            for (auto &v : eraseSeq) {
                eraseSeq_.push_back(to_string(v));
            }
        }
    }
    vector<T> insertSeq_;
    vector<T> eraseSeq_;
};

using SeqGenerator = SeqGeneratorBase<testType>;

template <typename T, typename ...Types>
void testInOrderBase() {
    T t;
    set<testType> expectResult;

    SeqGenerator seq({1,4,0,2,5,3,6}, 
        {1,0,3,4,6,5,2});
    for (auto& v : seq.insertSeq_) {
        cout << "insert" << LOGV(v) << endl;
        t.t_.insert(v);
        expectResult.insert(v);
        t.t_.template debugPrint<true>();
        if (!t.debugCheck(expectResult)) {
            break;
        }
        cout << "----------" << endl;
    }

    for (auto& v : seq.eraseSeq_) {
        cout << "erase" << LOGV(v) << endl;
        t.t_.erase(v);
        expectResult.erase(v);
        t.t_.template debugPrint<true>();
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
    set<testType> expectResult;
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
void testGetRankPerformanceNest() {
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
        testGetRankPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testGetRankPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testGetRankPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testGetByRankPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
        vector<int> rankSeq;
        for (uint i = 1;i <= seq.insertSeq_.size();i++) {
            rankSeq.push_back(i);
        }
        random_shuffle(rankSeq.begin(), rankSeq.end());
        {
            Timer timer(getType<T>() + " getByRank");
            for (auto &v : rankSeq) {
                t.getByRank(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testGetByRankPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testGetByRankPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testGetByRankPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T>
struct hasNext {
    template <typename U>
    static void check(decltype(declval<U>().next(nullptr))*);
    template <typename U>
    static int check(...);

    enum {value = std::is_void_v<decltype(check<T>(nullptr))>};
};
template <typename T>
struct hasPushBack {
    template <typename U>
    static void check(decltype(declval<U>().push_back(declval<typename U::allocator_type::value_type>()))*);
    template <typename U>
    static int check(...);

    enum {value = std::is_void_v<decltype(check<T>(nullptr))>};
};

template <typename T, typename ...Types>
void testRangePerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            if constexpr (hasPushBack<T>::value) {
                t.push_back(v);
            }else {
                t.insert(v);
            }
        }
        {
            Timer timer(getType<T>() + " range");
            if constexpr(hasNext<T>::value) {
                for (auto cur = t.begin(); cur != t.end(); cur = t.next(cur)) {
                }
            }else {
                for (auto cur = t.begin(); cur != t.end(); cur++) {
                }
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testRangePerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testRangePerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testRangePerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testRangeByRankPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.insertSeq_) {
            t.insert(v);
        }
        vector<pair<int, int>> rankPairSeq;
        for (uint i = 0;i < 1000;i++) {
            int j = rand() % t.size();
            int k = rand() % t.size();
            if (j < k) {
                rankPairSeq.push_back({j, k});
            }else {
                rankPairSeq.push_back({k, j});
            }
        }
        {
            Timer timer(getType<T>() + " rangeByRank");
            for (auto &p : rankPairSeq) {
                auto start = t.getByRank(p.first);
                auto end = t.getByRank(p.second);
                for (auto cur = start; cur != end; cur = t.next(cur)) {
                }
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testRangeByRankPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testRangeByRankPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testRangeByRankPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

int main() {
    Timer::setW(60);
    SeqGenerator::setUnique();
    srand(time(0));
    performanceElementSize = 1000000;
    randTestElementSize = 1000;
    randTestRepeatCount = 1000;
    randMax = INT_MAX;
    
    //testInOrder<RBTreeDebuger<RBTree<testType>>, RBTreeDebuger<RBTree<testType>>>();
    //testRand<RBTreeDebuger<RBTree<testType>>, RBTreeDebuger<RankRBTree<testType>>, SkipListDebuger<testType>>();
    testInsertPerformance<RBTree<testType>, RankRBTree<testType>, SkipList<testType>, ZSet<testType>, set<testType>, unordered_set<testType>>();
    testFindPerformance<RBTree<testType>, RankRBTree<testType>, SkipList<testType>, ZSet<testType>, set<testType>, unordered_set<testType>>();
    testErasePerformance<RBTree<testType>, RankRBTree<testType>, SkipList<testType>, ZSet<testType>, set<testType>, unordered_set<testType>>();
    testGetRankPerformance<RankRBTree<testType>, ZSet<testType>>();
    testGetByRankPerformance<RankRBTree<testType>, ZSet<testType>>();
    testRangePerformance<RankRBTree<testType>, ZSet<testType>, set<testType>, list<testType>>();
    testRangeByRankPerformance<RankRBTree<testType>, ZSet<testType>>();
}
