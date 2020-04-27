#include <string.h>
#include <filesystem>
namespace fs = std::filesystem;

#include "buildfile_parser.cpp"

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString);

int main(int argc, const char** argv) {

	// Send a usage statement if no arguments are provided
	if (argc < 2) {
		printf("usage: p++ requires one (or more) source directory(s)\n");
		return 0;
	}

	// A string to store the compiler flags
	std::string compileFlags = "";
	// A flag set if there is a specified output directory
	bool hasOutDir = false;

	// Lists of paths to include or exclude from compilation
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

		#define RUN_EACH_VALUE(key) if (objects.count(key) == 1) for (auto v : objects[key])

		RUN_EACH_VALUE("root") rootPaths.push_back(fs::absolute(baseDir / fs::path(v)));
		RUN_EACH_VALUE("exclude") excludePaths.push_back(fs::absolute(baseDir / fs::path(v)));

		RUN_EACH_VALUE("include") compileFlags += "-I\"" + (baseDir / fs::path(v)).string() + "\" ";
		RUN_EACH_VALUE("libDir") compileFlags += "-L\"" + (baseDir / fs::path(v)).string() + "\" ";
		RUN_EACH_VALUE("lib") compileFlags += "-l" + v + " ";

		RUN_EACH_VALUE("define") compileFlags += "-D" + v + " ";

		RUN_EACH_VALUE("g++flags") compileFlags += v + " ";

		if (objects.count("out") == 1) {
			compileFlags += "-o \"" + fs::absolute(baseDir / fs::path(objects["out"][0])).string() + "\" ";
			hasOutDir = true;
		}

	// If flags were instead written in the command line add g++ flags based on them
	} else {

		for (int i = 1; i < argc; i++) {
			std::string arg = std::string(argv[i]);

			// If the arg is '`e' add the following path to the excluded paths list
			if (arg.substr(0, 2) == "_e") excludePaths.push_back(fs::path(arg.substr(2)));
			// If the arg is '`i' add the following path to the included paths list
			else if (arg.substr(0, 2) == "_r") rootPaths.push_back(fs::path(arg.substr(2)));
			// If it's not a p++ flag pass it to g++
			else {
				// Set a flag if the output
				if (arg.substr(0, 2) == "-o" && arg.size() > 2) {
					hasOutDir = true;
				}

				// Add the argument to the list
				compileFlags += arg + " ";
			}
		}

	}

	// Inform the user if no source directories are provided
	if (rootPaths.size() < 1) {
		printf("usage: p++ requires one (or more) source directory(s)\n");
		return 0;
	}

	// If no out dir was specified, set the out to the source root
	if (!hasOutDir) compileFlags += "\"-o" + rootPaths[0].generic_string() + "/a.exe\" ";

	// A string to hold all of the file paths
	std::string fileString = "";
	
	for (int i = 0; i < rootPaths.size(); i++) {
		// Check if the root directory exists
		if (!fs::exists(rootPaths[i])) {
			fprintf(stderr, "error: source directory '%s' does not exist!\n", rootPaths[i].c_str());
			return 0;
		}

		// Search the source directory and add all relivant files to the list
		searchDir(rootPaths[i], excludePaths, fileString);
	}
	
	// Check if there were any c++ files found
	if(fileString.size() == 0) {
		printf("warning: no c++ files found in any of the source directories. program exiting...\n");
		return 0;
	}

	// Report the command to the user
	printf("> Executing command: g++ %s<\n\n", (fileString + compileFlags).c_str());
	// Execute the command
	system(("g++ " + fileString + compileFlags).c_str());

	return 0;
}

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString) {

	// Iterate over all the files in the directory
	for (auto file : fs::directory_iterator(root)) {

		bool shouldExclude = false;
		// Check the current file/folder against the the exclude paths, and skip it if it matches 
		for (auto path : excludePaths) {
			if (fs::equivalent(file.path(), path)) {
				shouldExclude = true;
				break;
			}
		}

		if (shouldExclude) continue;

		// If the file is a c++ file, add it to the list of to be compiled files
		if (file.path().extension() == ".cpp" || file.path().extension() == ".cc") {
			fileString += "\"" + file.path().string() + "\" ";
		// If it's a subdirectory, search it for additional files
		} else if (fs::is_directory(file.path())) {
			searchDir(file.path(), excludePaths, fileString);
		}
	}
}
