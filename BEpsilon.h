#ifndef BEPSILON_BEPSILON_H
#define BEPSILON_BEPSILON_H

#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include "swap_space.hpp"
#include "backing_store.hpp"

#include <assert.h>
#include <algorithm>

using namespace std;


class NoSuchKeyException : public exception {
    virtual const char *what() const throw() {
        return "No such key element.";
    }
};

class InvalidKeyRange : public exception {
    virtual const char *what() const throw() {
        return "Invalid key range.";
    }
};

template<typename Key, typename Value, int B>
class BEpsilonTree {
public:
    typedef enum {
        INSERT,
        REMOVE,
        UPDATE,
        ANY
    } Opcode;

    typedef enum {
        LEFT,
        RIGHT
    } Direction;

    class Message : public serializable {
    public:
        Message(){}
        Message(Opcode opcode, Key key, Value value) : opcode(opcode), key(key), value(value) {};

        bool operator<(const Message &other) {//for std::sort method
            return this->key > other.key;
        }

        bool operator==(const Message &other) {
            return this->key == other.key;
        }


        void _serialize(std::iostream &fs, serialization_context &context) {
            fs << "message{" << endl;
            fs << "opcode:" << std::endl;
            fs << opcode << std::endl;
            fs << "key:" << std::endl;
            serialize(fs, context, key);
            fs << endl;
            fs << "value:" << std::endl;
            serialize(fs, context, value);
            fs << endl;
            fs << "}" << endl;
        }

        void _deserialize(std::iostream &fs, serialization_context &context) {
            cout << "deserializing message" << endl;
            std::string dummy;
            int op_int;
            fs >> dummy;
            fs >> dummy;
            fs >> op_int;
            opcode = (Opcode) op_int;
            fs >> dummy;
            deserialize(fs, context, key);
            fs >> dummy;
            deserialize(fs, context, value);
            fs >> dummy;
            cout << "done." << endl;
        }
        //TODO add size function that calc the exact message size, when the Key/Value size isn't const.
    private:
        Opcode opcode;
        Key key;
        Value value;

        friend class BEpsilonTree;
    };

    class Node;

    // We let a swap_space handle all the I/O.
    typedef typename swap_space::pointer<Node> NodePointer;


    typedef typename vector<BEpsilonTree<Key, Value, B>::Message>::iterator MessageIterator;
    typedef typename vector<NodePointer>::iterator ChildIterator;

    BEpsilonTree(swap_space *sspace) : ss(sspace), size_(0) {
        root = NodePointer();
    }

    void insert(Key key, Value value);

    void remove(Key key);

    void printTree();

    bool contains(Key key);

    bool pointQuery(Key key, Value& value);

    int size();

    class Node : public serializable {
    public:
        static constexpr int BLOCK_SIZE = 1000;
        static constexpr double EPSILON = 0.1;
        static constexpr int BUFFER_SIZE = BLOCK_SIZE * EPSILON;
        static constexpr int MESSAGE_SIZE = sizeof(Message);
        static constexpr int MAX_NUMBER_OF_MESSAGE_PER_NODE = BUFFER_SIZE / MESSAGE_SIZE;

        Node(bool isLeaf = true, NodePointer parent = NodePointer(), NodePointer right_sibling = NodePointer(),
             NodePointer left_sibling = NodePointer());

        static inline void updateMinSubTreeKey(NodePointer node);

        void inOrder(int indent = 0);

        Key minSubTreeKeyTest();

        void bPlusValidation();

        void RI();

        void _serialize(std::iostream &fs, serialization_context &context) {
            fs << "isLeaf:" << std::endl;
            fs << isLeaf << std::endl;
            fs << "parent:" << std::endl;
            serialize(fs, context, parent);
            fs << "right_sibling:" << std::endl;
            serialize(fs, context, right_sibling);
            fs << "left_sibling:" << std::endl;
            serialize(fs, context, left_sibling);
            fs << "sub_tree_min_key:" << std::endl;
            fs << sub_tree_min_key << std::endl;
            fs << "keys:" << std::endl;
            serialize(fs, context, keys);
            fs << "values:" << std::endl;
            serialize(fs, context, values);
            fs << "children:" << std::endl;
            serialize(fs, context, children);
            fs << "messages:" << std::endl;
            serialize(fs, context, message_buff);
        }

