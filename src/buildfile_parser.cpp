#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <regex>

typedef std::vector<std::string> strvec;

strvec split(std::string string) {
	strvec tokens;
	
	// Captures : [ ] , " space deletes tab and newline
	std::regex e("([:[\\],\" ]|[^:[\\],\" \t\n]+)");

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

svmap parse_tokens(strvec tokens) {
	
	svmap objMap;

	bool isKey = true, isStr = false, isArr = false;

	std::string objKey, tempVal;
	strvec objVec;

	for (auto token : tokens) {
		if(token == "\"") isStr = !isStr; 
		else {
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
	objVec.push_back(tempVal);
	objMap.insert({objKey, objVec});

	return objMap;
}

#ifndef PPP_MAIN_FILE
int main() {
	strvec tokens = tokenize_file("./test/build.ppp");
	svmap objects = parse_tokens(tokens);

	svmap::iterator it;
	for (it = objects.begin(); it != objects.end(); it++) {
		std::cout << "k: \"" << it->first << "\", v: [";
		for (auto x : it->second) std::cout << "\"" << x << "\", ";
		std::cout << "]\n";
	}
}
#endif