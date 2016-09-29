#include "BackEnd.h"

#include <cstring>

void Output::checkLineLen() {
    if (lineLen > 60) {
        os << " &\n\t& ";
        lineLen = strlen("\t& ");
    }
}

Output::Output(std::ostream& os) : os(os) {
    lineLen = 0;
}

Output& Output::operator<<(const char c) {
    checkLineLen();
    if (c == '\n')
        lineLen = 0;
    else
        lineLen += 1;
    os << c;
    return (*this);
}

Output& Output::operator<<(const char *str) {
    checkLineLen();
    if (str[strlen(str)-1] == '\n')
        lineLen = 0;
    else
        lineLen += strlen(str);
    os << str;
    return (*this);
}

Output& Output::operator<<(std::string str) {
    checkLineLen();
    if (str[str.length()-1] == '\n')
        lineLen = 0;
    else
        lineLen += str.length();
    os << str;
    return (*this);
}

Output& Output::operator<<(float fval) {
    checkLineLen();
    lineLen += std::to_string(fval).length();
    os << fval;
    return (*this);
}

Output& Output::operator<<(int ival) {
    checkLineLen();
    lineLen += std::to_string(ival).length();
    os << ival;
    return (*this);
}

Output& Output::operator<<(unsigned long ival) {
    checkLineLen();
    lineLen += std::to_string(ival).length();
    os << ival;
    return (*this);
}

Output::~Output() {
}
