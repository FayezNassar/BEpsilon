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

    class Node;

    // We let a swap_space handle all the I/O.
    typedef typename swap_space::pointer<Node> NodePointer;


    BEpsilonTree(swap_space *sspace) : ss(sspace),size_(0) {
        root = NodePointer();
    }

    void insert(Key key, Value value);

    Value pointQuery(Key key);

    vector<Value> rangeQuery(Key minKey, Key maxKey);

    void remove(Key key);

    void printTree();

    bool contains(Key key);

    int size();


    /**
Approximately search a key in subtree rooted with this node,
if a given key is in the range of some leaf keys it will return that
leaf else it will return the first or the last leaf.
@param key to look up for.
@return a leaf which the given key is in the range of this leaf keys.
*/
    NodePointer approximateSearch(Key key){
        NodePointer res = root;

        while (!res->isLeaf) {
            int i=0;
            for (; i < res->keys.size() && (key >= res->keys[i]); i++) {
            }
            res = res->children[i];
        }
        return res;
    }

    class Node : public serializable {
    public:

        typedef enum {
            left,
            right
        } Direction;

        Node(bool isLeaf, NodePointer parent = NodePointer(), NodePointer right_sibling = NodePointer(), NodePointer left_sibling = NodePointer());

        /**
        Check if node is is full, node is full when it has B children.

        @return true if node is full, false else.
        */
        bool isFull();

        //A function to check if the node(leaf/internal is full or not)
        //is full(the number of key smaller than the minimum).
        bool isNotLegal();


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
        void splitChild(int ix, NodePointer child);

        //A utility function to make sure that all the leaf is on the same height.
        //should call after the key-value insertion from the leaf.
        void insertKeysUpdate();

        bool isSiblingBorrowable(Direction direction);

        void mergeKeysUpdate(int child_ix);

        //A utility function to make sure that all the leaf is on the same height.
        //should call after the key-value remove from the leaf.
        void balance(NodePointer child);

        // A utility function to insert a new key in the subtree rooted with
        // this node.
        bool insert(Key key, Value value);

        static inline void updateMinSubTreeKey(NodePointer node);

        // A utility function to remove a key in the subtree rooted with
        // this node.
        //the tree will not affected if the key isn't existing.
        bool remove(Key key);

        void inOrder(int indent = 0);

        Key minSubTreeKeyTest();

        void bPlusValidation();

        void RI();

        bool tryBorrowFromLeft();

        bool tryBorrowFromRight();

        bool tryMergeWithLeft();

        bool tryMergeWithRight();

        void updateParentKeys();

        void _serialize(std::iostream &fs, serialization_context &context) {
            fs << "isLeaf:" << std::endl;
            serialize(fs, context, isLeaf);
            fs << "parent:" << std::endl;
            serialize(fs, context, parent);
            fs << "right_sibling:" << std::endl;
            serialize(fs, context, right_sibling);
            fs << "left_sibling:" << std::endl;
            serialize(fs, context, left_sibling);
            fs << "sub_tree_min_key:" << std::endl;
            serialize(fs, context, sub_tree_min_key);
            fs << "keys:" << std::endl;
            serialize(fs, context, keys);
            fs << "values:" << std::endl;
            serialize(fs, context, values);
            fs << "children:" << std::endl;
            serialize(fs, context, children);
        }

        void _deserialize(std::iostream &fs, serialization_context &context) {
            std::string dummy;
            fs >> dummy;
            deserialize(fs, context, isLeaf);
            fs >> dummy;
            deserialize(fs, context, parent);
            fs >> dummy;
            deserialize(fs, context, right_sibling);
            fs >> dummy;
            deserialize(fs, context, left_sibling);
            fs >> dummy;
            deserialize(fs, context, sub_tree_min_key);
            fs >> dummy;
            deserialize(fs, context, keys);
            fs >> dummy;
            deserialize(fs, context, values);
            fs >> dummy;
            deserialize(fs, context, children);
        }


    private:
        bool isLeaf;
        NodePointer parent;
        vector<Key> keys;
        NodePointer right_sibling;
        NodePointer left_sibling;
        Key sub_tree_min_key;

        //if this node is a leaf
        //values.size() == keys.size(),for now it's equal to 1, keys.size.max == B-1;
        vector<Value> values;

        //if the node is internal
        //children.size() == keys.size()+1;
        vector<NodePointer> children;
        friend class BEpsilonTree;


    };


    swap_space *ss;
    NodePointer root;
    int size_;
};

