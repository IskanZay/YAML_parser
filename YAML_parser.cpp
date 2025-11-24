#include <iostream>
#include <string>
#include "config_parser.h"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " -f <input_file>" << std::endl;
    std::cout << "Converts configuration files to YAML format" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "-f") {
        printUsage(argv[0]);
        return 1;
    }

    std::string filename = argv[2];
    ConfigParser parser;

    if (parser.parseFile(filename)) {
        parser.outputYAML(std::cout);
        return 0;
    }
    else {
        return 1;
    }
}