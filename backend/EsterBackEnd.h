#ifndef ESTER_BACKEND_H
#define ESTER_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

class EsterBackEnd : public BackEnd {

    protected:
        ir::Program *prog;

        // real problem variables
        std::vector<std::string> syms;

        // variables introduced for non local equations (for instance, in:
        // phi0 = phi[0], phi0 would be variable because it is depends on phi
        std::vector<std::string> syms_dep;

        ir::EqLst *eqs;

        /// \brief transform non local equations to BC
        void nonLocEqToBC();
        void checkDefs();
        void emitDecls(std::ostream &os);
        void emitEquations(std::ostream &os);
        void emitExpr(std::ostream &os, ir::Expr *e);
        void emitBC(std::ostream &os, ir::BC *bc);
        void emitSetValue(std::ostream &os);
        void emitSolver(std::ostream &os);
        void emitFillOp(std::ostream &os);
        void emitNonLocEqs(std::ostream &os);
        void findVariables();
        bool isVar(std::string);
        std::string eqName(ir::Equation *);

        ir::Expr *diff(ir::Expr *, ir::Identifier *id);
        ir::Expr *diff(ir::Expr *, std::string);

        void writeBC(std::ostream& os, std::string eq,
                ir::Expr *e, bool neg = true);
        void writeAddBCTerm(std::ostream& os, std::string eq,
                ir::Expr *e, bool neg = false);
        void writeAddBCTerm(std::ostream& os, std::string eq,
                ir::Identifier *id, bool neg);

    public:
        EsterBackEnd(ir::Program *p);
        ~EsterBackEnd();
        void emitCode(std::ostream &os);
};

#endif
