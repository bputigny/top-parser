#include "config.h"
#include "EsterBackEnd.h"
#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <cassert>
#include <fstream>

EsterBackEnd::EsterBackEnd(ir::Program& p) : prog(p) {

    // std::ofstream file;
    // file.open("IR.dot");
    // prog.dumpDOT(file);
    // file.close();


    SymTab& symTab = prog.getSymTab();

    // Add Ester internals
    symTab.add(new ir::Function("lap", true));
    symTab.add(new ir::Function("zeros", true));
    symTab.add(new ir::Function("ones", true));
    symTab.add(new ir::Function("diff", true));
    symTab.add(new ir::Function("sin", true));
    symTab.add(new ir::Function("cos", true));
    symTab.add(new ir::Function("pow", true));

    // We need to provide a definition for internal variable
    symTab.add(new ir::Variable("r", new ir::Identifier("S.r"), true));
    symTab.add(new ir::Variable("theta", new ir::Identifier("S.theta"), true));
    
    prog.buildSymTab();
    prog.computeVarSize();

    nonLocEqToBC();

    // file.open("SymTab.dot");
    // prog.getSymTab().dumpDOT(file);
    // file.close();
}

void EsterBackEnd::nonLocEqToBC() {
    for (auto v:prog.getSymTab()) {
        if (v->getDim() != std::vector<int>(1, -1)) {
            // ir::Equation *eq = ;
            // ir::BoundaryCondition *bc = new ir::BoundaryCondition();
            assert(s->getDef());
            // prog.addBC(s->getDef());
        }
    }
}

void EsterBackEnd::emitCode(std::ostream& os) {
    int n = 0;

    os << "// Ester backend code\n";
    os << "void solve_equation(";

    for (auto s:prog.getSymTab()) {
        if (ir::Param *p = dynamic_cast<ir::Param *>(s)) {
            if (n > 0)
                os << ", ";
            n++;
            os << p->getType() << " ";
            os << p->getName();
        }
        // variables are either not defined, or non local equation
        else if (ir::Variable *v = dynamic_cast<ir::Variable *>(s)) {
            if (!s->getDef() || s->getDim() != std::vector<int>(1, -1)) {
                if (n > 0)
                    os << ", ";
                n++;
                os << "matrix *_";
                os << v->getName();
            }
        }
    }
    os << ") {\n";

    os << "\n    Symbolic S;\n";
    for (auto sym:prog.getSymTab()) {
        if (ir::Symbol *s = dynamic_cast<ir::Variable *>(sym)) {
            if (s->getDef() == NULL) {
                os << "    sym " << *s << " = S.regVar(" << *s << ");\n";
            }
        }
    }

    this->emitDecls(os);
    this->emitEquations(os);
    os << "}\n";
}

void EsterBackEnd::emitEquations(std::ostream& os) {
    int n = 0;

    os << "\n";
    os << "    //===== equations =====\n";
    os << "\n";

    for (auto e:prog.getEquations()) {
        ir::Expr *eq = new ir::BinaryExpr(&e->getLHS(),
                new ir::BinaryOperator("-"),
                &e->getRHS());
        ExprAnalysis ea(prog);

        os << "    eq" << n << " = ";
        emitExpr(os, eq);
        os << ";\n\n";

        for (auto v:*ea.getVariables(*eq)) {
            os << "    eq" << n << ".add(&op, \"eq" << n << "\", \"" << v.getName() << "\");\n";
        }
        n++;
        delete eq;
    }

    os << "    // TODO: set RHS!\n";

    os << "\n";
    os << "    //===== end equations =====\n";
    os << "\n";
}

void EsterBackEnd::emitExpr(std::ostream& os, ir::Expr *e) {
    os << *e;
}

void EsterBackEnd::emitDecls(std::ostream& os) {

    os << "\n";
    for (auto d:prog.getDeclarations()) {
        DeclAnalysis *da = new DeclAnalysis(*d);

        if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(&d->getLHS())) {
            ir::Symbol *sym = prog.getSymTab().search(id->getName());

            if (sym == NULL)
                error("%s undefined\n", id->getName().c_str());

            if (ir::Function *func = dynamic_cast<ir::Function *>(sym)) {
                error("%s: cannot define new function (%s)\n",
                        d->getLocation().c_str(),
                        func->getName().c_str());
            }
            else {
                int n = 0;
                assert(dynamic_cast<ir::Array *>(sym) ||
                        dynamic_cast<ir::Variable *>(sym) ||
                        "Symbol should not be instantiated");
                os << "    // " << sym->getName() << " dimension: ";
                for (auto s: sym->getDim()) {
                    if (n++ > 0)
                        os << ", ";
                    os << s;
                }
                os << "\n";
                os << "    sym " << sym->getName() <<
                    " = S.regVar(\"" << sym->getName() << "\");\n";
            }
        }
        else {
            error("%s is not a valid left hand side for a declaration\n");
        }
        // emitExpr(d->getExpr());
        // os << ";\n";
        delete da;
    }
    os << "\n";
    os << "    //===== end declarations =====\n";
    os << "\n";
    // for (auto d:prog.getDeclarations()) {
    //     DeclAnalysis *da = new DeclAnalysis(*d);

    //     if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(&d->getLHS())) {
    //         ir::Symbol *sym = prog.getSymTab().search(id->getName());
    //         if (d->getLHS()) {
    //             os << "    matrix *" << sym->getName() << "_val = new matrix();\n";
    //         }
    //     }
    //     else {
    //         error("'%s' not found in symbol table\n");
    //     }

    //     delete da;
    // }
}


void EsterBackEnd::emitBC(std::ostream& os, ir::BoundaryCondition *bc) {

}

