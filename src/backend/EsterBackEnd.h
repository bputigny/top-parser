#ifndef ESTER_BACKEND_H
#define ESTER_BACKEND_H

#include "BackEnd.h"
#include "SymTab.h"

class EsterBackEnd : public BackEnd {
    protected:
        ir::Program &prog;
        void emitDecls(std::ostream &os);
        void emitEquations(std::ostream &os);
        void emitExpr(std::ostream &os, ir::Expr *e);
        void emitBC(std::ostream &os, ir::BoundaryCondition *bc);
    public:
        EsterBackEnd(ir::Program &p);
        void emitCode(std::ostream &os);
};

#endif
