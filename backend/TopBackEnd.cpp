#ifndef TOP_BACKEND_H
#define TOP_BACKEND_H

#include "TopBackEnd.h"
#include "Printer.h"
#include "Analysis.h"

#include <cassert>

TopBackEnd::TopBackEnd(ir::Program *p) {
    this->prog = p;
}

TopBackEnd::~TopBackEnd() {
}

#define unsupported(e) { \
    err() << __FILE__ << ":" << __LINE__ << " unsupported expr\n"; \
    e->display("unsupported expr:"); \
    exit(EXIT_FAILURE); \
}

void TopBackEnd::emitCouplingExpr(std::ostream& os, ir::Expr *e) {
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        ir::Expr *lhs = be->getLeftOp();
        ir::Expr *rhs = be->getRightOp();
        this->emitCouplingExpr(os, lhs);
        if (be->getOp() == '^')
            os << "**";
        else
            os << be->getOp();
        this->emitCouplingExpr(os, rhs);
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() == '-') {
            os << "(-";
            this->emitCouplingExpr(os, ue->getExpr());
            os << ")";
        }
        else if (ue->getOp() == '\'') {
            if (auto id = dynamic_cast<ir::Identifier *>(ue->getExpr())) {
                if (id->getName() == "l") {
                    // warn() << "gotta add l\' terms...\n";
                }
                else {
                    err() << "radial derivative terms not allowed " <<
                        "in coupling coefficients\n";
                    exit(EXIT_FAILURE);
                }
            }
            else {
                err() << "unsupported term in coupling coefficient\n";
                exit(EXIT_FAILURE);
            }
        }
        else {
            err() << "unsupported unary operator: \"" << ue->getOp() << "\"\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        os << id->getName();
    }
    else if (auto v = dynamic_cast<ir::Value<int> *>(e)) {
        int val = v->getValue();
        os << val;
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(e)) {
        float val = v->getValue();
        os << val;
    }
    else {
        err() << "unsupported expression in coupling coefficient\n";
        e->display();
        exit(EXIT_FAILURE);
    }
}

bool llTerms(ir::Expr *e) {
    std::vector<ir::Identifier *> ids = getIds(e);
    for (auto i: ids) {
        if (i->getName() == "l")
            return true;
    }
    return false;
}

bool sumSubTerms(ir::Expr *e) {
    Analysis<ir::BinExpr> a;
    bool ret = false;
    auto checkSumSub = [&ret] (ir::BinExpr *be) {
        if (be->getOp() == '+' || be->getOp() == '-') {
            ret = true;
        }
    };
    a.run(checkSumSub, e);
    return ret;
}


ir::Expr *extract(ir::Expr *e) {
    std::vector<ir::Identifier *> ids = getIds(e);
    if (ids.size() == 1) {
        if (ids[0]->getName() == "l") {
            return e;
        }
    }
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        if (llTerms(be->getLeftOp()) && !llTerms(be->getRightOp())) {
            return extract(be->getLeftOp());
        }
        if (!llTerms(be->getLeftOp()) && llTerms(be->getRightOp())) {
            return extract(be->getRightOp());
        }
        if (llTerms(be->getLeftOp()) && llTerms(be->getRightOp())) {
            warn() << "Could not extract ll terms\n";
            return NULL;
        }
    }
    return NULL;
}

void TopBackEnd::emitLLExpr(std::ostream& os, ir::Expr *e, int ivar) {
    ir::Expr *expr = dynamic_cast<ir::Expr *>(e->copy());
    assert(expr);

    expr->setParents();

    auto replaceL = [] (ir::Identifier *id) {
        if (id->getName() == "l") {
            id->getParent()->replace(id, new ir::Identifier("j"));
        }
    };
    auto replaceLp = [] (ir::UnaryExpr *ue) {
        if (ue->getOp() == '\'') {
            if (auto id = dynamic_cast<ir::Identifier *>(ue->getExpr())) {
                if (id->getName() == "l")
                    ue->getParent()->replace(ue, new ir::Identifier("jj"));
            }
        }
    };

    Analysis<ir::UnaryExpr> alp;
    alp.run(replaceLp, expr);

    Analysis<ir::Identifier> al;
    al.run(replaceL, expr);

    std::function<void(ir::Expr *)> emitExpr =
        [&os, &emitExpr, &ivar] (ir::Expr *expr) {
        if (auto id = dynamic_cast<ir::Identifier *>(expr)) {
            if (id->getName() != "j" && id->getName() != "jj") {
                err() << "Only l and l\' terms allowed in `ll expr\'\n";
                expr->display();
                exit(EXIT_FAILURE);
            }
            os << "dm(1)%lvar(" << id->getName()<< ", " << ivar << ")";
        }
        else if (auto be = dynamic_cast<ir::BinExpr *>(expr)) {
            os << "(";
            emitExpr(be->getLeftOp());
            os << be->getOp();
            emitExpr(be->getRightOp());
            os << ")";
        }
        else if (auto v = dynamic_cast<ir::Value<int> *>(expr)) {
            os << v->getValue();
        }
        else {
            err() << "unsupported term in ll expr\n";
            expr->display();
            exit(EXIT_FAILURE);
        }
    };
    os << "dble(";
    emitExpr(expr);
    os << ")\n";

    delete(expr);
}

