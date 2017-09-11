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
        left,
        right
    } Direction;

    class Node;
    // We let a swap_space handle all the I/O.
    typedef typename swap_space::pointer<Node> NodePointer;
    BEpsilonTree(swap_space *sspace) : ss(sspace),size_(0){
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

        Node(bool isLeaf=true, NodePointer parent = NodePointer(), NodePointer right_sibling = NodePointer(), NodePointer left_sibling = NodePointer());

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
    bool insert(NodePointer p,Key key, Value value);

    //A function to check if the node(leaf/internal is full or not)
    //is full(the number of key smaller than the minimum).
    bool isNotLegal(NodePointer p);

    //A function to find the index of this node in the his parent children vector
    //the assumption is this->parent != NULL.
    int getOrder(NodePointer p);

    // A utility function to split the child of this node. ix is index
    // of child in child vector. The Child must be full when this
    // function is called
    void splitChild(NodePointer p,int ix, NodePointer child);

    bool isSiblingBorrowable(NodePointer p,Direction direction);

    void mergeKeysUpdate(NodePointer p,int child_ix);

    //A utility function to make sure that all the leaf is on the same height.
    //should call after the key-value remove from the leaf.
    void balance(NodePointer p,NodePointer child);

    // A utility function to remove a key in the subtree rooted with
    // this node.
    //the tree will not affected if the key isn't existing.
    bool remove(NodePointer p,Key key);


    bool tryBorrowFromLeft(NodePointer p);

    bool tryBorrowFromRight(NodePointer p);

    bool tryMergeWithLeft(NodePointer p);

    bool tryMergeWithRight(NodePointer p);

    void updateParentKeys(NodePointer p);

};

