#include "config.h"
#include "EsterBackEnd.h"
#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <algorithm>
#include <cassert>
#include <fstream>

void EsterBackEnd::writeAddBCTerm(std::ostream& os, std::string eq,
        ir::Identifier *id, bool neg) {
    assert(id);
    std::string negOp = "";
    if (neg)
        negOp = "-";
    if (auto fc = dynamic_cast<ir::FuncCall*>(id)) {
        assert(fc->getName() == "D");
        assert(fc->getArgs()->size() == 1);
        if (auto var = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0))) {
            std::string varName = var->getName();
            os << "        op.bc_add_d(\"" << eq << "\", \"" << varName <<
                "\", " << negOp << "ones(val_" <<
                varName << "->ncols(), " << varName << "->nrows()));\n";
        }
        else if (auto be = dynamic_cast<ir::BinExpr *>(fc->getArgs()->at(0))) {
            auto l = dynamic_cast<ir::Identifier *>(be->getLeftOp());
            auto r = dynamic_cast<ir::Identifier *>(be->getRightOp());
            if (! l || ! r) {
                err() << "BC not handled\n";
                exit(EXIT_FAILURE);
            }

            ir::Identifier *theVar = NULL;
            ir::Identifier *theProd = NULL;

            if (isVar(l->getName())) {
                theVar = l;
                theProd = r;
            }
            else if (isVar(r->getName())) {
                theVar = r;
                theProd = l;
            }
            else {
                err() << "BC not handled\n";
                exit(EXIT_FAILURE);
            }
            os << "        op.bc_add_r(\"" << eq << "\", \"" << theVar->getName() <<
                "\", " << negOp << "ones(val_" << theVar->getName() <<
                "->ncols(), " << theVar->getName() << "->nrows()), " << negOp << "val_" <<
                theProd->getName() << ");\n";
        }
    }
    else {
        err() << "unhandled Identifier in BC...\n";
        exit(EXIT_FAILURE);
    }
}

void EsterBackEnd::writeAddBCTerm(std::ostream& os, std::string eq,
        ir::Expr *e, bool neg) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        if (be->getOp() != '*') {
            err() << "I can only add terms! (i.e., products)\n";
            exit(EXIT_FAILURE);
        }
        ir::Expr *l = be->getLeftOp();
        ir::Expr *r = be->getRightOp();
        if (auto *id = dynamic_cast<ir::Identifier *>(be)) {
            if (id->getName() == "D") {
                ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(be);
                assert(fc);
                assert(fc->getArgs()->size() == 1);
                ir::Expr *arg = (*fc->getArgs())[0];
                // if (auto )
            }
        }
    }
    else {
        err() << "I can only add terms! (i.e., products)\n";
        exit(EXIT_FAILURE);
    }
}

void EsterBackEnd::writeBC(std::ostream& os, std::string eq,
        ir::Expr *e, bool neg) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        char op = be->getOp();
        if (op == '+') {
            writeBC(os, eq, be->getLeftOp(), neg);
            writeBC(os, eq, be->getRightOp(), neg);
        }
        else if (op == '-') {
            writeBC(os, eq, be->getLeftOp(), neg);
            writeBC(os, eq, be->getRightOp(), !neg);
        }
        else if (op == '*') {
            writeAddBCTerm(os, eq, e, neg);
        }
        else {
            err() << "unsupported operator in BC: " << op << "\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        char op = ue->getOp();
        if (op == '-') {
            writeBC(os, eq, ue->getExpr(), !neg);
        }
        else {
            err() << "unsupported operator in BC: " << op << "\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (id->getName() == "D") {
            writeAddBCTerm(os, eq, id, neg);
        }
        else {
            err() << "unsupported expression in bc...\n";
            exit(EXIT_FAILURE);
        }
    }
    else {
        err() << "unsupported expression in bc...\n";
        exit(EXIT_FAILURE);
    }
}

