#include <string.h>
#include <filesystem>
namespace fs = std::filesystem;

#include "buildfile_parser.cpp"

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

	// If there is a buildfile read it and add g++ flags accordingly
	std::string arg1 = argv[1];
	if (arg1.substr(0, 2) == "_b") {
		const char* mode = "";
		if (argc > 2) mode = argv[2];
		svmap objects = parse_file((arg1.size() > 2) ? (arg1.substr(2) + ".ppp").c_str() : "build.ppp", mode);
	
		fs::path baseDir = fs::path(arg1.substr(2)).parent_path();

		for (svmap::iterator it = objects.begin(); it != objects.end(); it++) {
			for (auto value : it->second) {
				fs::path relPath = baseDir / fs::path(value);
				if (it->first == "lib") compileFlags += "-l" + value + " ";
				else if (it->first == "include") compileFlags += "-I\"" + relPath.string() + "\" ";
				else if (it->first == "libDir") compileFlags += "-L\"" + relPath.string() + "\" ";
				else if (it->first == "exclude") excludePaths.push_back(relPath);
				else if (it->first == "root") includePaths.push_back(relPath);
				else if (it->first == "out") {
					compileFlags += "-o\"" + relPath.string() + "\" ";
					hasOutDir = true;
				}
				else if (it->first == "g++flags") compileFlags += value + " ";
			}
		}
	// If flags were instead written in the command line add g++ flags based on them
	} else {

		for (int i = 1; i < argc; i++) {
			std::string arg = std::string(argv[i]);

			// If the arg is '`e' add the following path to the excluded paths list
			if (arg.substr(0, 2) == "_e") excludePaths.push_back(fs::path(arg.substr(2)));
			// If the arg is '`i' add the following path to the included paths list
			else if (arg.substr(0, 2) == "_i") includePaths.push_back(fs::path(arg.substr(2)));
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
			std::cerr << "error: source directory '" << includePaths[i].generic_string() << "' does not exist!\n";
			return 0;
		}

		// Search the source directory and add all relivant files to the list
		searchDir(includePaths[i], excludePaths, fileString);
	}
	
	// Check if there were any c++ files found
	if(fileString.size() == 0) {
		std::cerr << "warning: no c++ files found in any of the source directories. program exiting...\n";
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
	std::cout << "usage: p++ requires one (or more) source directory(s) with the `i flag [e.g. p++ `ipath/to/your/source/directory/]\n";
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
