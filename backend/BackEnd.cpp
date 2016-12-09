#include "BackEnd.h"

#include <cstring>

void FortranOutput::checkLineLen(const std::string& str) {
    if (lineLen + str.length() > 79) {
        os << "&\n& ";
        lineLen = 2;
    }
}

Output::Output(std::ostream& os) : os(os) { }
Output::~Output() { }

FortranOutput::FortranOutput(std::ostream& os) : Output(os) {
    lineLen = 0;
}

LatexOutput::LatexOutput(std::ostream &os) : Output(os) { }
