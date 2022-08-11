#include <iostream>
#include <sstream>
#include <deque>
#include <chrono>
#include <pthread.h>
#include <thread>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>
#include <mutex>
#include <iconv.h>
#include <assert.h>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <limits.h>
#include <memory>
#include <queue>
#include <cmath>

using namespace std;
using namespace std::chrono;

namespace test_log{
    template<typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args&&... args) {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    class Any {
        public:
            template <typename T>
                Any(const T &t) : base_(make_unique<Data<T>>(t)) {
                }
            Any& operator=(const Any &other) {
                if (this == &other) {
                    return *this;
                }
                base_ = unique_ptr<Base>(other.base_->clone());
                return *this;
            }
            Any(const Any &other) {
                *this = other;
            }
            template <typename T>
                T any_cast() const {
                    return dynamic_cast<Data<T>*>(base_.get())->value_;
                }
        private:
            class Base {
                public:
                    virtual ~Base() = default;
                    virtual Base* clone() const = 0;
            };
            template <typename T>
                class Data : public Base {
                    public:
                        Data(const T &t) : value_(t) {}
                        virtual Data* clone() const override {
                            return new Data{*this};
                        }
                        T value_;
                };
            unique_ptr<Base> base_;
    };

    enum class Type {
        UNKNOWN,
        STRING,
        INT,
    };

    struct DataAny {
        DataAny(const char *value) : value_(string(value)), type_(Type::STRING) {
        }
        DataAny(const string &value) : value_(value), type_(Type::STRING) {
        }
        template <typename T>
            DataAny(T value, typename std::enable_if<std::is_integral<T>::value>::type* = 0) : value_(int64_t(value)), type_(Type::INT) {
            }
        string toString() const {
            if(type_ == Type::INT) {
                return to_string(value_.any_cast<int64_t>());
            }
            return value_.any_cast<string>();
        }
        private:
        Any value_;
        Type type_ = Type::UNKNOWN;
    };

    void log_debug(const vector<DataAny> &vs) {
        for (auto &v : vs) {
            cout << v.toString();
        }
        cout << endl;
    }

#define LOGVT(x) "|" , #x , "=" , x ,"|"
#define LOGV(x) "|" << #x << "=" << x << "|"
#ifdef LOG_TAG
    #define LDEBUG(...) test_log::log_debug({__VA_ARGS__})
#else
    #define LDEBUG(...)
#endif
}

struct Timer {
    Timer() = default;
    Timer(const string& name) : name_(name + ":") {} 
    virtual ~Timer() { 
        auto dur = system_clock::now() - tp;
        cout << setiosflags(ios::left) << std::setw(20) << name_ << "Cost " << duration_cast<milliseconds>(dur).count() << " ms" << endl; 
    } 
    string name_;
    system_clock::time_point tp = system_clock::now(); 
};
struct Bench : public Timer { 
    Bench() = default;
    Bench(const string& name) : Timer(name) {} 
    virtual ~Bench() { stop(); }
    void stop() { 
        auto dur = system_clock::now() - tp; 
        cout << setiosflags(ios::left) << std::setw(20) << name_ <<
            "Per op: " << duration_cast<nanoseconds>(dur).count() / std::max(val, 1L) << " ns" << endl; 
        auto perf = (double)val / duration_cast<milliseconds>(dur).count() / 10; 
        if (perf < 1) {
            cout << setiosflags(ios::left) << std::setw(20) << name_ <<
            "Performance: " << std::setprecision(3) << perf << " w/s" << endl; 
        }else {
            cout << setiosflags(ios::left) << std::setw(20) << name_ <<
            "Performance: " << perf << " w/s" << endl; 
        }
    } 
    Bench& operator++() { ++val; return *this; } 
    Bench& operator++(int) { ++val; return *this; } 
    Bench& add(long v) { val += v; return *this; } 
    long val = 0; 
};

int64_t TNOWMS() {
    std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    );
    return ms.count();
}

#define private public

