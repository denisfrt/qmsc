#include "parser.h"
#include "visitor_print.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[])
{
    std::string s = R"(
    msc {
     name:"chart 1"
     instance { name:"instance 1" } //hello
     instance { name:"instance 2" }
     message { name:"message 1" from:"instance 1" to:"instance 2" }
     message { name:"message 1" from:"instance 2" to:"instance 1" }
     text { name:"this is a text" }
     if ("this is a if condition") {
        message { name:"message 1" from:"instance 1" to:"instance 2" }
        message { name:"message 1" from:"instance 2" to:"instance 1" }
        while ("while condition") {
            reference { name:"ref name" xform:"ref xform" }
        }
        text { name:"this is a text" }
     }
     if ("this is a if condition") {
     }
     if ("this is a if condition") {
     }
    })";
    if (argc > 1) {
        std::filesystem::path path(argv[1]);
        std::ifstream f(argv[1], std::ios::in | std::ios::binary);
        const auto sz = std::filesystem::file_size(path);
        s.resize(sz, '\0');
        f.read(s.data(), sz);
    }
    qmsc::MscParser parser;
    if (parser.parse(s)) {
        std::cout << "successful parse" << std::endl;
        qmsc::PrintVisitor pv(std::cout, "\n");
        pv.visit(parser.ast());
    }
    return EXIT_SUCCESS;
}