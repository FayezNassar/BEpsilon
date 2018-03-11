#ifndef BEPSILON_BEPSILON_H
#define BEPSILON_BEPSILON_H

#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <map>

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
        return "Invalid keys range.";
    }
};

template<typename T, class sizer>
static inline int get_size(const T& t) {
    return sizer::getSize(t);
}

template<typename Key, typename Value>
class BEpsilonTree {
public:
    BEpsilonTree() : root(NULL),size_(0) {
        cout << "EPSILON: " << Node::EPSILON << endl;
        cout << "NODE_SIZE: " << Node::NODE_SIZE << endl;
        cout << "MAX_NUMBER_OF_MESSAGE_PER_NODE: " << Node::MAX_NUMBER_OF_MESSAGE_PER_NODE << endl;
        cout << "B: " << Node::B << endl;
    }

    void insert(Key key, Value value);

    bool pointQuery(Key key, Value& value);

    void remove(Key key);

    void printTree();

    bool contains(Key key);

    int size();

private:
    typedef enum {
        INSERT,
        REMOVE,
        UPDATE,
        ANY
    } Opcode;

    class Message {
    public:
        Message(Opcode opcode, Key key, Value value) : opcode(opcode), key(key), value(value) {};
        bool operator<(const Message& other) {//for std::sort method
            return this->key > other.key;
        }
        bool operator==(const Message& other) {
            return this->key == other.key;
        }
        //TODO add size function that calc the exact message size, when the Key/Value size isn't const.
    private:
        Opcode opcode;
        Key key;
        Value value;
        friend class BEpsilonTree;
    };

    class Node {
    public:
        static constexpr int NODE_SIZE = 128;
        static constexpr double EPSILON = 0.3;
        static constexpr int BUFFER_SIZE = NODE_SIZE * EPSILON;
        static constexpr int MESSAGE_SIZE = sizeof(Message);
        static constexpr int MAX_NUMBER_OF_MESSAGE_PER_NODE = BUFFER_SIZE/MESSAGE_SIZE;
        static constexpr int NODE_META_DATA_SIZE = 13; // 3 pointers and bool.
        static constexpr int B = (((1-EPSILON) * NODE_SIZE) - NODE_META_DATA_SIZE)/12;

        typedef typename vector<BEpsilonTree<Key, Value>::Message>::iterator MessageIterator;
        typedef typename vector<BEpsilonTree<Key, Value>::Node*>::iterator   ChildIterator;

        typedef enum {
            LEFT,
            RIGHT
        } Direction;

        Node(bool isLeaf, Node *parent = NULL, Node *right_sibling = NULL, Node *left_sibling = NULL);

        /**
        Check if node is is full, node is full when it has B children.

        @return true if node is full, false else.
        */
        bool isFull();

        //A function to check if the node(leaf/internal is full or not)
        //is full(the number of key smaller than the minimum).
        bool isNotLegal();

        /**
        Approximately search a key in subtree rooted with this node,
        if a given key is in the range of some leaf keys it will return that
        leaf else it will return the first or the last leaf.
        @param key to look up for.
        @return a leaf which the given key is in the range of this leaf keys.
        */
        Node *approximateSearch(Key key);

        bool pointQuery(Key key, Value& value);

        // A function that returns the index of the key in the parent that that point to this
        // the assumption is that parent != NULL and this node have some keys.
        //unused for now.
        int getKeyOrder();

        //A function to find the index of this node in the his parent children vector
        //the assumption is this->parent != NULL.
        int getOrder();


        // A utility function to split the child of this node. ix is index
        // of child in child vector. The Child must be full when this
        // function is called
        void splitChild(int ix, Node *child);

        //A utility function to make sure that all the leaf is on the same height.
        //should call after the key-value insertion from the leaf.
        void insertKeysUpdate();

        bool isSiblingBorrowable(Direction direction);

        void mergeKeysUpdate(int child_ix);

