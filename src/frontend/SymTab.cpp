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
    os << "digraph SymTab {\n";
    os << "node [shape=Mrecord]\n";

    os << (long)this << " [shape=plaintext]\n";
    os << (long)this << " [label=<\n<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">\n";
    os << "<tr>\n";
    for (auto s:*this) {
        os << "<td>\n";
        os << "<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">\n";
        s->dumpDOT(os);
        os << "</table>\n";
        os << "</td>\n";
    }
    os << "</tr>\n";
    os << "</table>>];\n";

    for (auto s:*this) {
        if (s->getDef()) {
            os << (long)this << ":" << (long)s << " -> " << (long)s->getDef() << ";\n";
            s->getDef()->dumpDOT(os) << ";\n";
        }
    }

    return os << "}\n";
}