void TopBackEnd::emitCoupling(std::ostream& os, ir::FuncCall *coef,
        ir::Identifier *id, int derOrder, int ieq, bool minus) {
    static int iartt = 1;
    int ivar = this->ivar(id->getName());
    ir::Expr *llExpr = NULL;

    os << "\tcall " << coef->getName() << "(";
    if (minus)
        os << "-(";
    assert(coef->getArgs()->size() == 1);

    coef->setParents();

    // check if l and l' terms are involved
    ir::Expr *expr = coef->getArgs()->at(0);
    if (llTerms(expr)) {
        llExpr = extract(expr);
        assert(llExpr);

        llExpr->getParent()->replace(llExpr, new ir::Value<float>(1.0));

        if (sumSubTerms(expr)) {
            err() << "No + or - terms allowed when l or l\' terms are involved\n";
            expr->display();
            exit(EXIT_FAILURE);
        }
        this->emitCouplingExpr(os, expr);
    }
    else {
        this->emitCouplingExpr(os, expr);
    }

    if (minus)
        os << ")";

    os << ",\t&\n";
    os << "\t\tdm(1)\%artt(1:grd(1)\%nr, 1:nt, 1:nt, "<< iartt << "), \t&\n";
    os << "\t\tdm(1)\%leq(1:nt, " << ieq << "),\t&\n";
    os << "\t\tdm(1)\%lvar(1:nt, " << ivar << "))\n";

    if (llExpr) {
        bool l = false, lp = false;
        Analysis<ir::Identifier> a;
        auto checkll = [&l, &lp] (ir::Identifier *id) {
            if (id->getName() == "l") {
                if (auto ue = dynamic_cast<ir::UnaryExpr *>(id->getParent())) {
                    if (ue->getOp() == '\'' && id->getName() == "l")
                        lp = true;
                }
                else {
                    l = true;
                }
            }
        };
        a.run(checkll, llExpr);

        if (l) {
            os << "\tdo j=1, nt\n";
        }
        if (lp) {
            if (l) os << "\t";
            os << "\tdo jj=1, nt\n";
        }
        if (l) os << "\t";
        if (lp) os << "\t";
        os << "\tdm(1)\%artt(1:grd(1)\%nr, ";
        if (l)
            os << "j, ";
        else
            os << "1:nt, ";
        if (lp)
            os << "jj, ";
        else
            os << "1:nt, ";
        os << iartt << ") = &\n";
        if (l) os << "\t";
        if (lp) os << "\t";
        os << "\t\tdm(1)\%artt(1:grd(1)\%nr, ";
        if (l)
            os << "j, ";
        else
            os << "1:nt, ";
        if (lp)
            os << "jj, ";
        else
            os << "1:nt, ";
        os << iartt << " * &\n";
        if (l) os << "\t";
        if (lp) os << "\t";
        os << "\t\t";
        this->emitLLExpr(os, llExpr, ivar);
        if (lp) {
            if (l) os << "\t";
            os << "\tend do\n";
        }
        if (l) {
            os << "\tend do\n";
        }
    }
    os << "\n";

    iartt++;
}