        //A utility function to make sure that all the leaf is on the same height.
        //should call after the key-value remove from the leaf.
        void balance(Node *child);

        // A utility function to insert a new key in the subtree rooted with
        // this node.
        bool insert(Key key, Value value);

        static inline void updateMinSubTreeKey(Node* node);

        // A utility function to remove a key in the subtree rooted with
        // this node.
        //the tree will not affected if the key isn't existing.
        bool remove(Key key);

        bool insertMessage(Opcode opcode, Key key, Value value = Value());

        bool insertMessages(MessageIterator begin, MessageIterator end);

        bool isMessagesBufferFull();

        void bufferFlushIfFull();

        static void insertMessage(vector<Message> &buff, Message m);

        void inOrder(int indent = 0);

        Key minSubTreeKeyTest();

        void bPlusValidation();

        void RI();

        bool tryBorrowFromLeft();

        bool tryBorrowFromRight();

        bool tryMergeWithLeft();

        bool tryMergeWithRight();

        void updateParentKeys();

    private:
        bool isLeaf;
        Node *parent;
        vector<Key> keys;
        Node *right_sibling;
        Node *left_sibling;
        Key sub_tree_min_key;

        //if this node is a leaf
        //values.size() == keys.size(),for now it's equal to 1, keys.size.max == B-1;
        vector<Value> values;

        //if the node is internal
        //children.size() == keys.size()+1;
        vector<Node *> children;

        //balanced message_buff for O(log(# of messages in the buffer)) insertion/deletion/query.
        vector<Message> message_buff;
        friend class BEpsilonTree;

    };

    Node *root;
    int size_;
};

template<typename Key, typename Value>
BEpsilonTree<Key, Value>::Node::Node(bool isLeaf, Node *parent, Node *right_sibling, Node *left_sibling) {
    this->parent = parent;
    this->right_sibling = right_sibling;
    this->left_sibling = left_sibling;
    this->isLeaf = isLeaf;
};

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::isFull() {
    //choose the max number of key and values in each node according to the block size.
    return this->keys.size() >= B;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::isNotLegal() {
    return this->keys.size() < B / 2;
};

template<typename Key, typename Value>
int BEpsilonTree<Key, Value>::Node::getKeyOrder() {
    //for sure this node isn't root and full, we check it before this function call.
    int ix = 0;
    while (ix < this->parent->keys.size() && this->parent->keys[ix] <= this->keys[0]) {
        ix++;
    }
    return ix;
};

template<typename Key, typename Value>
int BEpsilonTree<Key, Value>::Node::getOrder() {
    int ix = 0;
    //for sure this node isn't root and full, we check it before this function call.
    typedef typename vector<BEpsilonTree<Key, Value>::Node *>::iterator iterator;
    for (iterator it = this->parent->children.begin(); it != this->parent->children.end(); it++) {
        if ((*it) == this) {
            return ix;
        } else {
            ix++;
        }
    }
};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::splitChild(int ix, BEpsilonTree<Key, Value>::Node *left_child) {

    //this node is the parent of right and left child.
    // Create a new node which is going to store (child->keys.size()-1) keys of child
    BEpsilonTree<Key, Value>::Node *right_child = new BEpsilonTree<Key, Value>::Node(left_child->isLeaf,
                                                                                     left_child->parent,
                                                                                     left_child->right_sibling,
                                                                                     left_child->left_sibling);

    //update the sibling of both child and new node, add the new node between the child and the new node.
    //if the nods is internal and not a leaf, the sibling will be NULL.
    if (left_child->right_sibling != NULL) {
        left_child->right_sibling->left_sibling = right_child;
    }
    right_child->left_sibling = left_child;
    right_child->right_sibling = left_child->right_sibling;
    left_child->right_sibling = right_child;
    //left_child->left_sibling and left_child->right_sibling->right_sibling doesn't change.

    //update to move the minimum number of children for each node, and not 1.
    //B should be grater than 2, else infinite loop will occur.
    int middle_ix = B/2;
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

        this->keys.insert(this->keys.begin() + ix, right_child->keys[0]);

        right_child_min_key = right_child->keys[0];
        right_child->sub_tree_min_key = right_child->keys[0];
    } else {
        //find the appropriate key and its index.
        int key = left_child->keys[middle_ix];
        right_child_min_key = key;

        int key_ix = 0;
        while ((key_ix >= 0) && (key_ix < this->keys.size()) && (this->keys[key_ix] < key)) {
            key_ix++;
        }

        this->keys.insert(this->keys.begin() + key_ix, key);

        right_child->keys.insert(right_child->keys.begin(),
                                 left_child->keys.begin() + (middle_ix + 1),
                                 left_child->keys.end());

        left_child->keys.erase(left_child->keys.begin() + (middle_ix),
                               left_child->keys.end());

        right_child->children.insert(right_child->children.begin(),
                                     left_child->children.begin() + middle_ix + 1,
                                     left_child->children.end());

        // w/o this, compilation error, with nested template in dependent scope.
        typedef typename vector<BEpsilonTree<Key, Value>::Node *>::iterator iterator;
        for (iterator it = left_child->children.begin() + middle_ix + 1; it != left_child->children.end(); it++) {
            (*it)->parent = right_child;
        }

        left_child->children.erase(left_child->children.begin() + middle_ix + 1,
                                   left_child->children.end());

        right_child->sub_tree_min_key = right_child->children[0]->sub_tree_min_key;
    }

    MessageIterator first_message_it = left_child->message_buff.begin();
    while(first_message_it != left_child->message_buff.end() && first_message_it->key <= right_child_min_key) {
        first_message_it++;
    }
    right_child->message_buff.insert(right_child->message_buff.begin(),
                                                                                                      first_message_it,
                                                                                                      left_child->message_buff.end());
    left_child->message_buff.erase(first_message_it, left_child->message_buff.end());

    //set the new node as a child of this node
    this->children.insert(this->children.begin() + (ix + 1), right_child);
};


