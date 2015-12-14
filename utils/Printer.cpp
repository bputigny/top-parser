#include "Printer.h"
#include "config.h"

Printer::Printer(const std::string& str, std::ostream& stream) :
    pref(str), os(stream) { }

std::ostream& Printer::stream() {
    return os;
}

void Printer::print() {
    os << pref;
}

Printer edl_err("[error]: ", std::cerr);
std::ostream& err() {
    edl_err.print();
    return edl_err.stream();
}

Printer edl_warn("[warning]: ", std::cerr);
std::ostream& warn() {
    edl_warn.print();
    return edl_warn.stream();
}

