#pragma once
#include "/root/env/snippets/cpp/cpp_test_common.h"
#include <type_traits>

#ifdef DebugCount
static int64_t gTreeCount = 0;
#endif

template <typename KeyT, bool isRank>
class RBTreeBase{
    template <typename T>
    friend class RBTreeDebuger;
    using _KeyT = KeyT;
    static constexpr auto _isRank = isRank;
public:
    struct RankBaseNode {
        int64_t size = 1;
    };
    struct EmptyBaseNode {};
    struct Node : public std::conditional_t<isRank, RankBaseNode, EmptyBaseNode> {
        static inline Node Nil;
        static inline Node* NilPtr = &Node::Nil;
        Node() : parent(nullptr), left(nullptr), right(nullptr) {
            if constexpr(isRank) {
                this->size = 0;
            }
        }
        Node(const KeyT &key) : key(key) {}
        Node* parent = NilPtr;
        Node* left = NilPtr;
        Node* right = NilPtr;
        KeyT key;
        bool isRed = false;
    };
    RBTreeBase() = default;
    ~RBTreeBase() {
        deleteSubTree(root_);
    }
    RBTreeBase(const RBTreeBase& other) {
        copySubTree(root_, other.root_);
    }
    RBTreeBase& operator=(RBTreeBase other) {
        root_ = other.root_;
        other.root_ = Node::NilPtr;
        return *this;
    }
    Node* copySubTree(Node*& cur, const Node* other) {
        if (other == Node::NilPtr) {
            return Node::NilPtr;
        }
        cur = createNode(other->key);
        cur->isRed = other->isRed;
        cur->left = copySubTree(cur->left, other->left);
        if (cur->left) {
            cur->left->parent = cur;
        }
        cur->right = copySubTree(cur->right, other->right);
        if (cur->right) {
            cur->right->parent = cur;
        }
        return cur;
    }
    bool insert(const KeyT &key) {
        auto parent = findParent(root_, key);
        if (!parent) {
            return false;
        }
        LDEBUG(LOGVT(isNil(parent)), LOGVT(parent->key));
        auto newNode = createNode(key);
        newNode->isRed = true;
        if (parent != Node::NilPtr) {
            if (key > parent->key) {
                parent->right = newNode;
            }else {
                parent->left = newNode;
            }
            newNode->parent = parent;
        }
        while(1) {
            debugPrint<true>();
            LDEBUG(LOGVT(isNil(parent)), LOGVT(parent->key), LOGVT((isNil(newNode))), LOGVT(newNode->key));
            if (parent == Node::NilPtr) {
                root_ = newNode;
                if (root_->isRed) root_->isRed = false;
                return true;
            }
            if (!parent->isRed) {
                //2-node
                if (!parent->left->isRed && !parent->right->isRed) {
                    LDEBUG("2-node");
                    break;
                }
                //3-node
                LDEBUG("3-node-1");
                break;
            }
            if (parent->parent->left->isRed && parent->parent->right->isRed) {
                LDEBUG("4-node");
                //4-node
                parent->parent->left->isRed = false;
                parent->parent->right->isRed = false;
                parent->parent->isRed = true;
                newNode = parent->parent;
                parent = newNode->parent;
                continue;
            }
            //3-node
            LDEBUG("3-node-2");
            insert3Node(newNode); 
            break;
        }
        length_++;
        return true;
    }
    Node* find(const KeyT& key) {
        Node* cur = root_;
        while(cur != Node::NilPtr) {
#ifdef DebugCount
            gTreeCount++;
#endif
            if (key > cur->key) {
                cur = cur->right;
            }else if (key < cur->key) {
                cur = cur->left;
            }else {
                break;
            }
        }
        return cur;
    }
    bool erase(const KeyT& key) {
        auto deleteNode = find(key);
        if (deleteNode == Node::NilPtr) {
            return false;
        }
        auto deleteNextNode = findChildNext(deleteNode);
        if (deleteNextNode == deleteNode && deleteNode->left != Node::NilPtr) {
            deleteNextNode = deleteNode->left;
        }
        //update deleteNode
        deleteNode->key = deleteNextNode->key;

        if constexpr(isRank) {
            auto p = deleteNextNode->parent;
            while(p != Node::NilPtr) {
                p->size--;
                LDEBUG(LOGVT(p->key), LOGVT(p->size));
                p = p->parent;
            }
            deleteNextNode->size--;
        }

        deleteNode = deleteNextNode;
        while(1) {
            if (deleteNode == root_) {
                break;
            }
            if (deleteNode->isRed) {
                //delete 3-node, 4-node
                deleteNode->isRed = false;
                break;
            }
            if (deleteNode == deleteNode->parent->left) {
                //delete 2-node
                //left-leaning/right-leaning change
                if (deleteNode->parent->right->isRed) {
                    leftRotate(deleteNode->parent->right);
                    deleteNode->parent->parent->isRed = false;
                    deleteNode->parent->isRed = true;
                }
                //try get brother child's red node
                auto brother = deleteNode->parent->right;
                if (brother->left->isRed) {
                    rightRotate(brother->left);
                    brother = deleteNode->parent->right;
                    brother->isRed = false;
                    brother->right->isRed = true;
                }
                if (brother->right->isRed) {
                    leftRotate(brother);
                    deleteNode->parent->parent->isRed = deleteNode->parent->isRed;
                    deleteNode->parent->parent->right->isRed = false;
                    deleteNode->parent->isRed = false;
                    break;
                }
                //try get father's brother child's red node
                brother->isRed = true;
                deleteNode = deleteNode->parent;
            }else {
                //left-leaning/right-leaning change
                if (deleteNode->parent->left->isRed) {
                    rightRotate(deleteNode->parent->left);
                    deleteNode->parent->parent->isRed = false;
                    deleteNode->parent->isRed = true;
                }
                //try get brother child's red node
                auto brother = deleteNode->parent->left;
                if (brother->right->isRed) {
                    leftRotate(brother->right);
                    brother = deleteNode->parent->left;
                    brother->isRed = false;
                    brother->left->isRed = true;
                }
                if (brother->left->isRed) {
                    rightRotate(brother);
                    deleteNode->parent->parent->isRed = deleteNode->parent->isRed;
                    deleteNode->parent->parent->left->isRed = false;
                    deleteNode->parent->isRed = false;
                    break;
                }
                LDEBUG(LOGVT(isNil(brother)), LOGVT(brother->key), LOGVT((isNil(deleteNode))), LOGVT(deleteNode->key));
                //try get father's brother child's red node
                brother->isRed = true;
                deleteNode = deleteNode->parent;
            }
        }
        //update deleteNextNode->right
        if (deleteNextNode->right != Node::NilPtr) {
            deleteNextNode->right->parent = deleteNextNode->parent;
        }
        //update deleteNextNode->parent
        if (deleteNextNode->parent != Node::NilPtr) {
            if (deleteNextNode == deleteNextNode->parent->left) {
                deleteNextNode->parent->left = deleteNextNode->right;
            }else {
                deleteNextNode->parent->right = deleteNextNode->right;
            }
        }else {
            root_ = Node::NilPtr;
        }
        removeNode(deleteNextNode);
        length_--;
        return true;
    }
    Node* getByRank(int rank) {
        if (rank <= 0 || rank > length_) {
            return nullptr;
        }
        Node* cur;
        if constexpr(isRank) {
            return getByRank(root_, rank);
        }
        static_assert(isRank);
        return cur;
    }
    int64_t getRank(const KeyT& key) {
        int64_t rank = -1;
        if constexpr(isRank) {
            auto cur = find(key);
            if (cur == Node::NilPtr) {
                return rank;
            }
            rank = 0;
            do {
                rank += (cur->left->size + 1); 
                cur = findParentPrev(cur);
            }while (cur != Node::NilPtr);
            return rank;
        }
        static_assert(isRank);
        return rank;
    }
    Node* begin() {
        auto cur = root_;
        if (cur == Node::NilPtr) {
            return cur;
        }
        while(cur->left != Node::NilPtr) {
            cur = cur->left;
        }
        return cur;
    }
    Node* end() {
        return Node::NilPtr;
    }
    Node* next(Node* cur) {
        if (cur->right == Node::NilPtr) {
            while(cur != Node::NilPtr) {
                if (cur == cur->parent->left) {
                    return cur->parent;
                }
                cur = cur->parent;
            }
            return cur;
        }
        cur = cur->right;
        while (1) {
            if (cur->left == Node::NilPtr) {
                break;
            }
            cur = cur->left;
        }
        return cur;
    }
    uint64_t size() {
        return length_;
    }
    template <bool isRoot>
    void debugPrint(Node* cur = nullptr) {
    #ifndef LOG_TAG
        return;
    #endif
        if (isRoot && !cur) {
            cur = root_;
        }
        if (cur == Node::NilPtr) {
            return;
        }
        debugPrint<false>(cur->left);
        cout << LOGV(cur->key) << LOGV(cur->isRed) << 
            LOGV(isNil(cur)) << LOGV(isNil(cur->parent)) <<
            LOGV(cur->left->key) << LOGV(cur->right->key) << LOGV(cur->parent->key) << endl;
        debugPrint<false>(cur->right);
    }
private:
    Node* getByRank(Node* cur, int rank) {
        if constexpr (isRank) {
            int curRank = cur->left->size + 1;
            if (curRank == rank) {
                return cur;
            }
            if (rank < curRank) {
                return getByRank(cur->left, rank);
            }
            return getByRank(cur->right, rank - curRank);
        }
    }
    bool isNil(Node *cur) {
        return cur == Node::NilPtr;
    }
    Node* createNode(const KeyT &key) {
        auto node = new Node(key);
#ifdef CHECKMEMLEAK
        debugNodes_.insert(node);
#endif
        return node;
    }
    void removeNode(Node* node) {
#ifdef CHECKMEMLEAK
        debugNodes_.erase(node);
#endif
        delete node;
    }
    void deleteSubTree(Node* node) {
        if (node == Node::NilPtr) {
            return;
        }
        deleteSubTree(node->left);
        deleteSubTree(node->right);
        removeNode(node);
    }
    //turn curParent to be cur Node left child
    void leftRotate(Node* cur) {
        auto curLeft = cur->left;
        auto curParent = cur->parent;

        //update cur
        cur->left = curParent;
        cur->parent = curParent->parent;
        if (root_ == curParent) {
            root_ = cur;
        }
        if constexpr(isRank) {
            cur->size = curParent->size;
            LDEBUG(LOGVT(cur->key), LOGVT(cur->size));
        }

        //update cur->left
        if (curLeft != Node::NilPtr) {
            curLeft->parent = curParent;
        }

        //update curParent->parent
        if(curParent->parent != Node::NilPtr) {
            if (curParent == curParent->parent->left) {
                curParent->parent->left = cur;
            }else {
                curParent->parent->right = cur;
            }
        }
        
        //update curParent
        curParent->parent = cur;
        curParent->right = curLeft;
        if constexpr(isRank) {
            curParent->size = curParent->left->size + curParent->right->size + 1;
            LDEBUG(LOGVT(curParent->key), LOGVT(curParent->size));
        }
    }
    //turn curParent to be cur Node right child
    void rightRotate(Node* cur) {
        auto curRight = cur->right;
        auto curParent = cur->parent;

        //update cur
        cur->right = curParent;
        cur->parent = curParent->parent;
        if (root_ == curParent) {
            root_ = cur;
        }
        if constexpr(isRank) {
            cur->size = curParent->size;
            LDEBUG(LOGVT(cur->key), LOGVT(cur->size));
        }

        //update cur->right
        if (curRight != Node::NilPtr) {
            curRight->parent = curParent;
        }

        //update curParent->parent
        if (curParent->parent != Node::NilPtr) {
            if (curParent == curParent->parent->left) {
                curParent->parent->left = cur;
            }else {
                curParent->parent->right = cur;
            }
        }
        
        //update curParent
        curParent->parent = cur;
        curParent->left = curRight;
        if constexpr(isRank) {
            curParent->size = curParent->left->size + curParent->right->size + 1;
            LDEBUG(LOGVT(curParent->key), LOGVT(curParent->size));
        }
    }
    void insert3Node(Node* newNode) {
        auto parent = newNode->parent;
        if (parent == parent->parent->left) {
            if (newNode == parent->right) {
                LDEBUG("leftRotate", LOGVT(newNode->key));
                leftRotate(newNode);
                parent = newNode;
            }
            LDEBUG("rightRotate", LOGVT(parent->key));
            rightRotate(parent);
            parent->isRed = false;
            parent->right->isRed = true;
            return;
        }
        if (newNode == parent->left) {
            LDEBUG("rightRotate", LOGVT(newNode->key));
            rightRotate(newNode);
            parent = newNode;
        }
        LDEBUG("leftRotate", LOGVT(parent->key));
        leftRotate(parent);
        parent->isRed = false;
        parent->left->isRed = true;
        return;
    }
    Node* findChildNext(Node* cur) {
        if (cur->right == Node::NilPtr) {
            return cur;
        }
        cur = cur->right;
        while (1) {
            if (cur->left == Node::NilPtr) {
                break;
            }
            cur = cur->left;
        }
        return cur;
    }
    Node* findParentPrev(Node* cur) {
        while(cur != Node::NilPtr) {
            if (cur == cur->parent->right) {
                return cur->parent;
            }
            cur = cur->parent;
        }
        return cur;
    }
    Node* findParent(Node* cur, const KeyT &key) {
        if (cur == Node::NilPtr) {
            return cur;
        }
        int index = 0;
        Node* nodes[64];
        while(1) {
            if (key > cur->key) {
                if constexpr (isRank) {
                    nodes[index++] = cur;
                }
                if (cur->right == Node::NilPtr) {
                    break;
                }
                cur = cur->right;
            }else if (key < cur->key) {
                if constexpr (isRank) {
                    nodes[index++] = cur;
                }
                if (cur->left == Node::NilPtr) {
                    break;
                }
                cur = cur->left;
            }else {
                return nullptr;
            }
        }
        if constexpr(isRank) {
            for (int i = 0;i < index;i++) {
                nodes[i]->size++;
            }
        }
        return cur;
    }
    Node* root_ = Node::NilPtr;
    uint64_t length_ = 0;
};