ir::Expr *EsterBackEnd::diff(ir::Expr *expr, ir::Identifier *id) {
    assert(id && expr);
    return diff(expr, id->getName());
}

ir::Expr *EsterBackEnd::diff(ir::Expr *expr, std::string var) {
    assert(expr);
    if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(expr)) {
        if (id->getName() == var) {
            return new ir::FuncCall("D", id);
        }
        else {
            if (ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(id)) {
                bool depOnVar = false;
                Analysis<ir::Identifier> a;
                auto checkDeps = [&depOnVar, &var] (ir::Identifier *i) {
                    if (i->getName() == var) {
                        depOnVar = true;
                    }
                };
                for (auto arg: *fc->getArgs()) {
                    a.run(checkDeps, arg);
                }
                if (depOnVar) {
                    return new ir::FuncCall("D", id);
                }
                return NULL;
            }
            else {
                return NULL;
            }
        }
    }
    else if (dynamic_cast<ir::Value<int> *>(expr) ||
            dynamic_cast<ir::Value<float> *>(expr)) {
        return NULL;
    }
    else if (ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(expr)) {
        char op = ue->getOp();
        ir::Expr *d = diff(ue->getExpr(), var);
        if (d)
            return new ir::UnaryExpr(d, op);
        return NULL;
    }
    else if (ir::BinExpr *be = dynamic_cast<ir::BinExpr *>(expr)) {
        char op = be->getOp();
        ir::Expr *dl = diff(be->getLeftOp(), var);
        ir::Expr *dr = diff(be->getRightOp(), var);
        if (op == '+' || op == '-') {
            if (dr && dl) {
                return new ir::BinExpr(dl, op, dr);
            }
            if (dl) {
                return dl;
            }
            if (dr) {
                if (op == '-')
                    return new ir::UnaryExpr(dr, '-');
                else
                    return dr;
            }
            assert(dl == NULL && dr == NULL);
            return NULL;
        }
        else if (op == '*') {
            if (dr && dl) {
                return new ir::BinExpr(
                        new ir::BinExpr(
                            dl,
                            '*',
                            be->getRightOp()),
                        '+',
                        new ir::BinExpr(
                            be->getLeftOp(),
                            '*',
                            dr));
            }
            if (dl) {
                return new ir::BinExpr(
                            dl,
                            '*',
                            be->getRightOp());
            }
            if (dr) {
                return new ir::BinExpr(
                            be->getLeftOp(),
                            '*',
                            dr);
            }
            assert(dl == NULL && dr == NULL);
            return NULL;
        }
        else {
            err() << "diff: don't know how to differentiate operator: " << op << "\n";
            exit(EXIT_FAILURE);
        }
    }
    else {
        err() << "non-handled expression type in diff\n";
        exit(EXIT_FAILURE);
    }

}

EsterBackEnd::EsterBackEnd(ir::Program *p) {

    std::ofstream file;

    prog = p;

    SymTab *symTab = prog->getSymTab();

    // Add Ester internals
    symTab->add(new ir::Function("lap", true));
    symTab->add(new ir::Function("zeros", true));
    symTab->add(new ir::Function("ones", true));
    symTab->add(new ir::Function("diff", true));
    symTab->add(new ir::Function("sin", true));
    symTab->add(new ir::Function("cos", true));
    symTab->add(new ir::Function("pow", true));
    symTab->add(new ir::Function("sqrt", true));
    symTab->add(new ir::Function("Tmean", true));
    symTab->add(new ir::Function("Tpole", true));

    // We need to provide a definition for internal variable
    symTab->add(new ir::Variable("r", new ir::Identifier("S.r"), true));
    symTab->add(new ir::Variable("theta", new ir::Identifier("S.theta"), true));

    log() << "buildSymTab:\n";
    prog->buildSymTab();
    log() << "\n";

    log() << "checkDef:\n";
    checkDefs();
    log() << "\n";

    log() << "findVariables:\n";
    findVariables();
    log() << "\n";
}

