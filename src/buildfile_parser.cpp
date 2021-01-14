#include <stdio.h>
#include <fstream>
#include <vector>
#include <map>
#include <regex>

typedef std::vector<std::string> strvec;

strvec split(std::string string) {
	strvec tokens;
	
	// Captures : [ ] , " # space deletes tab and newline
	std::regex e("([:\\[\\],\"# ]|[^:\\[\\],\"# \\t\\n\\r]+)");

	std::regex_iterator<std::string::iterator> rit(string.begin(), string.end(), e), rend;

	while (rit != rend) tokens.push_back(rit++->str());

	return tokens;
}

strvec tokenize_file(const char* filepath, const char* delimiters = " ") {
	std::ifstream file;
	file.open(filepath);

	if (file.fail()) {
		fprintf(stderr, "tokenizer error: file '%s' not found.\n", filepath);
		return {};
	}

	std::stringstream sstr;
	sstr << file.rdbuf();
    file.close();

	strvec tokens = split(sstr.str());

	return tokens;
}

typedef std::map<std::string, strvec> svmap;

void insert(svmap &map, strvec &vec, std::string key) {
	auto it = map.find(key);
	if (it != map.end()) {
		auto v = map[key];
		v.insert(v.end(), vec.begin(), vec.end());
		it->second = v;
	} else map.insert({key, vec});
}


svmap parse_file(const char* filepath, const std::string profile) {

	strvec tokens = tokenize_file(filepath);

	const std::string os =
	#if defined(_WIN32)
	"windows";
	#elif defined(__linux__)
	"linux";
	#elif defined(__APPLE__)
	"osx";
	#endif

	const std::string isUnix = 
	#if defined(unix) || defined(__unix) || defined(__unix__)
	"unix";
    #else
    "";
	#endif

	svmap objMap;

	bool isKey = true, isStr = false, isArr = false;
	/* 0b00 = invaliid, 0b01 = valid, 0b1_ = checking */
	uint8_t osValid = 1, profileValid = 1;

	std::string objKey, tempVal;
	strvec objVec;

	for (auto token : tokens) {
		if(token == "\"") isStr = !isStr;
		else {
			// Check os and profile compatibility
			if (token == "#") {
				if (osValid >= 2) { profileValid = 2; osValid -= 2; }
				else osValid += 2;
			}
			else if (profileValid == 2) {
				if (token == profile || token == "*") profileValid = 1;
				else profileValid = 0;
			}
			else if (osValid >= 2) {
				if (token == os || token == isUnix || token == "*") osValid = 1;
				else osValid = 0;
			}

			// Valid os and profile
			else if (osValid == 1 && profileValid == 1) {
				if (isKey) {
					if (token == ":" && !isStr) isKey = false;
					else if (token != " ") objKey += token;
				} else {
					if (isStr) tempVal += token;
					else if (token == "[") isArr = true;
					else if (token == "]") isArr = false;
					else if (token == ",") {
						if (!tempVal.empty()) {
							objVec.push_back(tempVal);
							tempVal = "";
						}
						if (!isArr) {
							insert(objMap, objVec, objKey);
							objKey = "";
							objVec = {};
							isKey = true;
						}
					} else if (token != " " && token != "\t" && token != "\n")
                        tempVal += token;
				}
			}
		}
	}
	if (!tempVal.empty() || !isArr) {
		objVec.push_back(tempVal);
		insert(objMap, objVec, objKey);
	}

	return objMap;
}
