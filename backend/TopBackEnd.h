#ifndef TOP_BACKEND_H
#define TOP_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

#include <map>

class TopBackEnd : public BackEnd {

    private:
        ir::Program *prog;
        std::vector<ir::Expr *> *splitIntoTerms(ir::Expr *);
        std::vector<ir::Variable *> vars;
        void emitTerm(Output&, ir::Expr *, int);
        void emitBCTerm(Output&, ir::Expr *, int, int, int);
        void emitBC(Output&, ir::BC *, int);
        ir::Identifier *findVar(ir::Expr *e);
        ir::FuncCall *findCoupling(ir::Expr *e);
        std::string findDerivativeOrder(ir::Identifier *);
        void simplify(ir::Expr *);
        int findPower(ir::Expr *);
        ir::Expr *extractLlExpr(ir::Expr *);
        void emitExpr(Output&, ir::Expr *, bool isBC = false, int bcLoc = 0);
        void emitLVarExpr(Output&, ir::Expr *, int);
        void emitScalarExpr(Output&, ir::Expr *);
        void emitScalarBCExpr(Output&, ir::Expr *, int);
        void emitInitA(Output&);
        void emitDecl(Output&, ir::Decl *, std::map<std::string, bool>&,
                std::map<std::string, bool>&);

        bool isVar(std::string);
        bool isField(std::string);
        bool isParam(std::string);
        bool isScal(std::string);
        bool isDef(std::string);

        int nas;
        int nartt;
        int nart;
        int nvar;
        int natbc;
        int nattbc;

    public:
        int ivar(std::string);
        int ieq(std::string);
        TopBackEnd(ir::Program *p);
        ~TopBackEnd();
        void emitCode(Output& of);
};


#endif