EsterBackEnd::~EsterBackEnd() {
}

void EsterBackEnd::checkDefs() {
    for (auto d: *prog->getDecls()) {
        assert(d->getLHS());
        assert(d->getDef());
        ir::Identifier *id = dynamic_cast<ir::Identifier *>(d->getLHS());
        if (!id) {
            err() << "LHS of definition should be identifiers\n";
            exit(EXIT_FAILURE);
        }
        log() << "checking definition of " << id->getName() << ":\n";
        Analysis<ir::ArrayExpr> aeAnal;
        std::list<ir::ArrayExpr *> aExprs;
        auto checkDef = [&aExprs] (ir::ArrayExpr *ae) {
            log() << "    Array: " << ae->getName() << "\n";
            aExprs.push_back(ae);
        };
        aeAnal.run(checkDef, d->getDef());

        // TODO: The correct way to do this would be check that indexes are
        // not different (it is OK to write: a = b[0, 0] + c[0, 0]). 
        // These expressions will be converted to boundary conditions at
        // corrdinated defined by the index => if the indexes of all arrays are
        // the same, it's OK
        if (aExprs.size() > 1) {
            err() << "in definition of `" << id->getName() << "'" <<
                ": \n\tNo more than one array expression allowed in definitions\n";
            exit(EXIT_FAILURE);
        }
    }
}

void EsterBackEnd::findVariables() {
    SymTab *symTab = prog->getSymTab();

    for (auto eq: *prog->getEqs()) {
        Analysis<ir::Expr> a;
        std::function<void (ir::Expr *)> exprAnal =
            [&exprAnal, symTab, this] (ir::Expr *e) {
                assert(e);
                if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(e)) {
                    if (ir::Symbol *s = symTab->search(id)) {
                        if (s->getDef() == NULL) {
                            if (dynamic_cast<ir::Variable *>(s)) {
                                if (!isVar(id->getName()))
                                    syms.push_back(id->getName());
                            }
                        }
                        else {
                            Analysis<ir::ArrayExpr> a;
                            std::list<ir::ArrayExpr *> arrays;
                            auto getArrayExpr = [&arrays, this] (ir::ArrayExpr *ae) {
                                arrays.push_back(ae);
                            };
                            a.run(getArrayExpr, s->getDef());
                            if (!arrays.empty()) {
                                if (!isVar(id->getName()))
                                syms_dep.push_back(id->getName());
                            }
                            exprAnal(s->getDef());
                        }
                    }
                    else {
                        err() << id->getName() << " is undefined\n";
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    for (auto c: e->getChildren()) {
                        assert(dynamic_cast<ir::Expr *>(c));
                        exprAnal(dynamic_cast<ir::Expr *>(c));
                    }
                }
            };
        a.run(exprAnal, eq->getLHS());
        a.run(exprAnal, eq->getRHS());
    }

    for (auto n: syms) {
        log() << n << " is a variable\n";
    }
    for (auto n: syms_dep) {
        log() << n << " is a (dep) variable\n";
    }
}

void EsterBackEnd::nonLocEqToBC() {
}

