#ifndef TOP_BACKEND_H
#define TOP_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

#include <map>

typedef enum {
    AS, ART, ARTT, ATBC, ATTBC
} TermType;

typedef enum {
    FULL, T, TT
} IndexType;

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
        std::string getMatrix(IndexType);
        std::string getMatrixI();
        virtual void emitInitIndex(FortranOutput& o);
};

class TermBC : public Term {
    public:
        std::string varLoc;
        std::string eqLoc;

        TermBC(Term);
        virtual TermType getType();
        virtual void emitInitIndex(FortranOutput& o);
};

class TopBackEnd : public BackEnd {

    private:
        ir::Program *prog;
        std::list<ir::Variable *> vars;
        std::map<std::string, std::list<Term *>> eqs;

        std::vector<ir::Expr *> *splitIntoTerms(ir::Expr *);
        void emitTermI(FortranOutput&, Term *);
        void emitTermI(FortranOutput&, TermBC *);

        void emitTerm(FortranOutput&, Term *);
        void emitTerm(FortranOutput&, TermBC *);

        void emitExpr(ir::Expr *expr, FortranOutput& o,
                int ivar,
                int ieq,
                bool emitLlExpr = false,
                std::string bcLocation = "");

        ir::Identifier *findVar(ir::Expr *e);
        ir::FuncCall *findCoupling(ir::Expr *e);
        std::string findDerivativeOrder(ir::Identifier *);
        int findPower(ir::Expr *);
        ir::Expr *extractLlExpr(ir::Expr *);

        void simplify(ir::Expr *);

        void checkCoupling(ir::Expr *expr);
        void emitUseModel(FortranOutput&);
        void emitInitA(FortranOutput&);
        void emitDecl(FortranOutput&, ir::Decl *,
                std::map<std::string, bool>&,
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

        void buildTermList(std::list<ir::Equation *>);
        void emitDeclRHS(FortranOutput& fo, ir::Expr *expr);

        std::list<ir::Equation *> formatEquations();

        void buildVarList();
        int computeTermIndex(Term *);

        /// this constructs a Term object based on the expression given as
        /// argument
        Term *buildTerm(ir::Expr *);

    public:
        int ivar(std::string);
        int ieq(std::string);
        TopBackEnd(ir::Program *p);
        ~TopBackEnd();
        void emitCode(FortranOutput& of);
        void emitLaTeX(LatexOutput& lo);
};

#endif
