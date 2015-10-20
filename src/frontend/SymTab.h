#ifndef SYMTAB_H
#define SYMTAB_H

#include "IR.h"
#include "Printer.h"

#include <list>

///
/// Symbol table
//
class SymTab : public std::list<ir::Symbol *>, DOT {
    public:
        SymTab() { }
        void push_back(ir::Symbol *s) {
            add(s);
        }
        void add(ir::Symbol *s) {
            if (this->search(s->getName()) == NULL)
                    std::list<ir::Symbol *>::push_back(s);
            else
                error("'%s' already defined\n", s->getName().c_str());
        }
        ir::Symbol *search(const std::string &id);
        std::ostream& dumpDOT(std::ostream& os);
};

#endif // SYMTAB_H