void EsterBackEnd::emitCode(std::ostream& os) {
    int n = 0;

    os << "// Ester backend code\n";
    os << "void solve_equation(";

    for (auto s:*prog->getSymTab()) {
        if (ir::Param *p = dynamic_cast<ir::Param *>(s)) {
            if (n > 0)
                os << ", ";
            n++;
            os << p->getType() << " ";
            os << p->getName();
        }
        // variables are either not defined, or non local equation
        // else if (ir::Variable *v = dynamic_cast<ir::Variable *>(s)) {
        //     if (!s->getDef()) { // || s->getDim() != std::vector<int>(1, -1)) {
        //         if (n > 0)
        //             os << ", ";
        //         n++;
        //         os << "matrix *val_" << v->getName();
        //     }
        // }
    }
    for (auto s: syms) {
        if (n > 0)
            os << ", ";
        os << "matrix *val_" << s;
        n++;
    }
    for (auto s: syms_dep) {
        if (n > 0)
            os << ", ";
        os << "matrix *val_" << s;
        n++;
    }
    os << ") {\n";

    os << "\n    double tol = 1e-10, error = 1.0;\n";
    os << "\n    int it = 0;\n";
    os << "\n    Symbolic S;\n";

    this->emitDecls(os);

    this->emitEquations(os);
    this->emitSolver(os);
    os << "\n";
    os << "    while (error > tol && it < 10000) {\n";
    this->emitSetValue(os);
    os << "\n";
    this->emitFillOp(os);
    os << "\n";
    this->emitNonLocEqs(os);
    os << "\n";
    for (auto bc: *prog->getBCs()) {
        this->emitBC(os, bc);
    }
    os << "    }\n";
    os << "}\n";
}

bool EsterBackEnd::isVar(std::string name) {
    return std::any_of(syms.begin(), syms.end(),
            [this, name] (std::string s) {return s == name;}) ||
        std::any_of(syms_dep.begin(), syms_dep.end(),
                [this, name] (std::string s) {return s == name;});
}

std::string EsterBackEnd::eqName(ir::Equation *e) {
    std::string eqName = "undef";
    Analysis<ir::Identifier> a;
    std::function<void (ir::Identifier *)> findEqName =
        [&eqName] (ir::Identifier *i) {
            if (!dynamic_cast<ir::FuncCall *>(i)) {
                if (!dynamic_cast<ir::ArrayExpr *>(i)) {
                    if (eqName == "undef") {
                        eqName = i->getName();
                    }
                }
            }
        };
    a.run(findEqName, e->getLHS());
    return eqName;
}

void EsterBackEnd::emitEquations(std::ostream& os) {
    int n = 0;
    SymTab *symTab = prog->getSymTab();

    os << "\n";
    os << "    //=====   equations   =====\n";
    os << "\n";

    for (auto e:*prog->getEqs()) {
        ir::Expr *eq = new ir::BinExpr(e->getLHS(),
                '-',
                e->getRHS());

        os << "    sym " << eqName(e) << " = ";
        emitExpr(os, eq);
        os << ";\n";
        n ++;
    }
    os << "\n";
    os << "    //===== end equations =====\n";
    os << "\n";
}

void EsterBackEnd::emitFillOp(std::ostream& os) {
    int n = 0;
    SymTab *symTab = prog->getSymTab();

    os << "        op.reset();\n";

    for (auto e:*prog->getEqs()) {
        Analysis<ir::Identifier> ea;
        std::vector<std::string> deps;
        std::function<void (ir::Identifier *)> emitVar =
            [&emitVar, &n, &ea, &deps, this] (ir::Identifier *id) -> void {
                if (isVar(id->getName())) {
                    deps.push_back(id->getName());
                }
                else {
                    ir::Symbol *s = prog->getSymTab()->search(id);
                    if (!s->isInternal()) {
                        if (!s) {
                            err() << "undefined symbol: " << id->getName() << "\n";
                            assert(s);
                        }
                        ir::Expr *def = s->getDef();
                        if (def) {
                            ea.run(emitVar, def);
                        }
                    }
                }
            };
        ea.run(emitVar, e);

        // and we always add "r"
        deps.push_back("r");

        // and we remove duplicates
        std::sort(deps.begin(), deps.end());
        deps.erase(std::unique(deps.begin(), deps.end()), deps.end());

        for (auto d: deps)
            os << "        " << eqName(e) << ".add(&op, \"" << eqName(e) << "\", \""
                << d << "\");\n";
        n ++;
        os << "\n";
    }

    os << "        // TODO: set RHS!\n";
}

