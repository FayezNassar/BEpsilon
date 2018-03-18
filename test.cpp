#include <map>
#include <stdlib.h>
#include <fstream>
#include "BEpsilon.h"
#include <ittnotify.h>

void insertTest(int size, int access) {
    __itt_pause();
    BEpsilonTree<int, int> *tree = new BEpsilonTree<int, int>();
    for (int i = 0; i < size; i++) {
        tree->insert(i, i);
    }
    __itt_resume();
    for(int i = 0; i < access; i++) {
        __itt_pause();
        int key = rand()%size;
        __itt_resume();
        assert(tree->contains(key));
    }

    cout << "done" << endl;
}

//--num_of_insertion= --only_insert=bool
int main(int argc, char **argv) {
    __itt_pause();
    insertTest(atoi(argv[1]), atoi(argv[2]));
}