template <typename T, typename getValueFuncT, typename CmpT = std::less<decltype(declval<getValueFuncT>()(declval<T>()))>>
class RangePopHeap {
    using KeyT = decltype(declval<getValueFuncT>()(declval<T>()));
public:
    RangePopHeap(const getValueFuncT& func) : cmp_(CmpT()), getValueFunc_(func) {
    }
    RangePopHeap(const getValueFuncT& func, int size) : cmp_(CmpT()), getValueFunc_(func) {
        vs_.reserve(size);
    }
    RangePopHeap(const getValueFuncT& func, const std::function<bool(const T&)> &cmp, int size) : 
        cmp_(cmp), getValueFunc_(func) {
        vs_.reserve(size);
    }
    void push(const T& v) {
        insert(v);
    }
    vector<T> rangePop(const KeyT& key) {
        bool ok = false;
        auto results = rangePopLittleData(key, vs_.size() / std::log2(vs_.size()), ok);
        if (!ok) {
            rangePopLargeData(key, results);
        }
        return results;
    }
    void rangePopLargeData(const KeyT& key, vector<T>& results) {
        vector<T> newVs;
        newVs.reserve(vs_.capacity());
        for (auto &v : vs_) {
            if (cmp_(getValueFunc_(v), key)) {
                newVs.emplace_back(move(v));
                continue;
            }
            results.emplace_back(move(v));
        }
        makeHeap(newVs);
    }
    //smallHeap, range pop value, util value > key
    vector<T> rangePopLittleData(const KeyT& key, int64_t countLimit, bool &ok) {
        ok = false;
        vector<T> results;
        results.reserve(vs_.capacity());
        while(!empty()) {
            auto &v = top();
            if (!countLimit--) {
                break;
            }
            if (cmp_(getValueFunc_(v), key)) {
                ok = true;
                break;
            }
            results.emplace_back(move(v));
            pop();
        }
        return results;
    }
    T& top() {
        return vs_[0];
    }
    void pop() {
        swap(vs_[0], vs_[vs_.size() - 1]);
        vs_.pop_back();
        heapify(0);
    }
    bool empty() {
        return vs_.empty();
    }
    void makeHeap(const vector<T> &vs) {
        vs_ = vs;
        for (int64_t i = vs.size() / 2;i >= 0;i--) {
            heapify(i);
        }
    }
private:
    void heapify(int64_t index) {
        int64_t leftIndex = left(index);
        int64_t rightIndex = right(index);
        int64_t largestIndex = index;
        if (leftIndex < int64_t(vs_.size()) && cmp_(getValue(index), getValue(leftIndex))) {
            largestIndex = leftIndex;
        }
        if (rightIndex < int64_t(vs_.size()) && cmp_(getValue(largestIndex), getValue(rightIndex))) {
            largestIndex = rightIndex;
        }
        if (largestIndex != index) {
            swap(vs_[index], vs_[largestIndex]);
            heapify(largestIndex);
        }
    }
    void insert(const T &v) {
        vs_.push_back(v);
        int64_t index = vs_.size() - 1;
        while(index > 0 && cmp_(getValue(parent(index)), getValue(index))) {
            swap(vs_[parent(index)], vs_[index]);
            index = parent(index);
        }
    }
    int64_t left(int64_t index) {
        return 2 * index + 1;
    }
    int64_t right(int64_t index) {
        return 2 * index + 2;
    }
    int64_t parent(int64_t index) {
        return (index - 1) / 2;
    }
    int64_t getValue(int64_t index) {
        return getValueFunc_(vs_[index]);
    }
    CmpT cmp_;
    getValueFuncT getValueFunc_;
    vector<T> vs_;
};

template <typename T, typename getValueFuncT, typename CmpT>
void checkHeap(T heap, const getValueFuncT& getValueFunc_) {
    if (heap.empty()) {
        return;
    }
    CmpT cmp;
    int last = getValueFunc_(heap.top());
    while(!heap.empty()) {
        auto &v = heap.top();
        auto realV = getValueFunc_(v);
        if (cmp(last, realV)) {
            cout << "wtf" << endl;
            break;
        }
        last = realV;
        heap.pop();
    }
}
int allCount = 0;
int nestCount = 0;
int now = 0;
using pairT = pair<int, string>;

void testAlgorithm() {
    cout << "test algorithm start" << endl;
    vector<int> vs;
    for (int i = 0;i < allCount;i++) {
        vs.push_back(rand() % INT_MAX);
    }
    auto f = [](const int &v) {
        return v;
    };
    RangePopHeap<int, decltype(f), std::greater<int>> heap(f);
    {
        Timer t;
        for (auto &v : vs) {
            heap.push(v);
        }
        checkHeap<decltype(heap), decltype(f), std::greater<int>>(heap, f);
    }
    cout << "test algorithm end" << endl;
}