void EsterBackEnd::emitNonLocEqs(std::ostream& os) {
    SymTab *symTab = prog->getSymTab();

    for (auto s: syms_dep) {
        os << "        // Non Local equation for " << s << "\n";
        ir::Symbol *sym = symTab->search(s);
        assert(sym);
        ir::Expr *def = sym->getDef();
        assert(def);

        int value;
        Analysis<ir::ArrayExpr> a;
        auto findLoc = [&s, &value] (ir::ArrayExpr *e) {
            int n = 0;
            int idx = 0;
            int nIdx = 0;
            for (auto i: e->getChildren()) {
                ir::IndexRange *range = dynamic_cast<ir::IndexRange *>(i);
                assert(range);
                ir::Expr *lb = range->getLB();
                ir::Expr *ub = range->getUB();
                assert(lb && ub);
                ir::Value<int> *ilb = dynamic_cast<ir::Value<int> *>(lb);
                ir::Value<int> *iub = dynamic_cast<ir::Value<int> *>(ub);
                if (iub == NULL || ilb == NULL) {
                    err() << "Only integer range are currently implemented\n";
                    exit(EXIT_FAILURE);
                }
                if (ub == lb) {
                    value = iub->getValue();
                    nIdx ++;
                    idx = n;
                }
                n ++;
            }
            if (nIdx != 1) {
                err() << "setting BC for " << s << " failed (fixed indices: " <<
                    nIdx << ")\n";
                exit(EXIT_FAILURE);
            }
            if (idx != 0) {
                err() << "setting BC for " << s <<
                    " failed (I dont't know how to set BC on theta...)\n";
                exit(EXIT_FAILURE);
            }
        };
        a.run(findLoc, def);
        ir::Equation *loc = new ir::Equation(
                new ir::Identifier("r"),
                new ir::Value<int>(value));

        ir::Equation *cond = new ir::Equation(
                new ir::BinExpr(
                    new ir::Identifier(s),
                    '-',
                    dynamic_cast<ir::Expr *>(def->copy())),
                new ir::Value<float>(0));
        cond->setParents();

        Analysis<ir::ArrayExpr> replace;
        auto replaceArray = [] (ir::ArrayExpr *ae) {
            ir::Identifier *id = new ir::Identifier(ae->getName());
            assert(ae->getParent());
            ae->getParent()->replace(ae, id);
            ae->clear();
            delete(ae);
        };
        replace.run(replaceArray, cond);

        ir::BC *bc = new ir::BC(cond, loc);

        emitBC(os, bc);

        bc->clear();
        delete(bc);
    }
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
    os << "    //=====   declarations   =====\n\n";
    for (auto s: syms) {
        os << "    sym sym_" << s << " = S.regvar(\"" << s << "\");\n";
    }
    for (auto s: syms_dep) {
        os << "    sym sym_" << s << " = S.regvar(\"" << s << "\");\n";
    }
    os << "\n";
    os << "    //===== end declarations =====\n";
    os << "\n";
}

