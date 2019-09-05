#include <filesystem>
#include <iostream>
#include <string.h>
#include <vector>
namespace fs = std::filesystem;

void usage();

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString);

int main(int argc, char** argv) {

	// Send a usage statement if no arguments are provided
	if (argc < 2) {
		usage();
		return 0;
	}

	// A string to store the compiler flags
	std::string compileFlags = "";
	// A flag set if there is a specified output directory
	bool hasOutDir = false;

	// Lists of paths to include or exclude from compilation
	std::vector<fs::path> includePaths, excludePaths;

	for (int i = 1; i < argc; i++) {
		std::string arg = std::string(argv[i]);

		// If the arg is '`i' add the following path to the included paths list
		if (arg.substr(0, 2) == "`i") includePaths.push_back(fs::path(arg.substr(2)));
		// If the arg is '`e' add the following path to the excluded paths list
		else if (arg.substr(0, 2) == "`e") excludePaths.push_back(fs::path(arg.substr(2)));
		// If the arg is '--libDirOrigin' add a flag for linux that will make the shared object search directory the origin
		else if (arg == "--libDirOrigin") compileFlags += "-Wl,-rpath='${ORIGIN}' ";
		// If it's not a p++ flag pass it to g++
		else {
			// Set a flag if the output
			if (arg.substr(0, 2) == "-o" && arg.size() > 2) {
				hasOutDir = true;
			}

			// Add the argument to the list
			compileFlags += arg;
		}
	}

	// Send a usage statement if no source directories are provided
	if (includePaths.size() < 1) {
		usage();
		return 0;
	}

	// If no out dir was specified, set the out to the source root
	if (!hasOutDir) compileFlags += "\"-o" + includePaths[0].generic_string() + "/a.exe\" ";

	// A string to hold all of the file paths
	std::string fileString = "";
	
	for (int i = 0; i < includePaths.size(); i++) {
		// Check if the root directory exists
		if (!fs::exists(includePaths[i])) {
			std::cerr << "error: source directory '" << includePaths[i].generic_string() << "' does not exist!" << std::endl;
			return 0;
		}

		// Search the source directory and add all relivant files to the list
		searchDir(includePaths[i], excludePaths, fileString);
	}
	
	// Check if there were any c++ files found
	if(fileString.size() == 0) {
		std::cerr << "warning: no c++ files found in any of the source directories. program exiting..." << std::endl;
		return 0;
	}

	// Report the command to the user
	std::cout << "> Executing command: g++ " << fileString << compileFlags << "<" << "\n\n";
	// Execute the command
	system(("g++ " + fileString + compileFlags).c_str());

	return 0;
}

// A simple usage statment to throw is the user forgot to include soure directories
void usage() {
	std::cout << "usage: p++ requires one (or more) source directory(s) with the `i flag [e.g. p++ `ipath/to/your/source/directory]" << std::endl;
}

void searchDir(fs::path root, std::vector<fs::path> excludePaths, std::string& fileString) {

	// Iterate over all the files in the directory
	for (auto file : fs::directory_iterator(root)) {

		bool shouldExclude = false;
		// Check the current file/folder against the the exclude paths, and skip it if it matches 
		for (auto path : excludePaths) {
			if (file.path() == path) {
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