void testIntOrderedPerformace() {
    cout << "test int64 ordered performance start" << endl;
    vector<int64_t> vs;
    for (int i = 0;i < allCount;i++) {
        vs.push_back(rand() % INT_MAX);
    }
    sort(vs.begin(), vs.end());
    {
        set<int64_t> s;
        Timer t("set");
        for (auto &v : vs) {
            s.insert(v);
        }
    }
    {
        priority_queue<int64_t, vector<int64_t>, std::greater<int>> q;
        Timer t("priority_queue");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    {
        auto f = [](const int64_t &v) {
            return v;
        };
        RangePopHeap<int64_t, decltype(f), std::greater<int>> q(f, allCount);
        Timer t("RangePopHeap");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    cout << "test int64 ordered performance end" << endl;
}

void testIntReverseOrderedPerformace() {
    cout << "test int64 reverse ordered performance start" << endl;
    vector<int64_t> vs;
    for (int i = 0;i < allCount;i++) {
        vs.push_back(rand() % INT_MAX);
    }
    sort(vs.begin(), vs.end());
    {
        set<int64_t> s;
        Timer t("set");
        for (auto &v : vs) {
            s.insert(v);
        }
    }
    {
        priority_queue<int64_t, vector<int64_t>, std::less<int>> q;
        Timer t("priority_queue");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    {
        auto f = [](const int64_t &v) {
            return v;
        };
        RangePopHeap<int64_t, decltype(f), std::less<int>> q(f, allCount);
        Timer t("RangePopHeap");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    cout << "test int64 reverse ordered performance end" << endl;
}

void testIntRandomPerformace() {
    cout << "test int64 random performance start" << endl;
    vector<int64_t> vs;
    for (int i = 0;i < allCount;i++) {
        vs.push_back(rand() % INT_MAX);
    }
    {
        set<int64_t> s;
        Timer t("set");
        for (auto &v : vs) {
            s.insert(v);
        }
    }
    {
        priority_queue<int64_t, vector<int64_t>, std::greater<int>> q;
        Timer t("priority_queue");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    {
        auto f = [](const int64_t &v) {
            return v;
        };
        RangePopHeap<int64_t, decltype(f), std::greater<int>> q(f, allCount);
        Timer t("RangePopHeap");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    cout << "test int64 random performance end" << endl;
}

void testTimePerformance() {
    cout << "test int64 performance start" << endl;
    vector<int64_t> vs;
    for (int i = 0;i < allCount / nestCount;i++) {
        int64_t v = now + i;
        for (int j = 0;j < nestCount;j++) {
            vs.push_back(v - j);
        }
    }
    {
        set<int64_t> s;
        Timer t("set");
        for (auto &v : vs) {
            s.insert(v);
        }
    }
    {
        priority_queue<int64_t, vector<int64_t>, std::greater<int>> q;
        Timer t("priority_queue");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    {
        auto f = [](const int64_t &v) {
            return v;
        };
        RangePopHeap<int64_t, decltype(f), std::greater<int>> q(f, allCount);
        Timer t("RangePopHeap");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    cout << "test int64 performance end" << endl;
}

void testTimePairPerformance() {
    cout << "test pairT performance start" << endl;
    vector<pair<int64_t, string>> vs;
    for (int i = 0;i < allCount / nestCount;i++) {
        int64_t v = now + i;
        for (int j = 0;j < nestCount;j++) {
            string s = to_string(i * nestCount + j);
            vs.push_back({v - j, s});
        }
    }
    {
        map<int64_t, set<string>> m;
        Timer t("map");
        for (auto &v : vs) {
            m[v.first].insert(v.second);
        }
    }
    {
        priority_queue<pairT, vector<pairT>, std::greater<pairT>> q;
        Timer t("priority_queue");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    {
        auto f = [](const pairT &v) {
            return v.first;
        };
        RangePopHeap<pairT, decltype(f), std::greater<int64_t>> q(f, allCount);
        Timer t("RangePopHeap");
        for (auto &v : vs) {
            q.push(v);
        }
    }
    cout << "test pairT performance end" << endl;
}

void testTimePairRangePopPerformance(int keyPercent = 2) {
    cout << "test pairT rangePop performance start" << LOGV(keyPercent) << endl;
    vector<pair<int64_t, string>> vs;
    for (int i = 0;i < allCount / nestCount;i++) {
        int64_t v = now + i;
        for (int j = 0;j < nestCount;j++) {
            string s = to_string(i * nestCount + j);
            vs.push_back({v - j, s});
        }
    }

    int key = vs[vs.size() / keyPercent].first;
    {
        map<int64_t, set<string>> m;
        for (auto &v : vs) {
            m[v.first].insert(v.second);
        }
        Timer t("map");
        vector<pairT> result;
        result.reserve(allCount);
        for (auto iter = m.begin(); iter != m.end();) {
            if (iter->first > key) {
                break;
            }
            for (auto &v : iter->second) {
                result.emplace_back(iter->first, move(v));
            }
            iter = m.erase(iter);
        }
    }
    {
        auto f = [](const pairT &v) {
            return v.first;
        };
        RangePopHeap<pairT, decltype(f), std::greater<int>> q(f, allCount);
        for (auto &v : vs) {
            q.push(v);
        }
        auto q1 = q;
        auto q2 = q;
        {
            Timer t("rangePopLargeData");
            vector<pairT> vs;
            vs.reserve(allCount * nestCount);
            q.rangePopLargeData(key, vs);
        }
        checkHeap<decltype(q), decltype(f), std::greater<int>>(q, f);
        {
            Timer t("rangePopLittleData");
            int64_t countLimit = INT_MAX;
            bool ok = false;
            auto result = q1.rangePopLittleData(key, countLimit, ok);
        }
        checkHeap<decltype(q), decltype(f), std::greater<int>>(q1, f);
        {
            Timer t("rangePop");
            auto result = q2.rangePop(key);
        }
        checkHeap<decltype(q), decltype(f), std::greater<int>>(q2, f);
    }
    cout << "test pairT rangePop performance end" << endl;
}

int main() {
    srand(time(0));

    allCount = 1000000;
    nestCount = 10;
    now = TNOWMS();
    testAlgorithm();
    testIntOrderedPerformace();
    testIntReverseOrderedPerformace();
    testIntRandomPerformace();
    testTimePerformance();
    testTimePairPerformance();
    testTimePairRangePopPerformance();
    for (int i = 1;i < 30;i++) {
        testTimePairRangePopPerformance(i);
    }
    return 0;
}
