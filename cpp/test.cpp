#include "nextjson.h"

#include <stdlib.h>
#include <iostream>

using namespace nextjson;

int size = 0;

void *operator new(size_t n) {
    size += n;
    return std::malloc(n);
}

int main() {
    nextjson::FileStream input("./json/array.json");
    nextjson::Document doc(input);

    // json::Document doc("[false,true,123,null, \"string\", {\"key\":3.14156}]");
    // json::Document doc("[false]");
    doc.parse();
    doc.format();
    std::cout << "alloc size: " << size << std::endl;
}