        void _deserialize(std::iostream &fs, serialization_context &context) {
            std::string dummy;
            fs >> dummy;
            fs >> isLeaf;
            fs >> dummy;
            deserialize(fs, context, parent);
            fs >> dummy;
            deserialize(fs, context, right_sibling);
            fs >> dummy;
            deserialize(fs, context, left_sibling);
            fs >> dummy;
            fs >> sub_tree_min_key;
            fs >> dummy;
            deserialize(fs, context, keys);
            fs >> dummy;
            deserialize(fs, context, values);
            fs >> dummy;
            deserialize(fs, context, children);
            fs >> dummy;
            deserialize(fs, context, message_buff);
        }


    private:
        bool isLeaf;
        NodePointer parent;
        vector <Key> keys;
        NodePointer right_sibling;
        NodePointer left_sibling;
        Key sub_tree_min_key;

        //if this node is a leaf
        //values.size() == keys.size(),for now it's equal to 1, keys.size.max == B-1;
        vector <Value> values;

        //if the node is internal
        //children.size() == keys.size()+1;
        vector <NodePointer> children;

        //balanced message_buff for O(log(# of messages in the buffer)) insertion/deletion/query.
        vector <Message> message_buff;

        friend class BEpsilonTree;
    };


    swap_space *ss;
    NodePointer root;
    int size_;
    Key default_key_;

private:
    /**
Check if node is is full, node is full when it has B children.

@return true if node is full, false else.
*/
    bool isFull(NodePointer p);

    //A utility function to make sure that all the leaf is on the same height.
    //should call after the key-value insertion from the leaf.
    void insertKeysUpdate(NodePointer p);

    // A utility function to insert a new key in the subtree rooted with
    // this node.
    bool insert(NodePointer p, Key key, Value value);

    //A function to check if the node(leaf/internal is full or not)
    //is full(the number of key smaller than the minimum).
    bool isNotLegal(NodePointer p);

    //A function to find the index of this node in the his parent children vector
    //the assumption is this->parent != NULL.
    int getOrder(NodePointer p);

    bool pointQuery(NodePointer p, Key key, Value& value);

    // A function that returns the index of the key in the parent that that point to this
    // the assumption is that parent != NULL and this node have some keys.
    //unused for now.
    int getKeyOrder(NodePointer p);

    // A utility function to split the child of this node. ix is index
    // of child in child vector. The Child must be full when this
    // function is called
    void splitChild(NodePointer p, int ix, NodePointer child);

    bool isSiblingBorrowable(NodePointer p, Direction direction);

    void mergeKeysUpdate(NodePointer p, int child_ix);

    //A utility function to make sure that all the leaf is on the same height.
    //should call after the key-value remove from the leaf.
    void balance(NodePointer p, NodePointer child);

    // A utility function to remove a key in the subtree rooted with
    // this node.
    //the tree will not affected if the key isn't existing.
    bool remove(NodePointer p, Key key);

    bool insertMessage(NodePointer p, Opcode opcode, Key key, Value value = Value());

    bool isMessagesBufferFull(NodePointer p);

    void bufferFlushIfFull(NodePointer p);

    bool tryBorrowFromLeft(NodePointer p);

    bool tryBorrowFromRight(NodePointer p);

    bool tryMergeWithLeft(NodePointer p);

    bool tryMergeWithRight(NodePointer p);

    void updateParentKeys(NodePointer p);

};

