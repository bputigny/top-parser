#ifndef TOP_BACKEND_H
#define TOP_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

#include <map>

typedef enum {
    AS, ART, ARTT, ATBC, ATTBC
} TermType;

class LlExpr {
    public:
        int ivar;
        ir::Expr *expr;
        std::string type;
        std::string op;

        LlExpr(int, ir::Expr *);
};

class Term {
    public:
        ir::Expr *expr;
        LlExpr *llExpr;
        ir::Symbol var;
        int power;
        std::string der;
        int ivar;
        int ieq;
        std::string eqName;
        std::string varName;

        int idx;

        Term(ir::Expr *expr, ir::Expr *llTerm, ir::Symbol var,
                int power, std::string der, int ivar, std::string varName);
        virtual TermType getType();
        std::string getMatrix();
};

class TermBC : public Term {
    public:
        std::string varLoc;
        std::string eqLoc;

        TermBC(Term);
        virtual TermType getType();
};

class TopBackEnd : public BackEnd {

    private:
        ir::Program *prog;
        std::list<ir::Variable *> vars;
        std::map<std::string, std::list<Term *>> eqs;
        // std::map<std::string, std::list<TermBC *>> bcs;
        // void emitTerm(Output&, ir::Expr *, int);

        std::vector<ir::Expr *> *splitIntoTerms(ir::Expr *);
        void emitTermI(Output&, Term *);
        void emitTermI(Output&, TermBC *);

        void emitTerm(Output&, Term *);
        void emitTerm(Output&, TermBC *);

        void emitExpr(ir::Expr *expr, Output& o,
                int ivar,
                int ieq,
                std::string bcLocation = "");

        // void emitBCTerm(Output&, ir::Expr *, int, int, int);
        // void emitBC(Output&, ir::BC *, int);
        ir::Identifier *findVar(ir::Expr *e);
        ir::FuncCall *findCoupling(ir::Expr *e);
        std::string findDerivativeOrder(ir::Identifier *);
        void simplify(ir::Expr *);
        int findPower(ir::Expr *);
        ir::Expr *extractLlExpr(ir::Expr *);

        void checkCoupling(ir::Expr *expr);
        // void emitExpr(Output&, ir::Expr *, bool isBC = false, int bcLoc = 0);
        // void emitLVarExpr(Output&, ir::Expr *, int);
        // void emitScalarExpr(Output&, ir::Expr *);
        // void emitScalarBCExpr(Output&, ir::Expr *, int);
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

        int powerMax;

        // std::list<Term> eqTerms;
        // std::list<TermBC> bcTerms;
        void buildTermList(std::list<ir::Equation *>);

        std::list<ir::Equation *> formatEquations();

        void buildVarList();
        int computeTermIndex(Term *);

        /// this constructs a Term object based on the expression given as
        // argument
        Term *buildTerm(ir::Expr *);

    public:
        int ivar(std::string);
        int ieq(std::string);
        TopBackEnd(ir::Program *p);
        ~TopBackEnd();
        void emitCode(Output& of);
};

#endif
