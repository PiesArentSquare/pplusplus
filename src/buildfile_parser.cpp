#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <regex>

typedef std::vector<std::string> strvec;

strvec split(std::string string) {
	strvec tokens;
	
	// Captures : [ ] , " # space deletes tab and newline
	std::regex e("([:[\\],\"# ]|[^:[\\],\"# \t\n]+)");

	std::regex_iterator<std::string::iterator> rit(string.begin(), string.end(), e), rend;

	while (rit != rend) tokens.push_back(rit++->str());

	return tokens;
}

strvec tokenize_file(const char* filepath, const char* delimiters = " ") {
	std::ifstream file;
	file.open(filepath);

	// Error check
	if (file.fail()) {
		std::cerr << "tokenizer error: file '" << filepath << "' not found.\n";
		return {};
	}

	std::stringstream sstr;
	sstr << file.rdbuf();

	strvec tokens = split(sstr.str());

	return tokens;
}

typedef std::map<std::string, strvec> svmap;

svmap parse_file(const char* filepath, std::string mode) {

	strvec tokens = tokenize_file(filepath);

	const std::string os =
	#if defined(unix) || defined(__unix) || defined(__unix__)
	"unix";
	#elif defined(_WIN32)
	"windows";
	#elif defined(__linux__)
	"linux";
	#elif defined(__APPLE__)
	"osx";
	#endif
	
	svmap objMap;

	bool isKey = true, isStr = false, isArr = false;
	// 0 = invaliid, 1 = valid, 2 = checking
	uint8_t osValid = 1, modeValid = 1;

	std::string objKey, tempVal;
	strvec objVec;

	for (auto token : tokens) {
		// Check if string
		if(token == "\"") isStr = !isStr;
		else {
			// Check os and mode compatibility
			if (token == "#") {
				if (osValid >= 2) { modeValid = 2; osValid -= 2; }
				else osValid += 2;
			}
			else if (modeValid == 2) {
				if (token == mode || token == "*") modeValid = 1;
				else modeValid = 0;
			}
			else if (osValid >= 2) {
				if (token == os || token == "*") osValid = 1;
				else osValid = 0;
			}

			// Valid os and mdoe
			else if (osValid == 1 && modeValid == 1) {
				if (isKey) {
					if (token == ":" && !isStr) isKey = false;
					else objKey += token;
				} else {
					if (isStr) tempVal += token;
					else if (token == "[") isArr = true;
					else if (token == "]") isArr = false;
					else if (token == ",") {
						objVec.push_back(tempVal);
						tempVal = "";
						if (!isArr) {
							objMap.insert({objKey, objVec});
							objKey = "";
							objVec = {};
							isKey = true;
						}
					} else if (token != " " && token != "\t" && token != "\n") tempVal += token;
				}
			}

		}

	}
	if (!tempVal.empty()) {
		objVec.push_back(tempVal);
		objMap.insert({objKey, objVec});
	}

	return objMap;
}