template<typename Key, typename Value, int B>
BEpsilonTree<Key, Value, B>::Node::Node(bool isLeaf, NodePointer parent, NodePointer right_sibling,
                                        NodePointer left_sibling) {
    this->parent = parent;
    this->right_sibling = right_sibling;
    this->left_sibling = left_sibling;
    this->isLeaf = isLeaf;
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isFull(NodePointer p) {
    //choose the max number of key and values in each node according to the block size.
    return p->keys.size() >= B;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isNotLegal(NodePointer p) {
    return p->keys.size() < B / 2;
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::getKeyOrder(NodePointer p) {
    //for sure this node isn't root and full, we check it before this function call.
    int ix = 0;
    while (ix < p->parent->keys.size() && p->parent->keys[ix] <= p->keys[0]) {
        ix++;
    }
    return ix;
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::getOrder(NodePointer p) {
    int ix = 0;
    //for sure this node isn't root and full, we check it before this function call.
    typedef typename vector<NodePointer>::iterator iterator;
    for (iterator it = p->parent->children.begin(); it != p->parent->children.end(); it++) {
        if ((*it) == p) {
            return ix;
        } else {
            ix++;
        }
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::splitChild(NodePointer p, int ix, NodePointer left_child) {

    //this node is the parent of right and left child.
    // Create a new node which is going to store (child->keys.size()-1) keys of child
    NodePointer right_child = ss->allocate(new BEpsilonTree<Key, Value, B>::Node(left_child->isLeaf,
                                                                                 left_child->parent,
                                                                                 left_child->right_sibling,
                                                                                 left_child->left_sibling));

    //update the sibling of both child and new node, add the new node between the child and the new node.
    //if the nods is internal and not a leaf, the sibling will be NULL.
    if (!left_child->right_sibling.isNull()) {
        left_child->right_sibling->left_sibling = right_child;
    }
    right_child->left_sibling = left_child;
    right_child->right_sibling = left_child->right_sibling;
    left_child->right_sibling = right_child;
    //left_child->left_sibling and left_child->right_sibling->right_sibling doesn't change.

    //update to move the minimum number of children for each node, and not 1.
    //B should be grater than 2, else infinite loop will occur.
    int middle_ix = B / 2;
    Key right_child_min_key;

    if (left_child->isLeaf) {
        right_child->keys.insert(right_child->keys.begin(),
                                 left_child->keys.begin() + middle_ix,
                                 left_child->keys.end());

        left_child->keys.erase(left_child->keys.begin() + middle_ix,
                               left_child->keys.end());

        right_child->values.insert(right_child->values.begin(),
                                   left_child->values.begin() + middle_ix,
                                   left_child->values.end());

        left_child->values.erase(left_child->values.begin() + middle_ix,
                                 left_child->values.end());

        p->keys.insert(p->keys.begin() + ix, right_child->keys[0]);
        right_child_min_key = right_child->keys[0];
        right_child->sub_tree_min_key = right_child->keys[0];
    } else {

        //find the appropriate key and its index.
        Key key = left_child->keys[middle_ix];
        right_child_min_key = key;
        int key_ix = 0;
        while ((key_ix >= 0) && (key_ix < p->keys.size()) && (p->keys[key_ix] < key)) {
            key_ix++;
        }

        p->keys.insert(p->keys.begin() + key_ix, key);

        right_child->keys.insert(right_child->keys.begin(),
                                 left_child->keys.begin() + (middle_ix + 1),
                                 left_child->keys.end());

        left_child->keys.erase(left_child->keys.begin() + (middle_ix),
                               left_child->keys.end());

        right_child->children.insert(right_child->children.begin(),
                                     left_child->children.begin() + middle_ix + 1,
                                     left_child->children.end());

        // w/o this, compilation error, with nested template in dependent scope.
        typedef typename vector<NodePointer>::iterator iterator;
        for (iterator it = left_child->children.begin() + middle_ix + 1; it != left_child->children.end(); it++) {
            (*it)->parent = right_child;
        }

        left_child->children.erase(left_child->children.begin() + middle_ix + 1,
                                   left_child->children.end());

        right_child->sub_tree_min_key = right_child->children[0]->sub_tree_min_key;
    }

    MessageIterator first_message_it = left_child->message_buff.begin();
    while (first_message_it != left_child->message_buff.end() && first_message_it->key <= right_child_min_key) {
        first_message_it++;
    }
    right_child->message_buff.insert(right_child->message_buff.begin(),
                                     first_message_it,
                                     left_child->message_buff.end());
    left_child->message_buff.erase(first_message_it, left_child->message_buff.end());
    //set the new node as a child of this node
    p->children.insert(p->children.begin() + (ix + 1), right_child);

};


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::insertKeysUpdate(NodePointer p) {
    if (isFull(p)) {
        if (p->parent.isNull()) {//this is root :)
            NodePointer node = ss->allocate(new Node(false));
            p->parent = node;
            node->children.insert(node->children.begin(), p);
            splitChild(node, 0, p);
            node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
        } else {
            splitChild(p->parent, getOrder(p), p);
            p->parent->sub_tree_min_key = p->parent->children[0]->sub_tree_min_key;
            insertKeysUpdate(p->parent);
        }
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isSiblingBorrowable(NodePointer p, Direction direction) {
    if (p->parent.isNull()) {
        return false;
    }

    NodePointer sibling = direction == RIGHT ? p->right_sibling : p->left_sibling;
    return !sibling.isNull() && sibling->parent == p->parent && sibling->keys.size() > B / 2;
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::mergeKeysUpdate(NodePointer p, int child_ix) {
    if (child_ix > 0) {
        p->keys.erase(p->keys.begin() + (child_ix - 1), p->keys.begin() + (child_ix - 1));
        p->children.erase(p->children.begin() + child_ix, p->children.begin() + child_ix);
    } else if (!p->parent.isNull()) {
        int parent_ix = getOrder(p);
        p->parent->keys[parent_ix > 0 ? parent_ix - 1 : 0] = p->keys[0];
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::tryBorrowFromLeft(NodePointer p) {
    if (isSiblingBorrowable(p, LEFT)) {

        if (p->isLeaf) {
            p->values.insert(p->values.begin(),
                             p->left_sibling->values.end() - 1,
                             p->left_sibling->values.end());
            p->left_sibling->values.pop_back();
            p->keys.insert(p->keys.begin(), p->left_sibling->keys[p->left_sibling->keys.size() - 1]);
        } else {
            p->keys.insert(p->keys.begin(), p->children[0]->sub_tree_min_key);
            p->children.insert(p->children.begin(),
                               p->left_sibling->children.end() - 1,
                               p->left_sibling->children.end());
            p->left_sibling->children[p->left_sibling->children.size() - 1]->parent = p;
            p->left_sibling->children.pop_back();
        }

        Key key = p->isLeaf ?
                  p->left_sibling->keys[p->left_sibling->keys.size() - 1] : p->children[0]->sub_tree_min_key;
        MessageIterator l_it = p->left_sibling->message_buff.begin();
        while (l_it->key < key) {
            l_it++;
        }
        p->message_buff.insert(p->message_buff.begin(), l_it, p->left_sibling->message_buff.end());
        p->left_sibling->message_buff.erase(l_it, p->left_sibling->message_buff.end());
        p->left_sibling->keys.pop_back();
        p->updateMinSubTreeKey(p);
        updateParentKeys(p->left_sibling);
        updateParentKeys(p);
        bufferFlushIfFull(p);
        return true;
    }

    return false;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::tryBorrowFromRight(NodePointer p) {
    if (isSiblingBorrowable(p, RIGHT)) {

        if (p->isLeaf) {
            p->values.insert(p->values.end(), p->right_sibling->values[0]);
            p->right_sibling->values.erase(p->right_sibling->values.begin());
            p->keys.insert(p->keys.end(), p->right_sibling->keys[0]);

        } else {
            p->keys.insert(p->keys.end(), p->right_sibling->children[0]->sub_tree_min_key);
            p->children.insert(p->children.end(), p->right_sibling->children[0]);
            p->right_sibling->children[0]->parent = p;
            p->right_sibling->children.erase(p->right_sibling->children.begin());
        }

        Key key = p->isLeaf ?
                  p->right_sibling->keys[0] : p->right_sibling->children[0]->sub_tree_min_key;
        MessageIterator r_it = p->right_sibling->message_buff.begin();
        while (r_it->key < key) {
            r_it++;
        }
        p->message_buff.insert(p->message_buff.end(), p->right_sibling->message_buff.begin(), r_it);
        p->right_sibling->message_buff.erase(p->right_sibling->message_buff.begin(), r_it);

        p->right_sibling->keys.erase(p->right_sibling->keys.begin());
        p->updateMinSubTreeKey(p);
        p->updateMinSubTreeKey(p->right_sibling);
        updateParentKeys(p->right_sibling);
        updateParentKeys(p);
        return true;
    }

    return false;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::tryMergeWithLeft(NodePointer p) {
    if (p->left_sibling.isNull() || (p->left_sibling->parent != p->parent)) {
        return false;
    }

    if (p->isLeaf) {
        p->left_sibling->values.insert(p->left_sibling->values.end(), p->values.begin(), p->values.end());
        p->left_sibling->keys.insert(p->left_sibling->keys.end(), p->keys.begin(), p->keys.end());
        p->values.erase(p->values.begin(), p->values.end());

    } else {
        bool first_time = true;
        while (p->children.size() > 0) {
            if (first_time) {
                p->left_sibling->keys.insert(p->left_sibling->keys.end(), p->children[0]->sub_tree_min_key);
                first_time = false;
            } else {
                p->left_sibling->keys.insert(p->left_sibling->keys.end(), p->keys[0]);
                p->keys.erase(p->keys.begin());
            }

            p->left_sibling->children.push_back(p->children[0]);
            p->children[0]->parent = p->left_sibling;
            p->children.erase(p->children.begin());
        }
    }

    p->left_sibling->message_buff.insert(p->left_sibling->message_buff.end(),
                                         p->message_buff.begin(),
                                         p->message_buff.end()
    );
    p->message_buff.erase(p->message_buff.begin(),
                          p->message_buff.end()
    );
    p->keys.erase(p->keys.begin(), p->keys.end());
    bufferFlushIfFull(p->left_sibling);
    return true;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::tryMergeWithRight(NodePointer p) {
    if (p->right_sibling.isNull() || (p->right_sibling->parent != p->parent)) {
        return false;
    }

    if (p->isLeaf) {
        p->right_sibling->values.insert(p->right_sibling->values.begin(), p->values.begin(), p->values.end());
        p->values.erase(p->values.begin(), p->values.end());
        p->right_sibling->keys.insert(p->right_sibling->keys.begin(), p->keys.begin(), p->keys.end());
    } else {
        bool first_time = true;
        while (p->children.size() > 0) {
            int last_index = p->children.size() - 1;
            if (first_time) {
                p->right_sibling->keys.insert(p->right_sibling->keys.begin(),
                                              p->right_sibling->children[0]->sub_tree_min_key);
                first_time = false;
            } else {
                p->right_sibling->keys.insert(p->right_sibling->keys.begin(), p->keys[p->keys.size() - 1]);
                p->keys.pop_back();
            }
            p->right_sibling->children.insert(p->right_sibling->children.begin(), p->children[last_index]);
            p->children[last_index]->parent = p->right_sibling;
            p->children.pop_back();
        }
    }
    p->right_sibling->message_buff.insert(p->right_sibling->message_buff.begin(),
                                          p->message_buff.begin(),
                                          p->message_buff.end());
    p->message_buff.erase(p->message_buff.begin(),
                          p->message_buff.end()
    );
    p->keys.erase(p->keys.begin(), p->keys.end());
    p->updateMinSubTreeKey(p->right_sibling);
    if (!p->left_sibling.isNull()) {
        bufferFlushIfFull(p->left_sibling);
    }
    return true;
}

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::updateParentKeys(NodePointer p) {
    //we shouldn't move keys from node to other,
    // but we yes should update the parent key to have the minimum key in this node in the case of minimum key remove
    if (!p->parent.isNull() && p->parent->keys.size() > 0) {
        int my_index = getOrder(p);
        int update_idx = (my_index > 0) ? my_index - 1 : 0;

        if (p->keys.size() == 0) {
            p->parent->children.erase(p->parent->children.begin() + my_index);
            p->parent->keys.erase(p->parent->keys.begin() + update_idx);
        } else {
            if (my_index > update_idx) {
                p->parent->keys[update_idx] = p->sub_tree_min_key;
            }
        }
        p->updateMinSubTreeKey(p->parent);
    }
};


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::balance(NodePointer p, NodePointer child) {
    if (!child.isNull() && child->keys.size() == 0) {
        updateParentKeys(child);
        if (!child->left_sibling.isNull()) {
            child->left_sibling->right_sibling = child->right_sibling;
        }
        if (!child->right_sibling.isNull()) {
            child->right_sibling->left_sibling = child->left_sibling;
        }
        child = NodePointer();
    }

    if (isNotLegal(p)) {
        if (!tryBorrowFromLeft(p)) {
            if (!tryBorrowFromRight(p)) {
                if (!tryMergeWithLeft(p)) {
                    tryMergeWithRight(p);
                }
            }
        }
    } else {
        p->updateMinSubTreeKey(p);
        if (!p->parent.isNull()) {
            updateParentKeys(p);
        }
    }

    if (!p->parent.isNull()) {
        balance(p->parent, p);
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::updateMinSubTreeKey(NodePointer node) {
    if (node->keys.size() == 0) return;
    if (node->isLeaf) {
        node->sub_tree_min_key = node->keys[0];
    } else {
        node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
    }
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::insert(NodePointer p, Key key, Value value) {
    return insertMessage(p,INSERT, key, value);
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::remove(NodePointer p, Key key) {
    return insertMessage(p,REMOVE, key);
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::inOrder(int indent) {
    if (!isLeaf) {
        this->children[this->children.size() - 1]->inOrder(indent + 4);
    }
    for (int i = this->keys.size() - 1; i >= 0; i--) {
        if (indent > 0) {
            cout << setw(indent) << ' ';
        }
        cout << setw(indent) << this->keys[i] << endl;
        if (!isLeaf) {
            this->children[i]->inOrder(indent + 4);
        }
    }
    cout << setw(indent) << ' ';
    cout << setw(indent) << "----" << endl;
}

template<typename Key, typename Value, int B>
Key BEpsilonTree<Key, Value, B>::Node::minSubTreeKeyTest() {
    Key min = this->sub_tree_min_key;
    if (!isLeaf) {
        Key minChildrenKey = this->children[0]->minSubTreeKeyTest();

        for (NodePointer child: children) {
            Key min_key = child->minSubTreeKeyTest();
            if (min_key <= minChildrenKey) {
                minChildrenKey = min_key;
            }
        }
        assert(this->sub_tree_min_key == minChildrenKey);
    } else {
        assert(min == this->keys[0]);
    }
    return min;
}

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::bPlusValidation() {
    //root can have less than B/2 keys.
    assert((this->parent.isNull()) || (this->keys.size() < B && this->keys.size() >= B / 2));
    assert(std::is_sorted(this->keys.begin(), this->keys.end()));
    if (isLeaf) {
        assert(this->keys.size() == this->values.size());
    } else {
        assert(this->keys.size() + 1 == this->children.size());
        for (NodePointer node : this->children) {
            node->bPlusValidation();
        }
    }

};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::RI() {
    minSubTreeKeyTest();
    bPlusValidation();
}

/*
 * the API B+ function
 * insert: A function for insertion to the tree
 * rangeQuery:
 * Query:
 * remove: A function to remove a key from the tree.s
 * */

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::insert(Key key, Value value) {
    if (root.isNull()) { // if the Tree is empty
        root = ss->allocate(new Node(true));
    }
    if (insert(root, key, value)) {
        size_++;
    }
    if (!root->parent.isNull()) {
        root = root->parent;
    }

};


template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::insertMessage(NodePointer p, Opcode opcode, Key key, Value value) {
    Message message(opcode, key, value);
    //ix will contains the appropriate index in the message.key in the message buffer.
    int ix = 0;
    for (; ix < p->message_buff.size() && p->message_buff[ix].key < key; ix++) {}

    if (ix < p->message_buff.size() && p->message_buff[ix].key == key) {
        if (message.opcode == INSERT && p->message_buff[ix].opcode == REMOVE) {
            p->message_buff[ix] = message;
        } else if (message.opcode == REMOVE && p->message_buff[ix].opcode == INSERT) {
            p->message_buff.erase(p->message_buff.begin() + ix);
        }
    } else {
        p->message_buff.insert(p->message_buff.begin() + ix, message);
    }
    //ask after the insert if there a need to split the message buffer.
    bufferFlushIfFull(p);
    //ask after the insert if there a need to split the message buffer.
    if (!p->keys.empty() && p->keys[0] < p->sub_tree_min_key) {
        p->sub_tree_min_key = p->keys[0];
    }
    return true;
}

/*
 * when the buffer got empty, we need to flush the message into the key, value buffers,
 * and then handle the node separate.*/
template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::bufferFlushIfFull(NodePointer p) {
    if (isMessagesBufferFull(p) == false) return;
    vector <MessageIterator> tmp;
    if (p->isLeaf) { //i.e. leaf node.. so apply the messages.
        for (MessageIterator m = p->message_buff.begin(); m != p->message_buff.end(); m++) {
            if (m->opcode == REMOVE) {
                int ix;
                for (ix = 0; ix < p->keys.size() && p->keys[ix] < m->key; ix++) {}
                if (ix < p->keys.size() && p->keys[ix] == m->key) {
                    if (p->keys[ix] == m->key) {
                        p->keys.erase(p->keys.begin() + ix);
                        p->values.erase(p->values.begin() + ix);
                    }
                }
                tmp.push_back(m);
            }
        }
        for (MessageIterator m : tmp) {
            p->message_buff.erase(m);
        }
        int num_of_applied_message = 0;
        for (Message m : p->message_buff) {
            if (isFull(p)) break;
            int ix;
            for (ix = 0; ix < p->keys.size() && p->keys[ix] < m.key; ix++) {}
            if (m.opcode == INSERT) {
                p->keys.insert(p->keys.begin() + ix, m.key);
                p->values.insert(p->values.begin() + ix, m.value);
                num_of_applied_message++;
            } else {
                assert("unexpected opcode!!!");
            }
        }
        p->message_buff.erase(p->message_buff.begin(), p->message_buff.begin() + num_of_applied_message);
        p->sub_tree_min_key = p->keys[0];
        insertKeysUpdate(p);
        balance(p, NodePointer());
    } else {
        MessageIterator message_it = p->message_buff.begin();
        ChildIterator child_it = p->children.begin();
        for (int key_ix = 0; key_ix < p->keys.size(); key_ix++) {
            while ((message_it != p->message_buff.end()) && (message_it->key < p->keys[key_ix])) {
                (*child_it)->message_buff.push_back(*message_it);
                message_it++;
            }
            std::sort((*child_it)->message_buff.begin(),
                      (*child_it)->message_buff.end(),
                      [](Message a, Message b) { return a.key < b.key; });
            child_it++;
        }
        //here
        if (message_it != p->message_buff.end()) {
            (*child_it)->message_buff.insert((*child_it)->message_buff.end(), message_it, p->message_buff.end());
            std::sort((*child_it)->message_buff.begin(),
                      (*child_it)->message_buff.end(),
                      [](Message a, Message b) { return a.key < b.key; });
        }
        p->message_buff.erase(p->message_buff.begin(), p->message_buff.end());
        NodePointer child = p->children.front();
        NodePointer right_child;
        while (!child.isNull()) {
            right_child = child->right_sibling;
            bufferFlushIfFull(child);
            child = right_child;
        }
    }
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isMessagesBufferFull(NodePointer p) {
    return p->message_buff.size() >= Node::MAX_NUMBER_OF_MESSAGE_PER_NODE;
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::remove(Key key) {
    if (!root.isNull()) {
        if (remove(root, key)) {
            size_--;
        }
        if (root->children.size() == 1) {
            root = root->children[0];
            root->parent = NodePointer();
        }
    }
    if (!root->parent.isNull()) {
        root = root->parent;
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::printTree() {
    if (!root.isNull()) {
        root->inOrder();
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::pointQuery(NodePointer p, Key key, Value& value) {
    Message m(ANY, key, Value());
    MessageIterator message_it = std::find(p->message_buff.begin(), p->message_buff.end(), m);
    if(message_it != p->message_buff.end()) { // the key is appear in
        switch(message_it->opcode) {
            case REMOVE : return false;
            case INSERT : value = message_it->value; return true;
            default: assert("no such opcode");
        }
    } else if (p->isLeaf) {
        typename vector<Key>::iterator key_it = p->keys.begin();
        int ix = 0;
        for(;*key_it < key && key_it != p->keys.end(); key_it++, ix++) {}
        if(key_it != p->keys.end()) {
            value = p->values[ix];
            return true;
        }
    } else {
        ChildIterator child_it = p->children.begin();
        typename vector<Key>::iterator key_it = p->keys.begin();
        while((*key_it) <= key && key_it != p->keys.end()) {
            child_it++;
            key_it++;
        }
        NodePointer child = child_it != p->children.end() ? (*child_it) : p->children.back();
        return pointQuery(child, key, value);
    }
    return false;
}


template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::pointQuery(Key key, Value& value) {
    if(!root.isNull()) {
        return pointQuery(root, key, value);
    }
    return false;
};


template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::contains(Key key) {
    Value value;
    return pointQuery(key, value);
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::size() {
    return size_;
};

//private o

#endif //BEPSILON_BEPSILON_H

