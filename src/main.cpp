#include <iostream>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#include "buildfile_parser.h"

std::map<char, std::string> cmdKeyToKeyMap = {
    {'r', "root"},
    {'e', "exclude"},
    {'i', "include"},
    {'l', "lib"},
    {'L', "libDir"},
    {'d', "define"},
    {'f', "gccflags"},
    {'c', "cwd"}
};

std::map<std::string, std::function<void(strvec)>> keyFunctions;

void searchDirectory(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString) {
    for (auto file : fs::directory_iterator(root)) {

        bool shouldExclude = false;
        for (auto path : excludePaths) 
            if (fs::equivalent(file.path(), path)) {
                shouldExclude = true;
                break;
            }

        if (shouldExclude) continue;

        if (file.path().extension() == ".cpp" || file.path().extension() == ".cc") {
            fileString += "\"" + file.path().string() + "\" ";
        } else if (fs::is_directory(file.path())) {
            searchDirectory(file.path(), excludePaths, fileString);
        }
    }
}

void build(svmap args, fs::path baseDir) {
    
}

svmap parseArgs(int argc, char const **argv) {
    auto readVal = [&argc, &argv](std::string arg, int &index) -> std::pair<char, std::string> {
        std::string value;
        if (arg.length() > 2)
            value = arg.substr(2);
        else if (index < argc)
            value = argv[++index];

        return {arg.at(1), value};
    };
    
    svmap args;
    std::string currentArg, currentKey;
    strvec currentValues;

    for (int i = 1; i < argc; i++) {
        currentArg = argv[i];
        if (currentArg.substr(0, 1) == "-") {
            if (!currentKey.empty())
                insert(args, currentValues, currentKey);
            
            auto [key, val] = readVal(currentArg, i);
            currentValues.push_back(val);
            currentKey = cmdKeyToKeyMap.at(key);
        } else {
            currentValues.push_back(currentArg);
        }
    }
    return args;
}

void buildWithFile(std::string path, std::string profile = "") {
    std::cout << "building: '" << path << "'";
    if (!profile.empty()) std::cout << " on profile: '" << profile << "'";
    std::cout << "\n";
}

void buildWithArgs(int argc, char const **argv) {
    std::cout << "building via args\n";
    auto args = parseArgs(argc, argv);
    fs::path baseDir;
}

int main(int argc, char const **argv) {
    if (argc < 2)
        buildWithFile("./build.ppp");
    else if (std::string arg1 = argv[1]; arg1.substr(0, 2) == "-b") {
        std::string buildfile = "./build";
        if (arg1.size() > 2) buildfile = arg1.substr(2);
        
        std::string profile = "";
        if (argc > 2) profile = argv[2];

        buildWithFile(buildfile + ".ppp", profile); 
    }
    else buildWithArgs(argc, argv);
    return 0;
}
