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

void SymTab::print() {
    std::cout << "-=SymTab=-\n";
    for (auto s:*this) {
        if (dynamic_cast<ir::Function *>(s)) {
            std::cout << "(Function):\t";
        }
        else if (dynamic_cast<ir::Array *>(s)) {
            std::cout << "(Array):\t";
        }
        else if (dynamic_cast<ir::Variable *>(s)) {
            std::cout << "(Variable):\t";
        }
        else if (ir::Param *p = dynamic_cast<ir::Param *>(s)) {
            std::cout << "(input " << p->getType().c_str() << "):\t";
        }
        else {
            std::cout << "(Unknown):\t";
        }
        std::cout << " " << s->getName() << "\n";
    }
    std::cout << "-========-\n";
}
