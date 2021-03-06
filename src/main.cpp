#include <string.h>
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
    {'c', "cwd"},
    {'o', "out"}
};

std::map<std::string, std::function<void(strvec)>> keyFunctions;

void searchDirectory(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString) {
    for (auto file : fs::directory_iterator(root)) {
        bool shouldExclude = false;
        for (auto path : excludePaths) {
            if (fs::equivalent(file.path(), path)) {
                shouldExclude = true;
                break;
            }
        }
        if (shouldExclude) continue;

        if (file.path().extension() == ".cpp" || file.path().extension() == ".cc") {
            fileString += "\"" + file.path().string() + "\" ";
        } else if (fs::is_directory(file.path())) {
            searchDirectory(file.path(), excludePaths, fileString);
        }
    }
}

bool checkAndCreateDir(fs::path dir) {
    if (!fs::exists(dir)) {        
        try {
            fs::create_directories(dir);
            std::cout << "directory '" << dir.string() << "' created!\n";
            return true;
        } catch (fs::filesystem_error e) {
            std::cout << e.what() << '\n';
            return false;
        }
    }
    return true;
}

int build(svmap args, fs::path baseDir) {
    auto keyExists = [](auto map, auto key) -> bool { return (map.find(key) != map.end()); };

    auto runOnEach = [&](std::string key, auto fn) -> void { if (keyExists(args, key)) for (auto v : args.at(key)) fn(v); };

    if (!keyExists(args, "root")) {
        std::cerr << "usage: p++ requires at least one root source directory.\n";
        return 1;
    }

    if (keyExists(args, "cwd"))
        baseDir = fs::absolute(fs::path(args.at("cwd").at(0)));

    std::vector<fs::path> rootDirs, excludePaths;
    runOnEach("root", [&](auto v) { rootDirs.push_back(baseDir / fs::path(v)); });
    runOnEach("exclude", [&](auto v) { excludePaths.push_back(baseDir / fs::path(v)); });

    std::string compilerFlags = "";
    runOnEach("include", [&](auto v) { compilerFlags += "-I \"" + (baseDir / fs::path(v)).string() + "\" "; });
    
    runOnEach("libDir", [&](auto v) { compilerFlags += "-L \"" + (baseDir / fs::path(v)).string() + "\" "; });
    if constexpr (isUnix()) compilerFlags += "-Wl,-rpath='$ORIGIN' ";
    runOnEach("lib", [&](auto v) { compilerFlags += "-l" + v + " "; });
    
    runOnEach("define", [&](auto v) { compilerFlags += "-D " + v + " "; });
    runOnEach("gccflags", [&](auto v) { compilerFlags += v + " "; });

    if (keyExists(args, "out"))
        compilerFlags += "-o \"" + (baseDir / fs::path(args.at("out").at(0))).string() + "\" ";
    else
        compilerFlags += "-o \"" + baseDir.string() + "/a.exe\" ";

    bool requiredDirsExist = true;
    runOnEach("mkdir", [&](auto v) { if (requiredDirsExist) requiredDirsExist = checkAndCreateDir(baseDir / fs::path(v)); });

    if (!requiredDirsExist) return 1;

    std::string filesString = "";
    for (auto dir : rootDirs) {
        if (!fs::exists(dir)) {
            std::cerr << "error: root directory '" << dir.c_str() << "' does not exist!\n";
            return 1;
        }
        searchDirectory(dir, excludePaths, filesString);
    }

    if (filesString.empty()) {
        std::cerr << "warning: no c++ files found in any of the specified root directories\n";
        return 1;
    }

    std::cout << "\033[33;1mexecuting: g++ " << filesString << compilerFlags << "\033[0m\n";
    auto a = system(("g++ " + filesString + compilerFlags).c_str());
    return 0;
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
            if (!currentKey.empty()) {
                insert(args, currentValues, currentKey);
                currentValues.clear();
            }

            auto [key, val] = readVal(currentArg, i);
            currentValues.push_back(val);
            currentKey = cmdKeyToKeyMap.at(key);
        } else {
            currentValues.push_back(currentArg);
        }
    }
    insert(args, currentValues, currentKey);
    return args;
}

int buildWithArgs(int argc, char const **argv) {
    std::cout << "building via command-line arguments\n";
    auto args = parseArgs(argc, argv);
    return build(args, fs::absolute(fs::path("./")));
}

int buildWithFile(std::string path, std::string profile = "") {
    std::cout << "building: '\033[34;1m" << path << "\033[0m'";
    if (!profile.empty()) std::cout << " on profile: '\033[32;1m" << profile << "\033[0m'";
    std::cout << "\n";
    auto args = parse_file(path, profile);
    return build(args, fs::absolute(fs::path(path)).parent_path());
}

int main(int argc, char const **argv) {
    int exitCode;    
    if (argc < 2)
        exitCode = buildWithFile("./build.ppp");
    else if (std::string arg1 = argv[1]; arg1.substr(0, 2) == "-b" || arg1.substr(0, 2) == "_b") {
        std::string buildfile = "./build";
        if (arg1.size() > 2) buildfile = arg1.substr(2);
        
        std::string profile = "";
        if (argc > 2) profile = argv[2];

        exitCode = buildWithFile(buildfile + ".ppp", profile); 
    } else exitCode = buildWithArgs(argc, argv);

    return exitCode;
}
