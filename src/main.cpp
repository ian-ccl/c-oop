#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "translate.hpp"
constexpr inline bool Error = true;
constexpr inline bool Ok = false;

bool program(const std::vector<std::string>& args) {
    info_t info;
    for (const std::string& file : args) {
        std::string code((std::istreambuf_iterator<char>(std::ifstream(file).rdbuf())), std::istreambuf_iterator<char>());
        size_t i = file.rfind(".");
        std::string outfname = file.substr(0,i) + (file.ends_with(".coop") ? ".c" : ".h");
        std::ofstream f(outfname); 
        f << translate(code, &info);
    }
    return Ok;
}

int main(int argc, char** argv) {
    #if INDEBUG
        std::cout << "\033[33m";
    #endif
    if (argc == 1) {
        std::cerr << "use: " << *argv << " files1.coop [file2.coop ...]";
        return 1;
    }

    std::vector<std::string> args;
    for (size_t i=1; i<argc;i++) {
        char* arg = argv[i];
        args.emplace_back(arg);
    }
    return program(args);
}