#include "Printer.h"
#include "config.h"

int Printer::verbosity = 0;

Printer::Printer(const std::string& str, std::ostream& stream, int level) :
    pref(str), os(stream) {
        devnull.open("/dev/null");
        this->level = level;
}

Printer::~Printer() {
    devnull.close();
}

void Printer::init(int v) {
    Printer::verbosity = v;
}

std::ostream& Printer::stream() {
    if (this->level > Printer::verbosity) {
        return devnull;
    }
    return os;
}

void Printer::print() {
    stream() << pref;
}

Printer edl_err("[error]: ", std::cerr, 0);
std::ostream& err() {
    edl_err.print();
    return edl_err.stream();
}

Printer edl_warn("[warning]: ", std::cerr, 1);
std::ostream& warn() {
    edl_warn.print();
    return edl_warn.stream();
}

Printer edl_log("[log]: ", std::cerr, 2);
std::ostream& log() {
    edl_log.print();
    return edl_log.stream();
}
