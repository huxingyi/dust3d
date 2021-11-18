#include <stdint.h>
#include <iostream>

#include "fbxdocument.h"

using std::cout;
using std::endl;
using namespace fbx;

int main(int argc, char** argv)
{
    if(argc < 2) {
        std::cout << "Specify file which you want to \"copy\"" << std::endl;
        return 1;
    }

    try {
        fbx::FBXDocument doc;
        std::cout << "Reading " << argv[1] << std::endl;
        doc.read(argv[1]);

        //doc.print();
        std::cout << "Writing test.fbx" << std::endl;
        doc.write("test.fbx");
    } catch(std::string e) {
        std::cout << e << std::endl;
        return 2;
    }

    return 0;
}
