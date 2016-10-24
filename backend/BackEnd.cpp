#include "BackEnd.h"

#include <cstring>

void FortranOutput::checkLineLen() {
#if 1
    if (lineLen > 65) {
        os << "&\n&";
        lineLen = 1;
    }
#endif
}

Output::Output(std::ostream& os) : os(os) { }
Output::~Output() { }

FortranOutput::FortranOutput(std::ostream& os) : Output(os) {
    lineLen = 0;
}

LatexOutput::LatexOutput(std::ostream &os) : Output(os) { }
