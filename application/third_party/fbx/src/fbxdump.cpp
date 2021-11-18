#include <stdint.h>
#include <iostream>
#include <string>

#include "fbxdocument.h"
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using namespace fbx;

bool findNode(std::string name, FBXNode where) {
    if(where.getName() == name) {
        where.print();
        return true;
    }
    for(FBXNode n : where.getChildren()) {
        if(findNode(name, n)) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        cerr << "Specify file which you want to dump" << endl;
        return 1;
    }

    try {
        fbx::FBXDocument d;
        d.read(argv[1]);
        if(argc >= 3) {
            for(auto n : d.nodes) {
                if(findNode(argv[2], n)) break;
            }
        } else {
            d.print();
        }

    } catch(string s) {
        cerr << "ERROR: " << s << endl;
        return 2;
    }
    return 0;
}
