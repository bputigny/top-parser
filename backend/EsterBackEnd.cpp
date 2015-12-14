#include "config.h"
#include "EsterBackEnd.h"
#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <cassert>
#include <fstream>


EsterBackEnd::EsterBackEnd(ir::Program& p) : prog(p) {

    std::ofstream file;

    SymTab *symTab = prog.getSymTab();

    // Add Ester internals
    symTab->add(new ir::Function("lap", true));
    symTab->add(new ir::Function("zeros", true));
    symTab->add(new ir::Function("ones", true));
    symTab->add(new ir::Function("diff", true));
    symTab->add(new ir::Function("sin", true));
    symTab->add(new ir::Function("cos", true));
    symTab->add(new ir::Function("pow", true));
    symTab->add(new ir::Function("sqrt", true));

    // We need to provide a definition for internal variable
    symTab->add(new ir::Variable("r", new ir::Identifier("S.r"), true));
    symTab->add(new ir::Variable("theta", new ir::Identifier("S.theta"), true));

    file.open("IR0.dot");
    prog.dumpDOT(file);
    file.close();
    
    prog.buildSymTab();
    // prog.computeVarSize();

    file.open("SymTab.dot");
    prog.getSymTab()->dumpDOT(file);
    file.close();

    nonLocEqToBC();

    file.open("IR1.dot");
    prog.dumpDOT(file);
    file.close();

}

void EsterBackEnd::nonLocEqToBC() {
#if 0
    SymTab *symTab = prog.getSymTab();

    for (auto v:*symTab) {
        if (v->getDim() != std::vector<int>(1, -1)) {
            std::vector<int> eqSize;
            assert(v->getDef());

            ir::Expr *def = v->getDef();

            std::list<ir::Node *> descendants =
                def->getDescendants<ir::Node>();

            if (ir::Node *topdef = dynamic_cast<ir::Node *>(def))
                descendants.push_back(topdef);

            // Game on: we replace the array expression 
            // with the variable itself
            // ir::Equation *loc = new ir::Equation(new ir::Identifier("r"),
            //         new ir::Value<float>(0));
            std::cout << "VAR " << v->getName() << ": ";
            def->dump(std::cout);
            std::cout << "\n";

            for (auto d:descendants) {
                if (dynamic_cast<ir::ArrayExpr *>(d)) {
                    ir::Identifier *id = new ir::Identifier("NEW ID");

                    // ir::Node **addr = d->getAddr();
                    // *addr = id;

                    // for (auto pc:d->getParent()->getChildren()) {
                    //     if (pc == ae) {
                    //         *pc = ir::Identifier("NEWIDENTIFIER");
                    //     }
                    // }
                    // d = id;
                }
            }
            std::cout << "VAR " << v->getName() << ": ";
            def->dump(std::cout);
            std::cout << "\n";

            // Build the equivalent boundary condition
            // ir::Equation *bcExpr = new ir::Equation(new ir::Identifier(v->getName()),
            //         def);
            // ir::BoundaryCondition *bc = new ir::BoundaryCondition(bcExpr, loc);
            // prog.getBCs().push_back(bc);
        }
    }
#endif
}

void EsterBackEnd::emitCode(std::ostream& os) {
    int n = 0;

    os << "// Ester backend code\n";
    os << "void solve_equation(";

    for (auto s:*prog.getSymTab()) {
        if (ir::Param *p = dynamic_cast<ir::Param *>(s)) {
            if (n > 0)
                os << ", ";
            n++;
            os << p->getType() << " ";
            os << p->getName();
        }
        // variables are either not defined, or non local equation
        else if (ir::Variable *v = dynamic_cast<ir::Variable *>(s)) {
            if (!s->getDef()) { // || s->getDim() != std::vector<int>(1, -1)) {
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
    for (auto sym:*prog.getSymTab()) {
        if (ir::Symbol *s = dynamic_cast<ir::Variable *>(sym)) {
            if (s->getDef() == NULL) {
                os << "    sym " << s->getName() << " = S.regVar(" <<
                    s->getName() << ");\n";
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

    for (auto e:*prog.getEqs()) {
        ir::Expr *eq = new ir::BinExpr(e->getLHS(),
                '-',
                e->getRHS());

        os << "    eq" << n << " = ";
        emitExpr(os, eq);
        os << ";\n\n";

        Analysis<ir::Identifier> ea = Analysis<ir::Identifier>(&prog);
        auto emitVar = [&os, &n] (ir::Identifier *id) -> void {
            os << "    eq" << n << ".add(&op, \"eq" << n << "\", \"" <<
                id->getName() << "\");\n";
        };
        ea.run(emitVar, e);
        // for (auto v:*ea.getVariables(*eq)) {
        //     os << "    eq" << n << ".add(&op, \"eq" << n << "\", \"" << v.getName() << "\");\n";
        // }
        n++;
        delete eq;
    }

    os << "    // TODO: set RHS!\n";

    os << "\n";
    os << "    //===== end equations =====\n";
    os << "\n";
}

void EsterBackEnd::emitExpr(std::ostream& os, ir::Expr *e) {
    if (ir::BinExpr *be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitExpr(os, be->getLeftOp());
        os << *be;
        emitExpr(os, be->getRightOp());
        os << ")";
    }
    else if (ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(e)) {
        int i = 0;
        ir::ExprLst *args = fc->getArgs();
        os << fc->getName() << "(";
        for (auto arg:*args) {
            emitExpr(os, arg);
            i ++;
            if (i < args->size())
                os << ", ";
        }
        os << ")";
        delete(args);
    }
    else if (ir::ArrayExpr *ea = dynamic_cast<ir::ArrayExpr *>(e)) {
        err() << "skipped non implemented ArrayExpr for " << *e << "\n";
    }
    else if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(e)) {
        os << id->getName();
    }
    else if (ir::Value<float> *val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue();
    }
    else if (ir::Value<int> *val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue();
    }
    else {
        err() << "skipped non implemented expression type " << *e << "\n";
    }
}

void EsterBackEnd::emitDecls(std::ostream& os) {

    os << "\n";
    for (auto s: *prog.getSymTab()) {
        if (ir::Expr *def = s->getDef()) {
            os << "    sym " << s->getName() << " = ";
            emitExpr(os, def);
            os << "\n";
        }
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


void EsterBackEnd::emitBC(std::ostream& os, ir::BC *bc) {

}

