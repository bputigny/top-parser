#ifndef ESTER_BACKEND_H
#define ESTER_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

class TopBackEnd : public BackEnd {

    private:
        ir::Program *prog;
        ir::Expr *splitIntoTerms(ir::Expr *);

        std::vector<ir::Variable *> vars;
        void emitEquation(std::ostream &, ir::Expr *lhs, ir::Expr *rhs);
        void emitExpr(std::ostream&, ir::Expr *, int, bool minus = false);
        void emitTerm(std::ostream&, ir::Expr *, int, bool minus = false);
        void emitCoupling(std::ostream &,
                ir::FuncCall *, ir::Identifier *, int, int, bool minus = false);
        void emitCouplingExpr(std::ostream&, ir::Expr *);
        void emitLLExpr(std::ostream&, ir::Expr *, int);
        void emitScalar(std::ostream&,
                ir::Expr *, bool minus = false);

    public:
        int ivar(std::string);
        TopBackEnd(ir::Program *p);
        ~TopBackEnd();
        void emitCode(std::ostream &os);
};


#endif
