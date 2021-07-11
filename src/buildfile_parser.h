#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

std::string read_file(std::string filepath) {
    std::ifstream file;
    file.open(filepath);

    if (file.fail()) {
        std::cerr << "file read error: file '" << filepath << "' not found.\n";
        return {};
    }
    
    std::string s, res;
    while (getline(file, s)) res += s + '\n';
    file.close();

    return res;
}

typedef std::vector<std::string> strvec;
typedef std::map<std::string, strvec> svmap;

void insert(svmap &map, std::string const &key, strvec &vec) {
    auto it = map.find(key);
    if (it != map.end()) {
        auto v = map[key];
        v.insert(v.end(), vec.begin(), vec.end());
        it->second = v;
    } else map.insert({key, vec});
}

constexpr bool isUnix() {
    #if defined(unix) || defined(__unix) || defined(__unix__)
    return true;
    #else
    return false;
    #endif
}

constexpr const char *getUnixStr() {
    if constexpr (isUnix()) return "unix";
    return "";
}

constexpr char const *os() {
    #if defined(_WIN32)
    return "windows";
    #elif defined(__linux__)
    return "linux";
    #elif defined(__APPLE__)
    return "osx";
    #else
    return "unknown";
    #endif
}

class Parser {
    std::string m_text;
    size_t m_pos = 0;
    char m_current;

    std::string m_profile;
    bool isCorrectOS = true, isCorrectProfile = true;
    bool isBracket = false;

    inline bool advance() {
        m_pos++;
        return (m_current = (m_pos < m_text.length()) ? m_text[m_pos] : 0);
    }

    inline bool contains(std::string const &source, char c) {
        return source.find(c) != std::string::npos;
    }

    inline std::string makeString(std::string delims = "\"", bool raw = false) {
        advance();
        std::string res;
        while (!contains(delims, m_current)) {
            if (!raw && m_current == '\\') if(!advance()) break;
            res += m_current;
            if(!advance()) break;
        }
        return res;
    }

    inline bool isComment() {
        if(!advance()) return false;
        if (m_current == '/') {
            makeString("\n", true);
            return true;
        }
        else if (m_current == '*') {
            do {
                makeString("*");
                if(!advance()) break;
            } while (m_current != '/');
            return true;
        }
        else return false;
    }

    inline void setProfileOrOS() {
        advance();
        if (m_current == '#') {
            auto currentProfile = makeString("\n"); 
            if (currentProfile == m_profile || currentProfile == "*") isCorrectProfile = true;
            else isCorrectProfile = false;
        }
        else {
            std::string currentOs{m_current};
            currentOs += makeString("\n");
            if (currentOs == os() || currentOs == getUnixStr() || currentOs == "*") isCorrectOS = true;
            else isCorrectOS = false;
        }
    }

public:
    Parser(std::string const &filename, std::string const &profile) : m_text(read_file(filename)), m_current(m_text[0]), m_profile(profile) {}

    inline svmap parse() {
        std::string objKey, tempValue, *currentString = &objKey;
        strvec objValue;
        svmap objMap;

        auto pushValue = [&]() -> void {
            if (!tempValue.empty()) {
                objValue.push_back(tempValue);
                tempValue = "";
            }
            if (!isBracket) {
                insert(objMap, objKey, objValue);
                currentString = &objKey;
                objKey = "";
                objValue = {};
            }
        };

        while (m_current) {
            if (m_current == '#') setProfileOrOS();
            else if (!isCorrectOS || !isCorrectProfile || contains(" \t\n\r", m_current)) {}
            else if (m_current == '/') {
                if (!isComment()) {
                    *currentString += '/';
                    m_pos--;
                }
            }
            else if (m_current == '"') *currentString += makeString();
            else if (m_current == ':') currentString = &tempValue;
            else if (m_current == '[') isBracket = true;
            else if (m_current == ']') isBracket = false;
            else if (m_current == ',') pushValue();
            else {
                *currentString += m_current;
                *currentString += makeString("# \t\n\r/\":[],");
                m_pos--;
            }
            if(!advance()) break;
        }

        pushValue();

        return objMap;
    }
};
