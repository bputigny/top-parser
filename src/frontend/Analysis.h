#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "config.h"
#include "IR.h"
#include "SymTab.h"

#include <list>

class DeclAnalysis {
    protected:
        ir::Declaration &decl;

    public:
        DeclAnalysis(ir::Declaration &decl);
        ~DeclAnalysis();
        bool isConst();
        SymTab buildSymTab();
};

class ProgramAnalysis {
    protected:
        ir::Program &prog;

    public:
        ProgramAnalysis(ir::Program &p) : prog(p) { }
        ~ProgramAnalysis();
        void buildSymTab();
        void computeVarSize();
};

class ExprAnalysis {
    protected:
        SymTab &symTab;
    public:
        ExprAnalysis(ir::Program &prog) : symTab(prog.getSymTab()) { }
        ExprAnalysis(SymTab &st) : symTab(st) { }
        std::list<ir::Variable> *getVariables(ir::Expr &expr);
};

#endif // ANALYSIS_H
