#include <string.h>
#include <filesystem>
namespace fs = std::filesystem;

#include "buildfile_parser.cpp"

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString);

int main(int argc, const char** argv) {

	if (argc < 2) {
		printf("usage: p++ requires one (or more) source directory(s)\n");
		return 0;
	}

	std::string compileFlags = "";
	bool hasOutDir = false;

	std::vector<fs::path> rootPaths, excludePaths;

	// If there is a buildfile read it and add g++ flags accordingly
	std::string arg1 = std::string(argv[1]);
	if (arg1.substr(0, 2) == "_b") {

		const char* profile = "";
		if (argc > 2) profile = argv[2];
		
		std::string buildPath = (arg1.size() > 2) ? arg1.substr(2) + ".ppp" : "build.ppp";
		svmap objects = parse_file(buildPath.c_str(), profile);

		fs::path baseDir;
		if (objects.count("cwd") == 1) baseDir = fs::absolute(fs::path(objects["out"][0]));
		else baseDir = fs::path(buildPath).parent_path();

		auto runEachValue = [&](std::string key, auto&& fn){ if (objects.count(key) == 1) for (auto v : objects[key]) fn(v); };

		runEachValue("root", [&](std::string v){ rootPaths.push_back(fs::absolute(baseDir / fs::path(v))); });
		runEachValue("exclude", [&](std::string v){ excludePaths.push_back(fs::absolute(baseDir / fs::path(v))); });

		runEachValue("include", [&](std::string v){ compileFlags += "-I\"" + (baseDir / fs::path(v)).string() + "\" "; });
		runEachValue("libDir", [&](std::string v){ compileFlags += "-L\"" + (baseDir / fs::path(v)).string() + "\" "; });
		runEachValue("lib", [&](std::string v){ compileFlags += "-l" + v + " "; });

		runEachValue("define", [&](std::string v){ compileFlags += "-D" + v + " "; });

		runEachValue("gccflags", [&](std::string v){ compileFlags += v + " "; });

		if (objects.count("out") == 1) {
			compileFlags += "-o \"" + fs::absolute(baseDir / fs::path(objects["out"][0])).string() + "\" ";
			hasOutDir = true;
		}

	// If flags were instead written in the command line add g++ flags based on them
	} else {

		for (int i = 1; i < argc; i++) {
			std::string arg = std::string(argv[i]);

			if (arg.substr(0, 2) == "_e") excludePaths.push_back(fs::path(arg.substr(2)));
			else if (arg.substr(0, 2) == "_r") rootPaths.push_back(fs::path(arg.substr(2)));
			else {
				if (arg.substr(0, 2) == "-o" && arg.size() > 2) {
					hasOutDir = true;
				}

				compileFlags += arg + " ";
			}
		}

	}

	if (rootPaths.size() < 1) {
		printf("usage: p++ requires one (or more) source directory(s)\n");
		return 0;
	}

	if (!hasOutDir) compileFlags += "\"-o" + rootPaths[0].generic_string() + "/a.exe\" ";

	std::string fileString = "";
	
	for (int i = 0; i < rootPaths.size(); i++) {
		if (!fs::exists(rootPaths[i])) {
			fprintf(stderr, "error: source directory '%s' does not exist!\n", rootPaths[i].c_str());
			return 0;
		}

		searchDir(rootPaths[i], excludePaths, fileString);
	}
	
	if(fileString.size() == 0) {
		printf("warning: no c++ files found in any of the source directories. program exiting...\n");
		return 0;
	}

	printf("> Executing command: g++ %s<\n\n", (fileString + compileFlags).c_str());
	system(("g++ " + fileString + compileFlags).c_str());

	return 0;
}

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString) {

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
			searchDir(file.path(), excludePaths, fileString);
		}
	}
}
