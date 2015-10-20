#include "SymTab.h"
#include "IR.h"
#include "Printer.h"
#include "config.h"


ir::Symbol *SymTab::search(const std::string &id) {
    for (auto s:*this) {
        if (s->getName() == id) {
            return s;
        }
    }
    return NULL;
}

std::ostream& SymTab::dumpDOT(std::ostream& os) {
    int n = 0;
    os << "digraph SymTab {\n";
    os << "node [shape=Mrecord]\n";

    os << (long)this << " [label=\"";
    for (auto s:*this) {
        if (n++ > 0)
            os << " | ";
        s->dumpDOT(os);
    }
    os << "\"]\n";

    for (auto s:*this) {
        if (s->getDef()) {
            os << (long)this << ":" << (long)s << " -> " << (long)s->getDef() << "\n";
            s->getDef()->dumpDOT(os);
        }
    }

    return os << "}\n";
}