template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::insertKeysUpdate() {
    if (this->isFull()) {
        if (this->parent == NULL) {//this is root :)
            Node *node = new Node(false);
            this->parent = node;
            node->children.insert(node->children.begin(), this);
            node->splitChild(0, this);
            node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
        } else {
            this->parent->splitChild(this->getOrder(), this);
            this->parent->sub_tree_min_key = this->parent->children[0]->sub_tree_min_key;
            this->parent->insertKeysUpdate();
        }
    }
};

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::isSiblingBorrowable(Direction direction) {
    if (this->parent == NULL) {
        return false;
    }

    Node *sibling = direction == RIGHT ? right_sibling : left_sibling;
    return sibling != NULL && sibling->parent == this->parent && sibling->keys.size() > B / 2;
};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::mergeKeysUpdate(int child_ix) {
    if (child_ix > 0) {
        this->keys.erase(this->keys.begin() + (child_ix - 1), this->keys.begin() + (child_ix - 1));
        this->children.erase(this->children.begin() + child_ix, this->children.begin() + child_ix);
    } else if (this->parent != NULL) {
        int parent_ix = this->getOrder();
        this->parent->keys[parent_ix > 0 ? parent_ix - 1 : 0] = this->keys[0];
    }
};

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::tryBorrowFromLeft() {
    if (this->isSiblingBorrowable(LEFT)) {
        if (this->isLeaf) {
            this->values.insert(this->values.begin(),
                                this->left_sibling->values.end() - 1,
                                this->left_sibling->values.end());
            this->left_sibling->values.pop_back();
            this->keys.insert(this->keys.begin(), left_sibling->keys[left_sibling->keys.size()-1]);
        } else {
            this->keys.insert(this->keys.begin(), this->children[0]->sub_tree_min_key);
            this->children.insert(this->children.begin(),
                                  this->left_sibling->children.end() - 1,
                                  this->left_sibling->children.end());
            this->left_sibling->children[left_sibling->children.size() - 1]->parent = this;
            this->left_sibling->children.pop_back();
        }

        Key key = this->isLeaf ?
                  left_sibling->keys[left_sibling->keys.size()-1] : this->children[0]->sub_tree_min_key;
        MessageIterator l_it = this->left_sibling->message_buff.begin();
        while(l_it->key < key) {
            l_it++;
        }
        this->message_buff.insert(this->message_buff.begin() , l_it, this->left_sibling->message_buff.end());
        this->left_sibling->message_buff.erase(l_it, this->left_sibling->message_buff.end());
        this->left_sibling->keys.pop_back();
        updateMinSubTreeKey(this);
        left_sibling->updateParentKeys();
        this->updateParentKeys();
        this->bufferFlushIfFull();
        return true;
    }
    return false;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::tryBorrowFromRight() {
    if (this->isSiblingBorrowable(RIGHT)) {
        if (this->isLeaf) {
            this->values.insert(this->values.end(),this->right_sibling->values[0]);
            right_sibling->values.erase(this->right_sibling->values.begin());
            this->keys.insert(this->keys.end(),right_sibling->keys[0]);

        } else {
            this->keys.insert(this->keys.end(),right_sibling->children[0]->sub_tree_min_key);
            this->children.insert(this->children.end(), this->right_sibling->children[0]);
            this->right_sibling->children[0]->parent = this;
            this->right_sibling->children.erase(this->right_sibling->children.begin());
        }

        Key key = this->isLeaf ?
                  right_sibling->keys[0] : right_sibling->children[0]->sub_tree_min_key;
        MessageIterator r_it = this->right_sibling->message_buff.begin();
        while(r_it->key < key) {
            r_it++;
        }
        this->message_buff.insert(this->message_buff.end() , this->right_sibling->message_buff.begin(), r_it);
        this->right_sibling->message_buff.erase(this->right_sibling->message_buff.begin(), r_it);

        this->right_sibling->keys.erase(this->right_sibling->keys.begin());
        updateMinSubTreeKey(this);
        updateMinSubTreeKey(right_sibling);
        right_sibling->updateParentKeys();
        this->updateParentKeys();
        return true;
    }

    return false;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::tryMergeWithLeft() {
    if (!left_sibling || (left_sibling->parent != this->parent)) {
        return false;
    }

    if (isLeaf) {
        this->left_sibling->values.insert(left_sibling->values.end(), this->values.begin(), this->values.end());
        this->left_sibling->keys.insert(left_sibling->keys.end(), this->keys.begin(), this->keys.end());
        this->values.erase(values.begin(), values.end());
    } else {
        bool first_time = true;
        while (children.size() > 0) {
            if(first_time){
                left_sibling->keys.insert(left_sibling->keys.end(), this->children[0]->sub_tree_min_key);
                first_time=false;
            } else{
                left_sibling->keys.insert(left_sibling->keys.end(), this->keys[0]);
                this->keys.erase(keys.begin());
            }

            left_sibling->children.push_back(children[0]);
            children[0]->parent = left_sibling;
            children.erase(children.begin());
        }
    }
    this->left_sibling->message_buff.insert(this->left_sibling->message_buff.end(),
                                            this->message_buff.begin(),
                                            this->message_buff.end()
    );
    this->message_buff.erase(this->message_buff.begin(),
                             this->message_buff.end()
    );
    keys.erase(keys.begin(), keys.end());
    this->left_sibling->bufferFlushIfFull();
    return true;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::tryMergeWithRight() {
    if (!right_sibling || (right_sibling->parent != this->parent)) {
        return false;
    }

    if (this->isLeaf) {
        right_sibling->values.insert(right_sibling->values.begin(), this->values.begin(), this->values.end());
        values.erase(values.begin(), values.end());
        right_sibling->keys.insert(right_sibling->keys.begin(),this->keys.begin(),this->keys.end());
    } else {
        bool first_time = true;
        while (children.size() > 0) {
            int last_index = children.size()-1;
            if(first_time){
                right_sibling->keys.insert(right_sibling->keys.begin(), right_sibling->children[0]->sub_tree_min_key);
                first_time=false;
            } else{
                right_sibling->keys.insert(right_sibling->keys.begin(), this->keys[this->keys.size()-1]);
                this->keys.pop_back();
            }
            right_sibling->children.insert(right_sibling->children.begin(), children[last_index]);
            children[last_index]->parent = right_sibling;
            children.pop_back();
        }
    }
    this->right_sibling->message_buff.insert(this->right_sibling->message_buff.begin(),
                                             this->message_buff.begin(),
                                             this->message_buff.end());
    this->message_buff.erase(this->message_buff.begin(),
                             this->message_buff.end()
    );
    keys.erase(keys.begin(), keys.end());
    updateMinSubTreeKey(right_sibling);
    if(this->left_sibling != NULL) {
        this->left_sibling->bufferFlushIfFull();
    }
    return true;
}

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::updateParentKeys() {
    //we shouldn't move keys from node to other,
    // but we yes should update the parent key to have the minimum key in this node in the case of minimum key remove
    if (parent != NULL && this->parent->keys.size() > 0) {
        int my_index = this->getOrder();
        int update_idx = (my_index > 0) ? my_index - 1 : 0;

        if (this->keys.size() == 0) {
            parent->children.erase(parent->children.begin() + my_index);
            parent->keys.erase(parent->keys.begin()+update_idx);
        } else {
            if(my_index>update_idx){
                parent->keys[update_idx] = this->sub_tree_min_key;
            }
        }
        updateMinSubTreeKey(parent);
    }
};


template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::balance(Node *child) {
    if (child && child->keys.size() == 0) {
        child->updateParentKeys();
        if (child->left_sibling) {
            child->left_sibling->right_sibling = child->right_sibling;
        }
        if (child->right_sibling) {
            child->right_sibling->left_sibling = child->left_sibling;
        }
        delete child;
        child = NULL;
    }
    if (isNotLegal()) {
        if (!tryBorrowFromLeft()) {
            if (!tryBorrowFromRight()) {
                if (!tryMergeWithLeft()) {
                    tryMergeWithRight();
                }
            }
        }
    } else {
        updateMinSubTreeKey(this);
        if(this->parent) {
            this->updateParentKeys();
        }
    }
    if (this->parent) {
        this->parent->balance(this);
    }
};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::updateMinSubTreeKey(Node* node){
    if(node->keys.size() == 0) return;
    if(node->isLeaf){
        node->sub_tree_min_key = node->keys[0];
    } else {
        node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
    }
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::insert(Key key, Value value) {
    return this->insertMessage(INSERT, key, value);
};

template<typename Key, typename Value>
typename BEpsilonTree<Key, Value>::Node *BEpsilonTree<Key, Value>::Node::approximateSearch(Key key) {
    Node *res = this;
    while (!res->isLeaf) {
        int i=0;
        for (; i < res->keys.size() && (key >= res->keys[i]); i++) {
        }
        res = res->children[i];
    }
    return res;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::pointQuery(Key key, Value& value) {
    Message m(ANY, key, Value());
    MessageIterator message_it = std::find(this->message_buff.begin(), this->message_buff.end(), m);
    if(message_it != this->message_buff.end()) { // the key is appear in
        switch(message_it->opcode) {
            case REMOVE : return false;
            case INSERT : value = message_it->value; return true;
            default: assert("no such opcode");
        }
    } else if (this->isLeaf) {
        typename vector<Key>::iterator key_it = this->keys.begin();
        int ix = 0;
        for(; key_it != this->keys.end() && *key_it < key; key_it++, ix++) {}
        if(key_it != this->keys.end() && *key_it == key) {
            value = this->values[ix];
            return true;
        }
    } else {
        ChildIterator child_it = this->children.begin();
        typename vector<Key>::iterator key_it = this->keys.begin();
        while(key_it != this->keys.end() && (*key_it) <= key) {
            child_it++;
            key_it++;
        }
        Node* child = child_it != this->children.end() ? (*child_it) : this->children.back();
        return child->pointQuery(key, value);
    }
    return false;
}

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::remove(Key key) {
    return this->insertMessage(REMOVE, key);
};

template <typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::insertMessage(Opcode opcode, Key key, Value value) {
    Message message(opcode, key, value);
    //ix will contains the appropriate index in the message.key in the message buffer.
    int ix = 0;
    for(;ix < this->message_buff.size() && this->message_buff[ix].key < key;ix++) {}

    if(ix < this->message_buff.size() && this->message_buff[ix].key == key) {
        this->message_buff[ix] = message;
    } else {
        this->message_buff.insert(this->message_buff.begin() + ix, message);
    }
    //ask after the insert if there a need to split the message buffer.
    this->bufferFlushIfFull();
    //ask after the insert if there a need to split the message buffer.
    if(!this->keys.empty() && this->keys[0] < this->sub_tree_min_key) {
        this->sub_tree_min_key = this->keys[0];
    }
    return true;
}

template <typename Key, typename Value>
bool BEpsilonTree<Key, Value>::Node::isMessagesBufferFull() {
    return this->message_buff.size() >=  MAX_NUMBER_OF_MESSAGE_PER_NODE;
};

template <typename  Key, typename Value>
void BEpsilonTree<Key, Value>::Node::insertMessage(vector<Message> &buff, Message m) {
    int ix = 0;
    for(; ix < buff.size() && buff[ix].key < m.key; ix++);
    if(ix < buff.size() && buff[ix].key == m.key) {
        buff[ix] = m;
    } else {
        buff.insert(buff.begin() + ix, m);
    }
};

/*
 * when the buffer got empty, we need to flush the message into the key, value buffers,
 * and then handle the node separate.*/
template <typename  Key, typename Value>
void BEpsilonTree<Key, Value>::Node::bufferFlushIfFull() {
    if(this->isMessagesBufferFull() == false) return;
    vector<Message> tmp;
    if(this->isLeaf) { //i.e. leaf node.. so apply the messages.
        for(MessageIterator m = this->message_buff.begin(); m != this->message_buff.end(); m++) {
            if(m->opcode == REMOVE) {
                int ix;
                for(ix = 0; ix < this->keys.size() && this->keys[ix] < m->key; ix++) {}
                if(ix < this->keys.size() && this->keys[ix] == m->key) {
                    if(this->keys[ix] == m->key) {
                        this->keys.erase(this->keys.begin() + ix);
                        this->values.erase(this->values.begin() + ix);
                    }
                }//here
                tmp.push_back(*m);
            }
        }
        for(Message m : tmp) {
            MessageIterator m_it = std::find(this->message_buff.begin(), this->message_buff.end(), m);
            this->message_buff.erase(m_it);
        }
        int num_of_applied_message = 0;
        for(Message m : this->message_buff) {
            if(this->isFull()) break;
            int ix; for(ix = 0; ix < this->keys.size() && this->keys[ix] < m.key; ix++) {}
            if(m.opcode == INSERT) {
                this->keys.insert(this->keys.begin() + ix, m.key);
                this->values.insert(this->values.begin() + ix, m.value);
                num_of_applied_message++;
            } else {
                assert("unexpected opcode!!!");
            }
        }
        this->message_buff.erase(this->message_buff.begin(), this->message_buff.begin() + num_of_applied_message);
        this->sub_tree_min_key = this->keys[0];
        this->insertKeysUpdate();
        this->balance(NULL);
    } else {
        MessageIterator message_it = this->message_buff.begin();
        ChildIterator child_it = this->children.begin();
        for(int key_ix = 0; key_ix < this->keys.size(); key_ix++) {
            while((message_it != this->message_buff.end()) && (message_it->key < this->keys[key_ix])) {
                insertMessage((*child_it)->message_buff, *message_it);
                message_it++;
            }
            child_it++;
        }
        if(message_it != this->message_buff.end()) {
            Node* child = *child_it;
            while(message_it != this->message_buff.end()) {
                insertMessage(child->message_buff, *message_it);
                message_it++;
            }
        }
        this->message_buff.erase(this->message_buff.begin(), this->message_buff.end());
        Node* child = this->children.front();
        Node* right_child = NULL;
        while(child != NULL) {
            right_child = child->right_sibling;
            child->bufferFlushIfFull();
            child = right_child;
        }
    }
}

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::inOrder(int indent) {
    if(!isLeaf) {
        this->children[this->children.size()-1]->inOrder(indent + 4);
    }
    for(int i = this->keys.size()-1 ; i >= 0 ; i--) {
        if(indent > 0) {
            cout << setw(indent) << ' ';
        }
        cout << setw(indent) << this->keys[i] << endl;
        if(!isLeaf) {
            this->children[i]->inOrder(indent + 4);
        }
    }
    cout << setw(indent) << ' ';
    cout << setw(indent) << "----" << endl;
}

template<typename Key, typename Value>
Key BEpsilonTree<Key, Value>::Node::minSubTreeKeyTest() {
    Key min = this->sub_tree_min_key;
    if(!isLeaf) {
        Key minChildrenKey = this->children[0]->minSubTreeKeyTest();

        for(Node* child: children){
            Key min_key = child->minSubTreeKeyTest();
            if(min_key <= minChildrenKey){
                minChildrenKey = min_key;
            }
        }
        assert(this->sub_tree_min_key == minChildrenKey);
    } else {
        assert(min == this->keys[0]);
    }
    return min;
}

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::bPlusValidation() {
    //root can have less than B/2 keys.
    assert((this->parent == NULL) || (this->keys.size() < B && this->keys.size() >= B/2));
    assert(std::is_sorted(this->keys.begin(),this->keys.end()));
    if(isLeaf) {
        assert(this->keys.size() == this->values.size());
    } else {
        assert(this->keys.size()+1 == this->children.size());
        for(Node* node : this->children) {
            node->bPlusValidation();
        }
    }

};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::Node::RI() {
    minSubTreeKeyTest();
    bPlusValidation();
}

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::insert(Key key, Value value) {
    if (root == NULL) { // if the Tree is empty
        root = new Node(true);
    }
    bool insertion_success_ = root->insert(key, value);
    if(insertion_success_) {
        size_++;
    }
    if(root->parent != NULL) {
        root = root->parent;
    }
};

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::pointQuery(Key key, Value& value) {
    if(root != NULL) {
        return root->pointQuery(key, value);
    }
    return false;
};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::remove(Key key) {
    if (root != NULL) {
        if(root->remove(key)) {
            size_--;
        }
        if (root->children.size() == 1) {
            root = root->children[0];
            delete root->parent;
            root->parent = NULL;
        }
    }
    if(root->parent != NULL) {
        root = root->parent;
    }
};

template<typename Key, typename Value>
void BEpsilonTree<Key, Value>::printTree() {
    if(root != NULL) {
        root->inOrder();
    }
};

template<typename Key, typename Value>
bool BEpsilonTree<Key, Value>::contains(Key key) {
    Value value;
    return pointQuery(key, value);
};

template<typename Key, typename Value>
int BEpsilonTree<Key, Value>::size() {
    return size_;
};

#endif //BEPSILON_BEPSILON_H

