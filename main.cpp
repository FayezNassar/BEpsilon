#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include "BEpsilon.h"

void printVector(vector<int> vector) {
    for (int i = 0; i < vector.size(); i++) {
        cout << "value: " << vector[i] << " ";
    }
    cout << endl;
}

void insertTest();

void removeLeftToRightTest();

void removeRightToLeftTest();

void removeLeftToRightMassiveTest();

void removeRightToLeftMassiveTest();

void removeRandomMassiveTest();

int getInt(int cnt, int size, int done);

int main() {
//    insertTest();
//    removeLeftToRightTest();
//    removeRightToLeftTest();
//    removeLeftToRightMassiveTest();
//    removeRightToLeftMassiveTest();
//    removeRandomMassiveTest();

    return 0;
}

int progress(int cnt, int size, int done) {
    if (((double) cnt) / ((double) size) >= 0.1) {
        done += cnt;
        double percent = ((double) done / (double) size) * 100;
        cout << percent << "% done..." << endl;
    }
    return done;
}

void bigTest() {
    cout << "entered bigTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;
    map<int, int> map;

    for (int i = 0; i < 3000; i++) {
        bplusTree.insert(i, i);
        map[i] = i;
    }
    cout << "done." << endl;
}

void insertTest() {
    cout << "entered insertTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;

    for (int i = 0; i < 300; i++) {
        bplusTree.insert(i, i);
    }
    for (int i = 0; i < 300; i++) {
        assert(bplusTree.contains(i));
    }
    cout << "done." << endl;
}

void removeLeftToRightTest() {
    cout << "entered removeLeftToRightTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;

    for (int i = 0; i < 800; i++) {
        bplusTree.insert(i, i);
    }

    int i = 0;
    while (bplusTree.size() > 0) {
        bplusTree.remove(i);
        assert(!bplusTree.contains(i));
        i++;
    }
    cout << "done." << endl;
}

void removeRightToLeftTest() {
    cout << "entered removeRightToLeftTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;

    for (int i = 0; i < 800; i++) {
        bplusTree.insert(i, i);
    }

    int i = 800;
    while (bplusTree.size() > 0) {
        bplusTree.remove(i);
        assert(!bplusTree.contains(i));
        i--;
    }
    cout << "done." << endl;
}

void removeLeftToRightMassiveTest() {
    cout << "entered removeLeftToRightMassiveTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;
    int cnt = 0;
    int size = 8000;
    int done = 0;

    cout << "inserting to tree..." << endl;
    for (int i = 0; i < size; i++) {
        bplusTree.insert(i, i);
        cnt++;
        int val = progress(cnt, size, done);
        if (val > done) {
            done = val;
            cnt = 0;
        }
    }
    cout << "done inserting." << endl;
    done = 0;
    cnt = 0;
    cout << "removing from tree..." << endl;

    int i = 0;
    while (bplusTree.size() > 0) {
        bplusTree.remove(i);
        assert(!bplusTree.contains(i));
        cnt++;
        int val = progress(cnt, size, done);
        if (val > done) {
            done = val;
            cnt = 0;
        }
        i++;
    }
    cout << "done removing." << endl;
    cout << "done." << endl;
}

void removeRightToLeftMassiveTest() {
    cout << "entered removeRightToLeftMassiveTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;
    int cnt = 0;
    int size = 8000;
    int done = 0;

    cout << "inserting to tree..." << endl;
    for (int i = 0; i < size; i++) {
        bplusTree.insert(i, i);
        cnt++;
        int val = progress(cnt, size, done);
        if (val > done) {
            done = val;
            cnt = 0;
        }
    }
    cout << "done inserting." << endl;

    done = 0;
    cnt = 0;

    cout << "removing from tree..." << endl;
    int i = size - 1;
    while (bplusTree.size() > 0) {
        bplusTree.remove(i);
        assert(!bplusTree.contains(i));
        cnt++;
        int val = progress(cnt, size, done);
        if (val > done) {
            done = val;
            cnt = 0;
        }
        i--;
    }
    cout << "done removing." << endl;
    cout << "done." << endl;
}


void removeRandomMassiveTest() {
    cout << "entered removeRandomMassiveTest..." << endl;
    BEpsilonTree<int, int, 3> bplusTree;
    int cnt = 0;
    int size = 80000;
    int done = 0;

    cout << "inserting to tree..." << endl;
    for (int i = 0; i < size; i++) {
        bplusTree.insert(i, i);
        cnt++;
        int val = progress(cnt, size, done);
        if (val > done) {
            done = val;
            cnt = 0;
        }
    }
    cout << "done inserting." << endl;

    done = 0;
    cnt = 0;

    cout << "removing from tree..." << endl;
    int to_remove = size;

    while(bplusTree.size()>0 && to_remove >0){
        int key = rand() % bplusTree.size();
        cout << "key to remove: " << key << endl;
        bool inTreeBefore = bplusTree.contains(key);
        cout << "key is in tree? " << inTreeBefore << endl;
        bplusTree.remove(key);
        if(inTreeBefore){
            assert(!bplusTree.contains(key));
        }

        cnt++;
        int val = progress(cnt, size, done);
        if(val>done){
            done=val;
            cnt=0;
        }

        to_remove--;
    }

//    bplusTree.remove(3);
//    bplusTree.remove(17);
//    bplusTree.remove(10);
//    bplusTree.remove(4);
//    bplusTree.remove(14);
//    bplusTree.printTree();
    cout << "done removing." << endl;
    cout << "done." << endl;
}


