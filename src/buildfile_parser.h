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

void insert(svmap &map, strvec &vec, std::string const &key) {
    auto it = map.find(key);
    if (it != map.end()) {
        auto v = map[key];
        v.insert(v.end(), vec.begin(), vec.end());
        it->second = v;
    } else map.insert({key, vec});
}

constexpr char const *isUnix() {
    #if defined(unix) || defined(__unix) || defined(__unix__)
    return "unix";
    #else
    return 0;
    #endif
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

    inline bool advance() {
        m_pos++;
        return m_current = (m_pos < m_text.length()) ? m_text[m_pos] : 0;
    }

    inline bool contains(std::string const &source, char c) {
        return source.find(c) != std::string::npos;
    }

    inline std::string makeString(char delim = '"') {
        std::string res;
        while (m_current != delim) {
            if (m_current == '\\') if(!advance()) break;
            res += m_current;
            if(!advance()) break;
        }
        advance();
        return res;
    }

    inline void setProfileOrOS() {
        advance();
        if (m_current == '#') {
            advance();
            if (m_profile == makeString('\n')) isCorrectProfile = true;
            else isCorrectProfile == false;
        }
        else {
            auto currentOs = makeString('\n');
            if (currentOs == os() || currentOs == isUnix()) isCorrectOS = true;
            else isCorrectOS = false;
        }
    }

public:
    Parser(std::string const &text, std::string const &profile) : m_text(text), m_current(text[0]), m_profile(profile) {}

    inline void parse() {

        std::string currentString;

        while (m_current) {
            if (m_current == '#') setProfileOrOS();
            else if (!isCorrectOS || !isCorrectProfile) advance();
            else if (contains(" \t\n\r", m_current)) advance();
            else if (m_current == '"') {
                advance();
                currentString += makeString();
            }
        }
    }
};

svmap parse_file(std::string filepath, const std::string profile) {

    std::string text = read_file(filepath);

    svmap objMap;

    bool isKey = 1, isStr = 0;
    std::string unixOS = isUnix ? "unix" : "";

    return objMap;
}
