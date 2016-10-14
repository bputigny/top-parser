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

std::ostream& Printer::operator<<(std::string& str) {
    if (Printer::verbosity >= this->level) {
        os << this->pref << str;
        return this->os;
    }
    return devnull;
}

std::ostream& Printer::operator<<(const char *str) {
    if (Printer::verbosity >= this->level) {
        os << this->pref << str;
        return this->os;
    }
    return devnull;
}

std::ostream& Printer::stream() {
    if (this->level > Printer::verbosity) {
        return devnull;
    }
    return os;
}

Printer err("[error]: ",    std::cerr, 0);
Printer warn("[warning]: ", std::cerr, 1);
Printer log("[log]: ",      std::cerr, 2);