template<typename Key, typename Value, int B>
BEpsilonTree<Key, Value, B>::Node::Node(bool isLeaf, NodePointer parent, NodePointer right_sibling, NodePointer left_sibling) {
    this->parent = parent;
    this->right_sibling = right_sibling;
    this->left_sibling = left_sibling;
    this->isLeaf = isLeaf;
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isFull(NodePointer p) {
    //choose the max number of key and values in each node according to the block size.
    return p->keys.size() == B;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isNotLegal(NodePointer p) {
    return p->keys.size() < B / 2;
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
void BEpsilonTree<Key, Value, B>::splitChild(NodePointer p,int ix, NodePointer left_child) {

    //this node is the parent of right and left child.
    // Create a new node which is going to store (child->keys.size()-1) keys of child
    NodePointer right_child = ss->allocate( new BEpsilonTree<Key, Value, B>::Node(left_child->isLeaf,
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
        right_child->sub_tree_min_key = right_child->keys[0];
    } else {

        //find the appropriate key and its index.
        Key key = left_child->keys[middle_ix];
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

    //set the new node as a child of this node
    p->children.insert(p->children.begin() + (ix + 1), right_child);

};


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::insertKeysUpdate(NodePointer p) {
    if (isFull(p)) {
        if (p->parent.isNull()) {//this is root :)
            NodePointer node = ss->allocate(new Node(false));
            //move it the the end;
            p->parent = node;
            node->children.insert(node->children.begin(), p);
            splitChild(node,0, p);
            node->sub_tree_min_key = node->children[0]->sub_tree_min_key;
        } else {
            splitChild(p->parent,getOrder(p), p);
            p->parent->sub_tree_min_key = p->parent->children[0]->sub_tree_min_key;
            insertKeysUpdate(p->parent);
        }
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::isSiblingBorrowable(NodePointer p,Direction direction) {
    if (p->parent.isNull()) {
        return false;
    }

    NodePointer sibling = direction == right ? p->right_sibling : p->left_sibling;
    return !sibling.isNull() && sibling->parent == p->parent && sibling->keys.size() > B / 2;
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::mergeKeysUpdate(NodePointer p,int child_ix) {
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
    if (isSiblingBorrowable(p,left)) {

        if (p->isLeaf) {
            p->values.insert(p->values.begin(),
                                p->left_sibling->values.end() - 1,
                                p->left_sibling->values.end());
            p->left_sibling->values.pop_back();
            p->keys.insert(p->keys.begin(), p->left_sibling->keys[p->left_sibling->keys.size()-1]);
        } else {
            p->keys.insert(p->keys.begin(), p->children[0]->sub_tree_min_key/*getMinSubTreeKey()*/);
            p->children.insert(p->children.begin(),
                                  p->left_sibling->children.end() - 1,
                                  p->left_sibling->children.end());
            p->left_sibling->children[p->left_sibling->children.size() - 1]->parent = p;
            p->left_sibling->children.pop_back();
        }

        p->left_sibling->keys.pop_back();
        p->updateMinSubTreeKey(p);
        updateParentKeys(p->left_sibling);
        updateParentKeys(p);
        return true;
    }

    return false;
}

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::tryBorrowFromRight(NodePointer p) {
    if (isSiblingBorrowable(p,right)) {

        if (p->isLeaf) {
            p->values.insert(p->values.end(),p->right_sibling->values[0]);
            p->right_sibling->values.erase(p->right_sibling->values.begin());
            p->keys.insert(p->keys.end(),p->right_sibling->keys[0]);

        } else {
            p->keys.insert(p->keys.end(),p->right_sibling->children[0]->sub_tree_min_key);
            p->children.insert(p->children.end(), p->right_sibling->children[0]);
            p->right_sibling->children[0]->parent = p;
            p->right_sibling->children.erase(p->right_sibling->children.begin());
        }

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
        p->left_sibling->keys.insert(p->left_sibling->keys.end(),p->keys.begin(),p->keys.end());
        p->values.erase(p->values.begin(), p->values.end());

    } else {
        bool first_time = true;
        while (p->children.size() > 0) {
            if(first_time){
                p->left_sibling->keys.insert(p->left_sibling->keys.end(), p->children[0]->sub_tree_min_key);
                first_time=false;
            } else{
                p->left_sibling->keys.insert(p->left_sibling->keys.end(), p->keys[0]);
                p->keys.erase(p->keys.begin());
            }

            p->left_sibling->children.push_back(p->children[0]);
            p->children[0]->parent = p->left_sibling;
            p->children.erase(p->children.begin());
        }
    }

    p->keys.erase(p->keys.begin(), p->keys.end());
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
        p->right_sibling->keys.insert(p->right_sibling->keys.begin(),p->keys.begin(),p->keys.end());
    } else {
        bool first_time = true;
        while (p->children.size() > 0) {
            int last_index = p->children.size()-1;
            if(first_time){
                p->right_sibling->keys.insert(p->right_sibling->keys.begin(), p->right_sibling->children[0]->sub_tree_min_key);
                first_time=false;
            } else{
                p->right_sibling->keys.insert(p->right_sibling->keys.begin(), p->keys[p->keys.size()-1]);
                p->keys.pop_back();
            }
            p->right_sibling->children.insert(p->right_sibling->children.begin(), p->children[last_index]);
            p->children[last_index]->parent = p->right_sibling;
            p->children.pop_back();
        }
    }

    p->keys.erase(p->keys.begin(), p->keys.end());
    p->updateMinSubTreeKey(p->right_sibling);
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
            p->parent->keys.erase(p->parent->keys.begin()+update_idx);
        } else {
            if(my_index>update_idx){
                p->parent->keys[update_idx] = p->sub_tree_min_key;
            }
        }
        p->updateMinSubTreeKey(p->parent);
    }
};


template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::balance(NodePointer p,NodePointer child) {
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
        if(!p->parent.isNull()) {
            updateParentKeys(p);
        }
    }

    if (!p->parent.isNull()) {
        balance(p->parent,p);
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
bool BEpsilonTree<Key, Value, B>::insert(NodePointer p,Key key, Value value) {

    //for finding the key position, we'll start with last index
    int ix = p->keys.size() - 1;
    //find the key index.
    while (ix >= 0 && p->keys[ix] > key) {
        ix--;
    }

    if (p->isLeaf) {
        for(Key k: p->keys){
            if(k == key){
                return false;
            }
        }
        p->keys.insert(p->keys.begin() + (ix + 1), key);
        p->values.insert(p->values.begin() + (ix + 1), value);
        p->sub_tree_min_key = p->keys[0];
        insertKeysUpdate(p);
        return true;
    } else {//this is internal node
        ix = ix == -1 ? 0 : ix;
        if (p->keys[ix] < key) {
            ix++;
        }
        if(key < p->sub_tree_min_key) {
            p->sub_tree_min_key = key;
        }
        return insert(p->children[ix],key, value);
    }
};

template<typename Key, typename Value, int B>
bool BEpsilonTree<Key, Value, B>::remove(NodePointer p,Key key) {
    //the position of the key in the node.
    int ix = p->keys.size() - 1;
    while (ix >= 0 && p->keys[ix] > key) {
        ix--;
    }
    if (ix == -1) {
        ix = 0;
    }
    if (p->isLeaf) {
        if (p->keys[ix] == key) {
            p->keys.erase(p->keys.begin() + ix);
            p->values.erase(p->values.begin() + ix);
            balance(p,NodePointer());
            return true;
        }
        return false;
    } else {
        if (p->keys[ix] <= key) {
            ix++;
        }
        return remove(p->children[ix],key);
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
    assert((this->parent.isNull()) || (this->keys.size() < B && this->keys.size() >= B/2));
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
    if (root.isNull()) { // if the Tree is empty
        root = ss->allocate(new Node(true));
        root->keys.insert(root->keys.begin(), key);
        root->values.insert(root->values.begin(), value);
        root->sub_tree_min_key = key;
    } else { //if the root is not null.
        if(insert(root,key, value)){
            size_++;
        }
        if (!root->parent.isNull()) {
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
    if (!root.isNull()) {
        //get appropriate leaf
        NodePointer current = approximateSearch(minKey);
        Value maxFound = minKey;
        bool flag = true;

        while (!current.isNull() && maxKey >= maxFound && flag) {
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
    if (!root.isNull()) {
        if(remove(root,key)) {
            size_--;
        }
        if (root->children.size() == 1) {
            root = root->children[0];
            root->parent = NodePointer();
        }
    }
    //TODO remove when analysis
//    root->RI();
};

template<typename Key, typename Value, int B>
void BEpsilonTree<Key, Value, B>::printTree() {
    if(!root.isNull()) {
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

