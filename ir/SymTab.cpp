#include "SymTab.h"
#include "IR.h"
#include "Printer.h"
#include "config.h"

#include <cassert>

SymTab::SymTab() { }

SymTab::~SymTab() {
    for (auto s: *this) {
        delete s;
    }
}

void SymTab::push_back(ir::Symbol *s) {
    add(s);
}

void SymTab::add(ir::Symbol *s) {
    assert(s);
    if (this->search(s->name) == NULL)
        std::list<ir::Symbol *>::push_back(s);
    else {
        err << s->name << " already defined\n";
        exit(EXIT_FAILURE);
    }
}

ir::Symbol *SymTab::search(ir::Identifier *id) const {
    assert(id);
    return search(id->name);
}

ir::Symbol *SymTab::search(const std::string& id) const {
    for (auto s:*this) {
        if (s->name == id) {
            return s;
        }
    }
    return NULL;
}

void SymTab::dumpDOT(std::ostream& os, std::string title, bool root) const {
    os << "digraph SymTab {\n";
    os << "node [shape=Mrecord]\n";

    os << (long)this << " [shape=plaintext]\n";
    os << (long)this;
    // os << " [label=<\n<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">\n";
    os << " [label=<\n<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
    os << "<tr>\n";
    for (auto s:*this) {
        os << "<td>\n";
        if (dynamic_cast<ir::Param *>(s)) {
            os << "user";
        }
        else if (dynamic_cast<ir::Function *>(s)) {
            os << "func";
        }
        else if (dynamic_cast<ir::Variable *>(s)) {
            if (s->getDef())
                os << "def";
            else
                os << "var";
        }
        else {
            os << "undef";
        }
        os << "</td>\n";
    }
    os << "</tr>\n";
    os << "<tr>\n";
    for (auto s:*this) {
        os << "<td port=\"" << (long)s << "\">\n";
        os << s->name << "\n";
        os << "</td>\n";
    }
    //  port=\"" << (long)s << "\"
    os << "</tr>\n";
    os << "</table>>];\n";

    for (auto s:*this) {
        if (s->getDef()) {
            os << (long)this << ":" << (long)s << " -> " << (long)s->getDef() << ";\n";
            s->getDef()->dumpDOT(os, title, false);
            os << ";\n";
        }
    }
    os << "}\n";
}
