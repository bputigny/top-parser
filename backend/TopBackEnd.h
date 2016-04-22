#ifndef ESTER_BACKEND_H
#define ESTER_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

class TopBackEnd : public BackEnd {

    private:
        ir::Program *prog;
        std::vector<ir::Expr *> *splitIntoTerms(ir::Expr *);
        std::vector<ir::Variable *> vars;
        void emitTerm(std::ostream&, ir::Expr *);
        ir::Identifier *findVar(ir::Expr *e);
        ir::FuncCall *findCoupling(ir::Expr *e);
        int findDerivativeOrder(ir::Identifier *);
        int findPower(ir::Expr *);
        ir::Expr *extractLlExpr(ir::Expr *);
        void emitExpr(std::ostream&, ir::Expr *);
        void emitLVarExpr(std::ostream&, ir::Expr *, int);
        void emitScalarExpr(std::ostream&, ir::Expr *);
        void emitInitA(std::ostream&);

        int nas;
        int nartt;
        int nart;
        int ieq;
        int nvar;

    public:
        int ivar(std::string);
        TopBackEnd(ir::Program *p);
        ~TopBackEnd();
        void emitCode(std::ostream &os);
};


#endif