template<typename Key, typename Value, int B>
BEpsilonTree<Key, Value, B>::Node::Node(bool isLeaf, NodePointer parent, NodePointer right_sibling, NodePointer left_sibling) {
    this->parent = parent;
    this->right_sibling = right_sibling;
    this->left_sibling = left_sibling;
    this->isLeaf = isLeaf;
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::isFull() {
    //choose the max number of key and values in each node according to the block size.
    return this->keys.size() == B;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::isNotLegal() {
    return this->keys.size() < B / 2;
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::Node::getKeyOrder() {
    //for sure this node isn't root and full, we check it before this function call.
    int ix = 0;
    while (ix < this->parent->keys.size() && this->parent->keys[ix] <= this->keys[0]) {
        ix++;
    }
    return ix;
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::Node::getOrder() {
    int ix = 0;
    //for sure this node isn't root and full, we check it before this function call.
    typedef typename vector<NodePointer>::iterator iterator;
    for (iterator it = this->parent->children.begin(); it != this->parent->children.end(); it++) {
        if ((*it) == this) {
            return ix;
        } else {
            ix++;
        }
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::splitChild(int ix, NodePointer left_child) {

    //this node is the parent of right and left child.
    // Create a new node which is going to store (child->keys.size()-1) keys of child
    NodePointer right_child = ss->allocate( new BEpsilonTree<Key, Value, B>::Node(left_child->isLeaf,
                                                                                           left_child->parent,
                                                                                           left_child->right_sibling,
                                                                                           left_child->left_sibling));

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
    int middle_ix = B / 2;


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
        right_child->sub_tree_min_key = right_child->keys[0];
    } else {

        //find the appropriate key and its index.
        int key = left_child->keys[middle_ix];
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
        typedef typename vector<NodePointer>::iterator iterator;
        for (iterator it = left_child->children.begin() + middle_ix + 1; it != left_child->children.end(); it++) {
            (*it)->parent = right_child;
        }

        left_child->children.erase(left_child->children.begin() + middle_ix + 1,
                                   left_child->children.end());

        right_child->sub_tree_min_key = right_child->children[0]->sub_tree_min_key;
    }

    //set the new node as a child of this node
    this->children.insert(this->children.begin() + (ix + 1), right_child);

};


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::insertKeysUpdate() {
    if (this->isFull()) {
        if (this->parent == NULL) {//this is root :)
            NodePointer node = ss->allocate(new Node(false));
            //move it the the end;
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

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::isSiblingBorrowable(Direction direction) {
    if (this->parent == NULL) {
        return false;
    }

    NodePointer sibling = direction == right ? right_sibling : left_sibling;
    return sibling != NULL && sibling->parent == this->parent && sibling->keys.size() > B / 2;
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::mergeKeysUpdate(int child_ix) {
    if (child_ix > 0) {
        this->keys.erase(this->keys.begin() + (child_ix - 1), this->keys.begin() + (child_ix - 1));
        this->children.erase(this->children.begin() + child_ix, this->children.begin() + child_ix);
    } else if (this->parent != NULL) {
        int parent_ix = this->getOrder();
        this->parent->keys[parent_ix > 0 ? parent_ix - 1 : 0] = this->keys[0];
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::tryBorrowFromLeft() {
    if (this->isSiblingBorrowable(left)) {

        if (this->isLeaf) {
            this->values.insert(this->values.begin(),
                                this->left_sibling->values.end() - 1,
                                this->left_sibling->values.end());
            this->left_sibling->values.pop_back();
            this->keys.insert(this->keys.begin(), left_sibling->keys[left_sibling->keys.size()-1]);
        } else {
            this->keys.insert(this->keys.begin(), this->children[0]->sub_tree_min_key/*getMinSubTreeKey()*/);
            this->children.insert(this->children.begin(),
                                  this->left_sibling->children.end() - 1,
                                  this->left_sibling->children.end());
            this->left_sibling->children[left_sibling->children.size() - 1]->parent = this;
            this->left_sibling->children.pop_back();
        }

        this->left_sibling->keys.pop_back();
        updateMinSubTreeKey(this);
        left_sibling->updateParentKeys();
        this->updateParentKeys();
        return true;
    }

    return false;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::tryBorrowFromRight() {
    if (this->isSiblingBorrowable(right)) {

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

        this->right_sibling->keys.erase(this->right_sibling->keys.begin());
        updateMinSubTreeKey(this);
        updateMinSubTreeKey(right_sibling);
        right_sibling->updateParentKeys();
        this->updateParentKeys();
        return true;
    }

    return false;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::tryMergeWithLeft() {
    if (!left_sibling || (left_sibling->parent != this->parent)) {
        return false;
    }

    if (isLeaf) {
        left_sibling->values.insert(left_sibling->values.end(), this->values.begin(), this->values.end());
        left_sibling->keys.insert(left_sibling->keys.end(),this->keys.begin(),this->keys.end());
        values.erase(values.begin(), values.end());

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

    keys.erase(keys.begin(), keys.end());
    return true;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::tryMergeWithRight() {
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

    keys.erase(keys.begin(), keys.end());
    updateMinSubTreeKey(right_sibling);
    return true;
}

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::updateParentKeys() {
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


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::balance(NodePointer child) {
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
        if(parent) {
            this->updateParentKeys();
        }
    }

    if (parent) {
        parent->balance(this);
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::updateMinSubTreeKey(NodePointer node){
    if(node->keys.size() == 0) return;
    if(node->isLeaf){
        node->sub_tree_min_key = node->keys[0];
    } else {
        node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
    }
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::insert(Key key, Value value) {

    //for finding the key position, we'll start with last index
    int ix = this->keys.size() - 1;
    //find the key index.
    while (ix >= 0 && keys[ix] > key) {
        ix--;
    }

    if (this->isLeaf) {
        for(Key k: this->keys){
            if(k == key){
                return false;
            }
        }
        this->keys.insert(this->keys.begin() + (ix + 1), key);
        this->values.insert(this->values.begin() + (ix + 1), value);
        this->sub_tree_min_key = this->keys[0];
        this->insertKeysUpdate();
        return true;
    } else {//this is internal node
        ix = ix == -1 ? 0 : ix;
        if (this->keys[ix] < key) {
            ix++;
        }
        if(key < this->sub_tree_min_key) {
            this->sub_tree_min_key = key;
        }
        return this->children[ix]->insert(key, value);
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::Node::remove(Key key) {
    //the position of the key in the node.
    int ix = this->keys.size() - 1;
    while (ix >= 0 && this->keys[ix] > key) {
        ix--;
    }
    if (ix == -1) {
        ix = 0;
    }
    if (this->isLeaf) {
        if (this->keys[ix] == key) {
            this->keys.erase(this->keys.begin() + ix);
            this->values.erase(this->values.begin() + ix);
            this->balance(NULL);
            return true;
        }
        return false;
    } else {
        if (this->keys[ix] <= key) {
            ix++;
        }
        return this->children[ix]->remove(key);
    }
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::inOrder(int indent) {
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

template<typename Key, typename Value, int B>
Key BEpsilonTree<Key, Value, B>::Node::minSubTreeKeyTest() {
    Key min = this->sub_tree_min_key;
    if(!isLeaf) {
        Key minChildrenKey = this->children[0]->minSubTreeKeyTest();

        for(NodePointer child: children){
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

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::Node::bPlusValidation() {
    //root can have less than B/2 keys.
    assert((this->parent == NULL) || (this->keys.size() < B && this->keys.size() >= B/2));
    assert(std::is_sorted(this->keys.begin(),this->keys.end()));
    if(isLeaf) {
        assert(this->keys.size() == this->values.size());
    } else {
        assert(this->keys.size()+1 == this->children.size());
        for(NodePointer node : this->children) {
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
    if (root == NULL) { // if the Tree is empty
        root = ss->allocate(new Node(true));
        root->keys.insert(root->keys.begin(), key);
        root->values.insert(root->values.begin(), value);
        root->sub_tree_min_key = key;
    } else { //if the root is not null.
        if(root->insert(key, value)){
            size_++;
        }
        if (root->parent != NULL) {
            root = root->parent;
        }
    }
    //TODO remove when analysis
//    root->RI();
};

template<typename Key, typename Value, int B>
vector<Value> BEpsilonTree<Key, Value, B>::rangeQuery(Key minKey, Key maxKey) {
    if (minKey > maxKey) {
        throw InvalidKeyRange();
    }
    vector<Value> res;
    if (root != NULL) {
        //get appropriate leaf
        NodePointer current = approximateSearch(minKey);
        Value maxFound = minKey;
        bool flag = true;

        while (current != NULL && maxKey >= maxFound && flag) {
            for (int i = 0; i < current->keys.size(); ++i) {
                if (minKey <= current->keys[i] && current->keys[i] <= maxKey) {
                    res.push_back(current->values[i]);
                    maxFound = current->keys[i];
                    if(maxFound == maxKey){
                        flag = false;
                    }
                }
            }
            current = current->right_sibling;
        }
    }
    return res;
}

template<typename Key, typename Value, int B>
Value BEpsilonTree<Key, Value, B>::pointQuery(Key key) {
    vector<Value> res = rangeQuery(key, key);
    if (!res.size()) {
        throw NoSuchKeyException();
    }
    return res[0];
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::remove(Key key) {
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
    //TODO remove when analysis
//    root->RI();
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::printTree() {
    if(root != NULL) {
        root->inOrder();
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::contains(Key key) {
    try{
        pointQuery(key);
        return true;
    } catch(NoSuchKeyException){
        return false;
    }
};

template<typename Key, typename Value, int B>
int BEpsilonTree<Key, Value, B>::size() {
    return size_;
};

#endif //BEPSILON_BEPSILON_H