void EsterBackEnd::emitBC(std::ostream& os, ir::BC *bc) {
    std::string bcType = "";
    std::string eq_name = eqName(bc->getCond());
    if (ir::Value<int> *l = dynamic_cast<ir::Value<int> *>(bc->getLoc()->getRHS())) {
        if (l->getValue() == 1) {
            bcType = "bc_bot1";
        }
        else {
            bcType = "bc_bot2";
        }
        Analysis<ir::FuncCall> a0;
        Analysis<ir::Expr> a1;

        // set RHS to zero
        ir::Equation *cond = NULL;
        if (auto *v = dynamic_cast<ir::Value<float> *>(bc->getCond()->getRHS())) {
            if (v->getValue() == 0) {
                cond = bc->getCond();
            }
        }
        if (auto *v = dynamic_cast<ir::Value<int> *>(bc->getCond()->getRHS())) {
            if (v->getValue() == 0) {
                cond = bc->getCond();
            }
        }
        if (cond == NULL) {
            cond = new ir::Equation(
                    new ir::BinExpr(
                        bc->getCond()->getLHS(),
                        '-',
                        bc->getCond()->getRHS()),
                    new ir::Value<float>(0));
        }

        // replace Tmean, Tpole... functions by the multiplication
        auto opToMult = [] (ir::FuncCall *fc) {
            assert(fc);
            if (fc->getName() == "Tmean" ||
                    fc->getName() == "Tpole" ||
                    fc->getName() == "Teq") {
                ir::ExprLst *args = fc->getArgs();
                assert(args);
                if (fc->getArgs()->size() != 1) {
                    err() << *fc << " should only have one argument\n";
                    exit(EXIT_FAILURE);
                }
                ir::BinExpr *mult = new ir::BinExpr(
                        (*args)[0],
                        '*',
                        new ir::Identifier(fc->getName()));
                assert(fc->getParent());
                fc->getParent()->replace(fc, mult);
                // fc->clear();
                // delete(fc);
            }
            if (fc->getName() == "diff") {
                ir::ExprLst *args = fc->getArgs();
                assert(args);
                if (fc->getArgs()->size() != 2) {
                    err() << *fc << " should only have two arguments\n";
                    exit(EXIT_FAILURE);
                }
                if (auto r = dynamic_cast<ir::Identifier *>((*args)[1])) {
                    if (r->getName() == "r") {
                        fc->getParent()->replace(fc, 
                                new ir::BinExpr(
                                    new ir::Identifier("map.R"),
                                    '*',
                                    (*args)[0]));
                    }
                }
            }

        };

        ir::Expr *diffExpr = cond->getLHS();
        ir::Expr *d = NULL;
        for (auto v: syms) {
            ir::Expr *dv = diff(diffExpr, v);
            if (dv) {
                if (d) {
                    d = new ir::BinExpr(dynamic_cast<ir::Expr *>(d->copy()),
                            '+',
                            dynamic_cast<ir::Expr *>(dv->copy()));
                }
                else
                    d = dv;
            }
        }
        for (auto v: syms_dep) {
            ir::Expr *dv = diff(diffExpr, v);
            if (dv) {
                if (d) {
                    d = new ir::BinExpr(dynamic_cast<ir::Expr *>(d->copy()),
                            '+',
                            dynamic_cast<ir::Expr *>(dv->copy()));
                }
                else
                    d = dv;
            }
        }

        if (d) {
            // diffExpr->display("expr");
            // d->display("D expr");
            d->setParents();
            a0.run(opToMult, d);
            // d->display("D expr (with mul op)");
            writeBC(os, eq_name, d);
            // d->display("BC");
        }
        // mult(cond->getLHS(), new ir::Identifier("x"))->display();
        if (cond != bc->getCond()) {
            cond->clear();
            delete(cond);
        }
    }
    else {
        os << "I don't know how to set this BC (location not too complex)...\n";
        exit(EXIT_FAILURE);
    }
}

void EsterBackEnd::emitSetValue(std::ostream& os) {
    for (auto v: syms) {
        os << "        S.set_value(\"sym_" << v << "\", val_" << v << ");\n";
    }
    for (auto v: syms_dep) {
        os << "        S.set_value(\"sym_" << v << "\", val_" << v << ");\n";
    }
    os << "        S.set_map(map);\n";
}

void EsterBackEnd::emitSolver(std::ostream& os) {
    os << "    solver op;\n";
    os << "    op.init(2, " <<  syms.size() + 5 << ", \"full\");\n\n";
    for (auto v: syms) {
        os << "    op.regvar(\"" << v << "\");\n";
    }
    for (auto v: syms_dep) {
        os << "    op.regvar(\"" << v << "\");\n";
    }
    os << "    op.regvar_dep(\"r\");\n";
    os << "    op.regvar(\"eta\");\n";
    os << "    op.regvar(\"deta\");\n";
    os << "    op.regvar(\"R\");\n";
    os << "    op.regvar(\"dR\");\n";
    os << "    op.set_nr(map.npts);\n";
}

