#include <atomic>
#include <utility>
#include <iostream>
#include <stdint.h>
#include <stdio.h>

using namespace std;

#include "/root/env/snippets/cpp/cpp_test_common.h"

#undef LOG_DELAY
#define LOG_DELAY LOG

template <typename V>
class ConcurrentLinkedQueue {
    struct Node;
    class alignas(16) pointer {
        Node *p_ = nullptr;
        uint64_t count_;
        uint64_t getCount() {
            static atomic<uint64_t> count = {0};
            return ++count;
        }
        mutex& getMutex() {
            static mutex mtx;
            return mtx;
        }
        void pointerCheck(const pointer &p) {
            static map<int, Node*> pointerM;
            static mutex mtx;
            unique_lock<mutex> lock(mtx);
            auto iter = pointerM.find(p.count_);
            if (iter != pointerM.end()) {
                if (iter->second != p.p_) {
                    LOG_DELAY << LOGV(p) << LOGV(iter->first) << LOGV(iter->second) << getBacktrace() << endl;
                    abort();
                }
                return;
            }
            pointerM.insert({p.count_, p.p_});
        }
    public:
        pointer() = default;
        explicit pointer(Node *p) : p_(p), count_(getCount()) {
//            pointerCheck(*this);
        }
        void del() {
            if (p_) {
                delete p_;
                p_ = nullptr;
            }
        }
        pointer(const pointer &other) {
            p_ = other.p_;
            count_= other.count_;
//            pointerCheck(*this);
        }
        pointer& operator=(const pointer &other) {
            p_ = other.p_;
            count_= other.count_;
//            pointerCheck(*this);
            return *this;
        }
        void reset() {
            p_ = nullptr;
            count_ = getCount();
        }
        operator bool() const {
            return p_;
        }
        Node* operator->() {
            return p_;
        }
        Node* get() {
            return p_;
        }
        bool operator==(const pointer &other) {
            return p_ == other.p_ && count_ == other.count_;
        }
        bool operator!=(const pointer &other) {
            return p_ != other.p_ || count_ != other.count_;
        }
        bool compare_exchange_weak(pointer &expected, const pointer &desired) {
            LOG << LOGV(*this) << LOGV(expected) << LOGV(desired) << endl;
            return __atomic_compare_exchange_n(
               reinterpret_cast<__int128*>(this),
               reinterpret_cast<__int128*>(&expected),
               *reinterpret_cast<const __int128*>(&desired), 
               true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
            unique_lock<mutex> lock(getMutex());
//            if (p_ == expected.p_) {
//                if (count_ != expected.count_) {
//                    abort();
//                }
//            }
            if (*this == expected) {
                p_ = desired.p_;
                count_= desired.count_;
                return true;
            }
            return false;
        }
        friend ostream& operator<<(ostream& os, const pointer &p) {
            os << p.p_ << "," << p.count_;
            return os;
        }
    };
    struct Node {
        Node() = default;
        template <typename T>
        Node(T && v) : val(forward<T>(v)) {}
        V val;
        pointer next;
    };
    pointer head_;
    pointer tail_;
public:
    ConcurrentLinkedQueue() : head_(new Node()), tail_(head_) {
        LOG_DELAY << "init" << LOGV(head_) << LOGV(tail_) << endl;
    }
    ~ConcurrentLinkedQueue() {
        if (head_) {
            head_.del();
            head_.reset();
            tail_.reset();
        }
    }
    template <typename T>
    void enqueue(T && v) {
        pointer node{new Node(v)};
        pointer tail, next;
        bool ok = false;
        while(true) {
            tail = tail_;
            next = tail->next;
            
            if (tail != tail_) {
                continue;
            }
            
            if (next) {
                LOG_DELAY << "before fix tail" << LOGV(next) << LOGV(node) << endl;
                ok = tail_.compare_exchange_weak(tail, next);
                LOG_DELAY << "after fix tail" << LOGV(next) << LOGV(node) << LOGV(ok) << endl;
                continue;
            }
            LOG_DELAY << "before update tail next" << LOGV(next) << LOGV(node) << endl;
            ok = tail->next.compare_exchange_weak(next, node);
            LOG_DELAY << "after update tail next" << LOGV(next) << LOGV(node) << LOGV(ok) << endl;
            if (ok) {
                break;
            }
        }
        LOG_DELAY << "before update tail" << LOGV(tail) << LOGV(node) << endl;
        ok = tail_.compare_exchange_weak(tail, node);
        LOG_DELAY << "after update tail" << LOGV(tail) << LOGV(node) << LOGV(ok) << endl;
    }
    pair<bool, V> dequeue() {
        V v{};
        pointer head, tail, next;
        bool ok = false;
        while(true) {
            head = head_;
            tail = tail_;
            next = head->next;

            if (head != head_) {
                continue;
            }

            if (head.get() == tail.get()) { //这里不能整体比较，因为赋值的时候可能没有赋值到count
                if (!next) {
                    return {false, v};
                }
                LOG_DELAY << "before fix tail" << LOGV(tail) << LOGV(next) << endl;
                ok = tail_.compare_exchange_weak(tail, next);
                LOG_DELAY << "after fix tail" << LOGV(tail) << LOGV(next) << LOGV(ok) << endl;
                continue;
            }
            v = next->val; //next指向的更新后的head_已经是dummy节点了
            LOG_DELAY << "before update head" << LOGV(head) << LOGV(next) << endl;
            ok = head_.compare_exchange_weak(head, next);
            LOG_DELAY << "after update head" << LOGV(head) << LOGV(next) << LOGV(ok) << endl;
            if (ok) {
                break;
            }
        }
        //这里不能删除head，因为这个元素可能在被别的线程正在使用中
        //线程1是A->B->C->D，它的next为B，准备pop A
        //线程2把AB都pop了，然后把B的地址del了，线程1就无法访问了
        //所以pointer需要引用计数
        LOG_DELAY << "before del" << LOGV(head) << endl;
//        head.del();
        LOG_DELAY << "after del" << LOGV(head) << endl;
        return {true, v};
    }
};
    
const int LOOP_TIMES = 100000;
const int THREAD_NUM = 4;

std::mutex mtx_m;
queue<int> q_m;

void mutexWrite() {
    for (int i = 0; i < LOOP_TIMES; ++i) {
        std::lock_guard<std::mutex> lock(mtx_m);
        q_m.push(i);
    }
}

void mutexRead() {
    for (int i = 0; i < LOOP_TIMES; ++i) {
        while(1) {
            std::lock_guard<std::mutex> lock(mtx_m);
            if (!q_m.empty()) {
                q_m.pop();
                break;
            }
        }
    }
}

ConcurrentLinkedQueue<int> q_c;

void casWrite() {
    for (int i = 0; i < LOOP_TIMES; ++i) {
        q_c.enqueue(i);
    }
}
void casRead() {
    for (int i = 0; i < LOOP_TIMES; ++i) {
        while(1) {
            auto ok = false;
            int v = 0;
            tie(ok, v) = q_c.dequeue();
            if (ok) {
                break;
            }
        }
    }
}

void testMutex() {
    Timer t("testMutex");
    vector<thread> workers;
    for (int i = 0; i < THREAD_NUM / 2; i++) {
        workers.emplace_back(mutexWrite);
        workers.emplace_back(mutexRead);
    }
    for (auto &worker : workers) {
        worker.join();
    }
}

void testCAS() {
    Timer t("testCAS");
    vector<thread> workers;
    for (int i = 0; i < THREAD_NUM / 2; i++) {
        workers.emplace_back(casWrite);
        workers.emplace_back(casRead);
    }
    for (auto &worker : workers) {
        worker.join();
    }
}

int main() {
    Timer::setW(60);

    testMutex();
    testCAS();
}
