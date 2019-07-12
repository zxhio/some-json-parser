#include "j4on.hh"

#include <iostream>

int main() {
    j4on::J4onParser parser("./test/json/false.json");
    parser.parse();

    j4on::Value v = parser.getRootValue();
    j4on::Literal lit = std::any_cast<j4on::Literal>(v.getAnyValue());

    std::cout << "type: " << j4on::typeToString(v.type()) << ", "
              << "literal: " << lit.getLiteral() << std::endl;
}