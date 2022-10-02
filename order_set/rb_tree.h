#pragma once
#include "/root/env/snippets/cpp/cpp_test_common.h"

#ifdef DebugCount
static int64_t treeCount = 0;
#endif

class Tree{
    using KeyT = int;
public:
    struct Node{
        static Node Nil;
        static Node* NilPtr;
        Node() : parent(nullptr), left(nullptr), right(nullptr) {}
        Node(const KeyT &key) : key(key) {}
        Node* parent = NilPtr;
        Node* left = NilPtr;
        Node* right = NilPtr;
        KeyT key;
        bool isRed = false;
    };
#define p2i(node) int64_t(node - Node::NilPtr)
    Tree() = default;
    ~Tree() {
        deleteSubTree(root_);
    }
    Tree(const Tree& other) {
        copySubTree(root_, other.root_);
    }
    Tree& operator=(Tree other) {
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
        LDEBUG(LOGVT(p2i(parent)), LOGVT(parent->key));
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
            LDEBUG(LOGVT(p2i(parent)), LOGVT(parent->key), LOGVT((p2i(newNode))), LOGVT(newNode->key));
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
        return true;
    }
    Node* find(const KeyT& key) {
        Node* cur = root_;
        while(cur != Node::NilPtr) {
#ifdef DebugCount
            treeCount++;
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
        auto deleteNextNode = findNext(deleteNode);
        if (deleteNextNode == deleteNode && deleteNode->left != Node::NilPtr) {
            deleteNextNode = deleteNode->left;
        }
        //update deleteNode
        deleteNode->key = deleteNextNode->key;

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
                LDEBUG(LOGVT(p2i(brother)), LOGVT(brother->key), LOGVT((p2i(deleteNode))), LOGVT(deleteNode->key));
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
        return true;
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
            LOGV(p2i(cur)) << LOGV(p2i(cur->parent)) <<
            LOGV(cur->left->key) << LOGV(cur->right->key) << LOGV(cur->parent->key) << endl;
        debugPrint<false>(cur->right);
    }
    struct DebugNode {
        int depth = 0;
        bool isRed = false;
        bool isLeaf = false;
        bool isColorOk = true;
        KeyT key;
    };
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
        debugCheck(vs, depth, cur->left);
        vs.push_back(node);
        debugCheck(vs, depth, cur->right);
    }
    int debugCheck() {
        vector<DebugNode> vs;
        debugCheck(vs, 0, root_);
        int lastDepth = 0;
        int lastValue = 0;
        bool ok = true;
        set<int> s;
        for (auto &v : vs) {
            if (!v.isLeaf) {
                if (s.count(v.key)) {
                    cout << LOGV("check duplicate failed") << LOGV(v.key) << endl;
                    ok = false;
                    break;
                }
                s.insert(v.key);
            }
            if (!v.isColorOk) {
                cout << LOGV("check color failed") << LOGV(v.key) << 
                    LOGV(v.isColorOk) << endl;
                ok = false;
                break;
            }
            if (v.isLeaf) {
                if (lastDepth != 0 && v.depth != lastDepth) {
                    cout << LOGV("check depth failed") << LOGV(v.key) << 
                        LOGV(v.depth) << LOGV(lastDepth) << endl;
                    ok = false;
                    break;
                }
                lastDepth = v.depth;
            }
            if (lastValue != 0) {
                if (!v.isLeaf && v.key < lastValue) {
                    cout << LOGV("check value failed") << LOGV(v.key) << 
                        LOGV(v.depth) << LOGV(lastDepth) << endl;
                    ok = false;
                    break;
                }
            }
            lastValue = v.key;
        }
        return ok;
    }
#ifdef CHECKMEMLEAK
    set<Node*> debugNodes_;
#endif
private:
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
    void leftRotate(Node* cur) {
        auto curLeft = cur->left;
        auto curParent = cur->parent;

        //update cur
        cur->left = curParent;
        cur->parent = curParent->parent;
        if (root_ == curParent) {
            root_ = cur;
        }

        //update cur->right
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
    }
    void rightRotate(Node* cur) {
        auto curRight = cur->right;
        auto curParent = cur->parent;

        //update cur
        cur->right = curParent;
        cur->parent = curParent->parent;
        if (root_ == curParent) {
            root_ = cur;
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
    Node* findNext(Node* cur) {
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
    Node* findParent(Node* cur, const KeyT &key) {
        if (cur == Node::NilPtr) {
            return cur;
        }
        while(1) {
            if (key > cur->key) {
                if (cur->right == Node::NilPtr) {
                    break;
                }
                cur = cur->right;
            }else if (key < cur->key) {
                if (cur->left == Node::NilPtr) {
                    break;
                }
                cur = cur->left;
            }else {
                return nullptr;
            }
        }
        return cur;
    }
    Node* root_ = Node::NilPtr;
};

Tree::Node Tree::Node::Nil;
Tree::Node* Tree::Node::NilPtr = &Tree::Node::Nil;