void TopBackEnd::emitScalar(std::ostream& os,
        ir::Expr *e, bool minus) {
    static int ias = 1;

    // lambda*b => 1
    // b => 1
    // 2*b => 1

    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        if (be->getOp() == '+') {
            this->emitScalar(os, be->getLeftOp(), minus);
            this->emitScalar(os, be->getRightOp(), minus);
        }
        else if (be->getOp() == '-') {
            this->emitScalar(os, be->getLeftOp(), minus);
            this->emitScalar(os, be->getRightOp(), !minus);
        }
        else if (be->getOp() == '*') {
            if (auto id1 = dynamic_cast<ir::Identifier *>(be->getLeftOp())) {
                if (auto id2 = dynamic_cast<ir::Identifier *>(be->getRightOp())) {
                    if (id1->getName() == "\\") {
                        ias++;
                    }
                    int ivar = this->ivar(id2->getName());
                    int ieq = 1;
                    int der = 1;
                    int power = 1;
                    os << "\tdm(1)\%as(" << ias << ") = \n\n";
                    os << "\tdm(1)\%asi(" << ias << ", 1) = " << power << " ! power\n";
                    os << "\tdm(1)\%asi(" << ias << ", 2) = " << der << " ! der\n";
                    os << "\tdm(1)\%asi(" << ias << ", 3) = " << ieq << " ! ieq\n";
                    os << "\tdm(1)\%asi(" << ias << ", 4) = " << ivar << " ! ivar\n";
                }
                else {
                    unsupported(e);
                }
            }
            else {
                unsupported(e);
            }
        }
        else {
            unsupported(e);
        }
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitTerm(std::ostream& os, ir::Expr *e, int ieq, bool minus) {
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        ir::Expr *lhs = be->getLeftOp();
        ir::Expr *rhs = be->getRightOp();
        if (be->getOp() == '*') {
            if (auto fc = dynamic_cast<ir::FuncCall *>(lhs)) {
                if (auto id = dynamic_cast<ir::Identifier *>(rhs)) {
                    this->emitCoupling(os, fc, id, 0, ieq, minus);
                }
                else if (auto ue = dynamic_cast<ir::UnaryExpr *>(rhs)) {
                    if (ue->getOp() == '\'') {
                        if (auto id = dynamic_cast<ir::Identifier *>(ue->getExpr())) {
                            this->emitCoupling(os, fc, id, 1, ieq, minus);
                        }
                        else {
                            err() << "unsupported coupling coefficient\n";
                            e->display();
                            exit(EXIT_FAILURE);
                        }
                    }
                    else {
                        err() << "unsupported coupling coefficient " <<
                            "(only '\'' UnaryOperator allowed)\n";
                        e->display();
                        exit(EXIT_FAILURE);
                    }
                }
                else {
                    err() << "unsupported coupling coefficient (rhs should be an " <<
                        "Identifier or a Identifier derivative ('\''))\n";
                    e->display();
                    exit(EXIT_FAILURE);
                }
            }
            else if (auto id1 = dynamic_cast<ir::Identifier *>(lhs)) {
                if (auto id2 = dynamic_cast<ir::Identifier *>(rhs)) {
                    this->emitScalar(os, e, minus);
                }
            }
            else {
                err() << "unsupported coupling coefficient\n";
                e->display();
                exit(EXIT_FAILURE);
            }
        }
        else {
            err() << "unsupported coupling coefficient (Terms should be '*' operator)\n";
            e->display();
            exit(EXIT_FAILURE);
        }
    }
    else {
        warn() << "non emitted expr: " << *e << "\n";
        e->display();
    }
}

void TopBackEnd::emitExpr(std::ostream& os, ir::Expr *e, int ieq, bool minus) {
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        ir::Expr *lhs = be->getLeftOp();
        ir::Expr *rhs = be->getRightOp();
        if (be->getOp() == '+') {
            this->emitExpr(os, lhs, ieq);
            this->emitExpr(os, rhs, ieq);
        }
        else if (be->getOp() == '-') {
            this->emitExpr(os, lhs, ieq);
            this->emitExpr(os, rhs, ieq, true);
        }
        else if (be->getOp() == '*') {
            this->emitTerm(os, e, ieq, minus);
        }
        else {
            err() << "non supported expr\n";
            e->display("unsupported expression:");
            exit(EXIT_FAILURE);
        }
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() == '-') {
            ir::Expr *tmpExpr = new ir::BinExpr(new ir::Value<int>(0),
                    '-',
                    (ir::Expr *) ue->getExpr()->copy());
            this->emitExpr(os, tmpExpr, ieq);
            delete(tmpExpr);
        }
        else {
            err() << "unsupported expr...\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(e)) {
        if (v->getValue() == .0) {}
        else {
            warn() << "skipped expr!\n";
            e->display("skipped expr:");
        }
    }
    else if (auto v = dynamic_cast<ir::Value<int> *>(e)) {
        if (v->getValue() == 0) {}
        else {
            warn() << "skipped expr!\n";
            e->display("skipped expr:");
        }
    }
    else {
        warn() << "skipped expr!\n";
        e->display("skipped expr:");
    }
}

void TopBackEnd::emitEquation(std::ostream& os, ir::Expr *lhs, ir::Expr *rhs) {
    static int ieq = 1;
    this->emitExpr(os, new ir::UnaryExpr(lhs, '-'), ieq);
    this->emitExpr(os, rhs, ieq);
    ieq++;
}

int TopBackEnd::ivar(std::string name) {
    int i = 1;
    for (auto v: this->vars) {
        if (v->getName() == name)
            return i;
        i++;
    }
    err() << "no such variable: `" << name << "\'\n";
    exit(EXIT_FAILURE);
}

void TopBackEnd::emitCode(std::ostream& os) {
    int ieq = 0;
    int nvar = 0;

    for (auto sym: *prog->getSymTab()) {
        if (auto var = dynamic_cast<ir::Variable *>(sym)) {
            this->vars.push_back(var);
            nvar++;
        }
    }

    for (auto v: this->vars) {
        log() << "ivar " << this->ivar(v->getName()) << ": " << v->getName() << "\n";
    }

    for (auto e:*prog->getEqs()) {
        ir::Expr *lhs = e->getLHS();
        ir::Expr *rhs = e->getRHS();
        os << "\tsubroutine eq_" << ieq << "()\n\n";
        os << "\tuse model\n";
        os << "\tuse inputs\n";
        os << "\tuse integrales\n";
        os << "\timplicit none\n\n";
        os << "\tinteger i, j, jj\n\n";
        this->emitEquation(os, lhs, rhs);
        os << "\tend subroutine eq_" << ieq << "\n";
        ieq++;
    }
}

#undef unsupported

#endif
