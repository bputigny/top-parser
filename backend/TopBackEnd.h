#ifndef TOP_BACKEND_H
#define TOP_BACKEND_H

#include "config.h"
#include "BackEnd.h"
#include "SymTab.h"

#include <map>

std::string escapeLaTeX(const std::string&);
class LaTeXRenamer : public std::map<std::string, std::string> {
    public:
        LaTeXRenamer() : std::map<std::string, std::string>() { }
        void add(std::string name, std::string new_name) {
            (*this)[name] = new_name;
        }
        std::string name(std::string name) {
            try {
                return this->at(name);
            }
            catch (std::out_of_range&) {
                return escapeLaTeX(name);
            }
        }
};
extern LaTeXRenamer *renamer;

typedef enum {
    CHEB,
    FD
} DerivativeType;

typedef enum {
    AS,
    AR, ASBC,
    ART, ARTT, ATBC, ATTBC
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

class TopBackEnd;
class Term {

    protected:
        TopBackEnd *backend;

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
                int power, std::string der, int ivar, std::string varName,
                TopBackEnd* backend);
        virtual TermType getType();
        std::string getMatrix(IndexType);
        std::string getMatrixI();
        virtual void emitInitIndex(FortranOutput& o);

        TopBackEnd *getBackend();
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

    static const std::map<std::string, ir::Symbol *> internalVariables;

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
        ir::FuncCall *extractAvg(ir::Expr *);
        ir::Expr *extractLlExpr(ir::Expr *);

        void simplify(ir::Expr *);

        void checkCoupling(ir::Expr *expr);
        void emitUseModel(FortranOutput&);
        void emitInitA(FortranOutput&);
        void emitDecl(FortranOutput&, ir::Decl *,
                std::map<std::string, bool>&,
                std::map<std::string, bool>&);

        int nvar;
        int nas;
        int nar;
        int nasbc;
        int nartt;
        int nart;
        int natbc;
        int nattbc;

        int powerMax;

        void buildTermList(std::list<ir::Equation *>);
        void emitDeclRHS(FortranOutput& fo, ir::Expr *expr);

        std::list<ir::Equation *> formatEquations();

        void buildVarList();
        int computeTermIndex(Term *);
        DerivativeType derType;

        /// this constructs a Term object based on the expression given as
        /// argument
        Term *buildTerm(ir::Expr *);

    public:
        const int dim;
        int ivar(std::string);
        int ieq(std::string);
        TopBackEnd(ir::Program *p, DerivativeType, int dim = 2);
        ~TopBackEnd();
        void emitCode(FortranOutput& of);
        void emitLaTeX(LatexOutput& lo, const std::string = "");

        bool isVar(std::string);
        bool isField(std::string);
        bool isParam(std::string);
        bool isScal(std::string);
        bool isDef(std::string);

};

#endif