template <typename KeyT>
using RBTree = RBTreeBase<KeyT, false>;

template <typename KeyT>
using RankRBTree = RBTreeBase<KeyT, true>;

template <typename RBTreeT>
class RBTreeDebuger {
    using KeyT = typename RBTreeT::_KeyT;
    using Node = typename RBTreeT::Node;
    static constexpr auto isRank = RBTreeT::_isRank;
public:
    RBTreeT t_;
    struct DebugNode {
        int depth = 0;
        bool isRed = false;
        bool isLeaf = false;
        bool isColorOk = true;
        KeyT key;
    };
    void insert(const KeyT &key) {
        t_.insert(key);
    }
    void debugPrint() {
        t_.template debugPrint<true>();
    }
    void debugCheck(vector<DebugNode>& vs, int depth, Node* cur) {
        if (!cur) {
            return;
        }
        DebugNode node;
        if (cur->isRed) {
            if (cur->left->isRed || cur->right->isRed) {
                node.isColorOk = false;
            }
        } else{
            depth = depth + 1;
            node.depth = depth;
        }
        if (!cur->left && !cur->right) node.isLeaf = true;
        node.key = cur->key;
        node.isRed = cur->isRed;
        if constexpr(isRank) {
            if (!node.isLeaf) {
                LDEBUG(LOGVT(node.key), LOGVT(cur->size));
            }
        }
        debugCheck(vs, depth, cur->left);
        vs.push_back(node);
        debugCheck(vs, depth, cur->right);
    }
    int debugCheck(const set<KeyT>& expectResult) {
        vector<DebugNode> vs;
        debugCheck(vs, 0, t_.root_);
        int lastDepth = 0;
        KeyT lastValue = KeyT{};
        bool ok = true;
        set<KeyT> s; //check duplicate
        vector<KeyT> checkNext; //check next() is correct
        int64_t rank = 1;
        for (auto &v : vs) {
            if (!v.isLeaf) {
                if (s.count(v.key)) {
                    cout << "check duplicate failed" << LOGV(v.key) << endl;
                    ok = false;
                    break;
                }
                s.insert(v.key);
                checkNext.push_back(v.key);
            }
            if (!v.isColorOk) {
                cout << "check color failed" << LOGV(v.key) << 
                    LOGV(v.isColorOk) << endl;
                ok = false;
                break;
            }
            if (v.isLeaf) {
                if (lastDepth != 0 && v.depth != lastDepth) {
                    cout << "check depth failed" << LOGV(v.key) << 
                        LOGV(v.depth) << LOGV(lastDepth) << endl;
                    ok = false;
                    break;
                }
                lastDepth = v.depth;
            }
            if (lastValue != KeyT{}) {
                if (!v.isLeaf && v.key < lastValue) {
                    cout << "check value failed" << LOGV(v.key) << 
                        LOGV(v.depth) << LOGV(lastDepth) << endl;
                    ok = false;
                    break;
                }
            }
            if constexpr(isRank) {
                if (!v.isLeaf) {
                    if (rank != t_.getRank(v.key)) {
                        cout << "check getRank failed" << LOGV(v.key) << 
                            LOGV(rank) << LOGV(t_.getRank(v.key)) << endl;
                        ok = false;
                        break;
                    }
                    if (v.key != t_.getByRank(rank)->key) {
                        cout << "check getByRank failed" << LOGV(v.key) << 
                            LOGV(rank) << LOGV(t_.getByRank(rank)->key) << endl;
                        ok = false;
                        break;
                    }
                    LDEBUG(LOGVT(v.key), LOGVT(rank), LOGVT(getRank(v.key)));
                    rank++;
                }
            }
            lastValue = v.key;
        }
        if (s != expectResult) {
            cout << "check expectResult failed" << 
                LOGV(s.size()) <<
                LOGV(expectResult.size()) << endl;
            ok = false;
            return ok;
        }
        int index = 0;
        for(auto node = t_.begin();node != t_.end();node = t_.next(node), index++) {
            if (node->key != checkNext[index]) {
                cout << "check next failed" << LOGV(node->key) << 
                    LOGV(index) << LOGV(checkNext[index]) << endl;
                ok = false;
                break;
            }
        }
        return ok;
    }
#ifdef DebugCount
    int64_t getDebugCount() {
        return gTreeCount;
    }
#endif
#ifdef CHECKMEMLEAK
    set<Node*> debugNodes_;
#endif
};
