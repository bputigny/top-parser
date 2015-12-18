#ifndef SYMTAB_H
#define SYMTAB_H

#include "config.h"
#include "DOT.h"

#include <list>
#include <ostream>

namespace ir{
    class Symbol;
    class Identifier;
}

///
/// Symbol table
//
class SymTab : public std::list<ir::Symbol *>, public DOT {
    public:
        SymTab() { }
        void push_back(ir::Symbol *s);
        void add(ir::Symbol *s);
        ir::Symbol *search(const std::string& id) const;
        ir::Symbol *search(ir::Identifier *) const;
        void dumpDOT(std::ostream& os, std::string title="", bool root=true) const;
};

#endif // SYMTAB_H
