#include "/root/env/snippets/cpp/cpp_test_common.h"

int performanceElementSize = 0;
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
        while(pushSeq_.size() < count) {
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
            pushSeq_.push_back(v);
        }
        removeSeq_ = pushSeq_;
        random_shuffle(removeSeq_.begin(), removeSeq_.end());
    }
    SeqGeneratorBase(const vector<int> &pushSeq,
        const vector<int> &removeSeq) {
        if constexpr(is_same_v<T, int>) {
            pushSeq_ = pushSeq;
            removeSeq_ = removeSeq;
        }else {
            for (auto &v : pushSeq) {
                pushSeq_.push_back(to_string(v));
            }
            for (auto &v : removeSeq) {
                removeSeq_.push_back(to_string(v));
            }
        }
    }
    vector<T> pushSeq_;
    vector<T> removeSeq_;
};

using SeqGenerator = SeqGeneratorBase<testType>;

template <typename K>
class RemovableLRU {
    struct Data {
        template<typename T>
        Data(T && k) : k(std::forward<T>(k)) {}
        K k;
        typename unordered_map<K, typename list<Data>::iterator>::iterator mIter;
    };
    list<Data> datas_;
    unordered_map<K, typename list<Data>::iterator> m_;
public:
    template <typename T>
    void pushBack(T && k) {
        datas_.emplace_back(std::forward<T>(k));
        typename list<Data>::iterator lIter = datas_.end(); 
        lIter--;
        auto ret = m_.insert({k, lIter});
        if (ret.second) {
            auto mIter = ret.first;
            mIter->second->mIter = mIter;
        }
    }
    template <typename T>
    void remove(T && k) {
        auto iter = m_.find(k);
        if (iter != m_.end()) {
            auto &lIter = iter->second;
            datas_.erase(lIter);
            m_.erase(iter);
        }
    }
    K popBack() {
        Data data = move(datas_.back());
        datas_.pop_back();
        m_.erase(data.mIter);
        return data.k;
    }
};

template <typename K>
class SimpleRemovableLRU {
    vector<K> vs_;
public:
    template <typename T>
    void pushBack(T && k) {
        vs_.emplace_back(move(k));
    }
    void remove(const K &k) {
        auto iter = find(vs_.begin(), vs_.end(), k);
        vs_.erase(iter);
    }
    K popBack() {
        auto v = move(vs_.back());
        vs_.pop_back();
        return v;
    }
};

template <typename T, typename ...Types>
void testPushPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        {
            //防止T析构也被计算
            Timer timer(getType<T>() + " pushBack");
            for (auto &v : seq.pushSeq_) {
                t.pushBack(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testPushPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testPushPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testPushPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testRemovePerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.pushSeq_) {
            t.pushBack(v);
        }
        {
            Timer timer(getType<T>() + " remove");
            for (auto &v : seq.removeSeq_) {
                t.remove(v);
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testRemovePerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testRemovePerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testRemovePerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}

template <typename T, typename ...Types>
void testPopPerformanceNest() {
    {
        SeqGenerator seq(performanceElementSize);
        T t;
        profile::MemoryHolder h;
        for (auto &v : seq.pushSeq_) {
            t.pushBack(v);
        }
        {
            Timer timer(getType<T>() + " popBack");
            for (uint i = 0;i < seq.pushSeq_.size();i++) {
                t.popBack();
            }
        }
    }
    if constexpr (sizeof...(Types) != 0) {
        testPopPerformanceNest<Types...>();
    }
}
template <typename ...Types>
void testPopPerformance() {
    cout << __FUNCTION__ << " start" << endl;
    testPopPerformanceNest<Types...>();
    cout << __FUNCTION__ << " end" << endl;
}
int main() {
    Timer::setW(60);
    SeqGenerator::setUnique();
    srand(time(0));
    performanceElementSize = 2000;
    randMax = INT_MAX;

    testPushPerformanceNest<RemovableLRU<testType>, SimpleRemovableLRU<testType>>();
    testRemovePerformanceNest<RemovableLRU<testType>, SimpleRemovableLRU<testType>>();
    testPopPerformanceNest<RemovableLRU<testType>, SimpleRemovableLRU<testType>>();
}

