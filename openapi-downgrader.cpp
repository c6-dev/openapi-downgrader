#include <iostream>
#include <fstream>
#include <string>
#include "Converter.h"

void printHelp() {
    std::cout << "Usage: openapi-downgrader filename_in filename_out\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string filename_in = argv[1];
    std::string filename_out = argv[2];

    if (filename_in.empty() || filename_out.empty()) {
        printHelp();
        return 1;
    }

    Converter converter;
    auto result = converter.Convert(filename_in);

    std::ofstream outFile(filename_out);
    if (outFile.is_open()) {
        outFile << result;
        outFile.close();
        std::cout << "\nConversion successful. Output written to " << filename_out << std::endl;
    }
    else {
        std::cerr << "Failed to open output file: " << filename_out << std::endl;
        return 1;
    }

    return 0;
}