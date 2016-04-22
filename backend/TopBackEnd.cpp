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

bool haveLlTerms(ir::Expr *e) {
    std::vector<ir::Identifier *> ids = getIds(e);
    for (auto i: ids) {
        if (i->getName() == "l")
            return true;
    }
    return false;
}

ir::Expr *TopBackEnd::extractLlExpr(ir::Expr *e) {
    assert(e);
    std::vector<ir::Identifier *> ids = getIds(e);
    if (ids.size() == 1) {
        if (ids[0]->getName() == "l") {
            return e;
        }
    }
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        if (haveLlTerms(be->getLeftOp()) && !haveLlTerms(be->getRightOp())) {
            return extractLlExpr(be->getLeftOp());
        }
        if (!haveLlTerms(be->getLeftOp()) && haveLlTerms(be->getRightOp())) {
            return extractLlExpr(be->getRightOp());
        }
        if (!haveLlTerms(be->getLeftOp()) && !haveLlTerms(be->getRightOp())) {
            return NULL;
        }
        unsupported(e);
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (haveLlTerms(ue->getExpr())) {
            return extractLlExpr(ue->getExpr());
        }
        return NULL;
    }
    else {
        unsupported(e);
    }
    return NULL;
}

std::vector<ir::Expr *> *TopBackEnd::splitIntoTerms(ir::Expr *e) {
    assert(e);
    std::vector<ir::Expr *> *rLeft = NULL;
    std::vector<ir::Expr *> *rRight = NULL;
    std::vector<ir::Expr *> *r = NULL;

    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        switch(be->getOp()) {
            case '+':
            case '-':
                rLeft = this->splitIntoTerms(be->getLeftOp());
                rRight = splitIntoTerms(be->getRightOp());
                for (auto expr: *rRight) {
                    if (be->getOp() == '-') {
                        rLeft->push_back(new ir::UnaryExpr(expr, '-'));
                    }
                    else {
                        rLeft->push_back(expr);
                    }
                }
                delete(rRight);
                return rLeft;
            case '*':
                r = new std::vector<ir::Expr *>();
                r->push_back(be);
                return r;
            default:
                err() << "unsupported operator `" << be->getOp() << "\'\n";
                exit(EXIT_FAILURE);
        }
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        switch(ue->getOp()) {
            case '-':
                r = this->splitIntoTerms(ue->getExpr());
                return r;
            default:
                err() << "unsupported operator `" << ue->getOp() << "\'\n";
                exit(EXIT_FAILURE);
        }
    }
    else if (auto v = dynamic_cast<ir::Value<int> *>(e)) {
        r = new std::vector<ir::Expr *>();
        r->push_back(v);
        return r;
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(e)) {
        r = new std::vector<ir::Expr *>();
        r->push_back(v);
        return r;
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        r = new std::vector<ir::Expr *>();
        r->push_back(id);
        return r;
    }
    else {
        warn() << "skipped term\n";
        e->display("skipped term");
    }
    return NULL;
}

ir::Identifier *TopBackEnd::findVar(ir::Expr *e) {
    assert(e);
    int nvar = 0;
    ir::Identifier *ret = NULL;
    std::vector<ir::Identifier *> ids = getIds(e);
    for (auto id: ids) {
        if (prog->getSymTab()->search(id)) {
            ret = id;
            nvar++;
        }
    }
    if (nvar > 1) {
        err() << "non linear term are not allowed\n";
        e->display("non linear term in:");
        exit(EXIT_FAILURE);
    }
    return ret;
}

ir::FuncCall *TopBackEnd::findCoupling(ir::Expr *e) {
    assert(e);
    int nfc = 0;
    ir::FuncCall *ret = NULL;
    std::vector<ir::Identifier *> ids = getIds(e);
    for (auto i: ids) {
        if (auto fc = dynamic_cast<ir::FuncCall *>(i)) {
            nfc++;
            ret = fc;
        }
    }
    if (nfc > 1) {
        err() << "non linear term are not allowed\n";
        e->display("non linear term in:");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int TopBackEnd::findDerivativeOrder(ir::Identifier *id) {
    assert(id);
    int order = 0;
    ir::Expr *e = id;
    assert(e);
    assert(e->getParent());
    while (e && e->getParent()) {
        if (auto ue = dynamic_cast<ir::UnaryExpr *>(e->getParent())) {
            if (ue->getOp() == '\'') {
                order++;
                e = dynamic_cast<ir::Expr *>(ue->getParent());
                assert(e);
            }
            else
                return order;
        }
        else {
            break;
        }
    }
    return order;
}

int TopBackEnd::findPower(ir::Expr *e) {
    assert(e);
    int p = 0;
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        std::vector<ir::Identifier *> ids;
        switch (be->getOp()) {
            case '+':
                if (findPower(be->getLeftOp()) == findPower(be->getRightOp()))
                    return findPower(be->getLeftOp());
                else
                    unsupported(e);
                break;
            case '-':
                unsupported(e);
                break;
            case '*':
                return findPower(be->getLeftOp()) + findPower(be->getRightOp());
                break;
            case '/':
                ids = getIds(be->getRightOp());
                for (auto i: ids) {
                    if (i->getName() == "fp") {
                        err() << "fp should not appear in the rhs of a div operator\n";
                        unsupported(e);
                    }
                }
                return findPower(be->getLeftOp());
                break;
            default:
                unsupported(e);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (id->getName() == "fp") {
            return 1;
        }
        else
            return 0;
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        return findPower(ue->getExpr());
    }
    else if (dynamic_cast<ir::Value<int> *>(e)) {
        return 0;
    }
    else if (dynamic_cast<ir::Value<float> *>(e)) {
        return 0;
    }
    else {
        unsupported(e);
    }
    return p;
}

void TopBackEnd::emitScalarExpr(std::ostream& os, ir::Expr *e) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitScalarExpr(os, be->getLeftOp());
        if (be->getOp() == '^') {
            os << "**";
        }
        else {
            os << be->getOp();
        }
        emitScalarExpr(os, be->getRightOp());
        os << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() != '-') {
            err() << "no derivative terms allowed here\n";
            e->display("unsupported derivative term:");
            exit(EXIT_FAILURE);
        }
        os << "-(";
        emitScalarExpr(os, ue->getExpr());
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (id->getName() == "fp" || prog->getSymTab()->search(id->getName())) {
            if (auto be = dynamic_cast<ir::BinExpr *>(id->getParent())) {
                switch (be->getOp()) {
                    case '*':
                    case '/':
                        os << "1d0";
                        break;
                    case '+':
                    case '-':
                        os << "0d0";
                        break;
                    default:
                        unsupported(e);
                }
            }
            else {
                unsupported(e);
            }
        }
        else {
            os << id->getName();
        }
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue();
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue();
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitExpr(std::ostream& os, ir::Expr *e) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitExpr(os, be->getLeftOp());
        if (be->getOp() == '^')
            os << "**";
        else
            os << be->getOp();
        emitExpr(os, be->getRightOp());
        os << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() != '-') {
            err() << "no derivative terms allowed here\n";
            e->display("unsupported derivative term:");
            exit(EXIT_FAILURE);
        }
        os << "-(";
        emitExpr(os, ue->getExpr());
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        os << id->getName();
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue();
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue();
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitLVarExpr(std::ostream& os, ir::Expr *e, int ivar) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitLVarExpr(os, be->getLeftOp(), ivar);
        if (be->getOp() == '^')
            os << "**";
        else
            os << be->getOp();
        emitLVarExpr(os, be->getRightOp(), ivar);
        os << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() != '-') {
            err() << "no derivative terms allowed here\n";
            e->display("unsupported derivative term:");
            exit(EXIT_FAILURE);
        }
        os << "-(";
        emitLVarExpr(os, ue->getExpr(), ivar);
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if(id->getName() == "j") {
            os << "dm(1)%lvar(j, " << ivar << ")";
        }
        else if(id->getName() == "jj") {
            os << "dm(1)%lvar(jj, " << ivar << ")";
        }
        else {
            os << id->getName();
        }
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue();
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue();
    }
    else {
        unsupported(e);
    }
}

bool lTerm(ir::Expr *e) {
    std::vector<ir::Identifier *> ids = getIds(e);
    if (ids.size() == 1) {
        if (ids[0]->getName() == "l") {
            if (auto ue = dynamic_cast<ir::UnaryExpr *>(ids[0]->getParent())) {
                if (ue->getOp() == '\'')
                    return false;
                else return true;
            }
            else return false;
        }
        else {
            return false;
        }
    }
    else return false;
}

bool lpTerm(ir::Expr *e) {
    std::vector<ir::Identifier *> ids = getIds(e);
    if (ids.size() == 1) {
        if (ids[0]->getName() == "l") {
            if (auto ue = dynamic_cast<ir::UnaryExpr *>(ids[0]->getParent())) {
                if (ue->getOp() == '\'')
                    return true;
                else return false;
            }
            else return false;
        }
        else {
            return false;
        }
    }
    else return false;
}

void TopBackEnd::emitTerm(std::ostream& os, ir::Expr *term) {
    assert(term);
    term->setParents();
    ir::Identifier *var = this->findVar(term);
    int power = 0;
    int derOrder = 0;
    ir::FuncCall *couplingIntergal = NULL;
    int ivar = 0;

    if (var) {
        power = this->findPower(term);
        derOrder = this->findDerivativeOrder(var);
        couplingIntergal = this->findCoupling(term);
        ivar = this->ivar(var->getName());
    }
    else { // no var => has to be term "0"
        assert(ivar == 0);
        if(auto val = dynamic_cast<ir::Value<int> *>(term)) {
            if (val->getValue() != 0) {
                unsupported(term);
            }
        }
        else if(auto val = dynamic_cast<ir::Value<float> *>(term)) {
            if (val->getValue() != 0.0) {
                unsupported(term);
            }
        }
        else if(auto ue = dynamic_cast<ir::UnaryExpr *>(term)) {
            if (ue->getOp() == '-') {
                if (auto val = dynamic_cast<ir::Value<int> *>(ue->getExpr())) {
                    if (val->getValue() != 0) {
                        unsupported(term);
                    }
                }
                else if (auto val = dynamic_cast<ir::Value<float> *>(ue->getExpr())) {
                    if (val->getValue() != 0.0) {
                        unsupported(term);
                    }
                }
            }
            else {
                unsupported(term);
            }
        }
        else {
            unsupported(term);
        }
        return;
    }

    if (couplingIntergal || extractLlExpr(term)) {
        ir::Expr *integralExpr = NULL;
        ir::Expr *expr = NULL;
        ir::Expr *llExpr = NULL;
        if (couplingIntergal) {
            integralExpr = couplingIntergal->getArgs()->at(0);
            expr = (ir::Expr *) integralExpr->copy();
            assert(couplingIntergal->getArgs()->size() == 1);
        }
        else {
            expr = (ir::Expr *) term->copy();
        }
        llExpr = extractLlExpr(expr);
        if (llExpr) {
            expr->setParents();
            assert(expr->getParent());
            llExpr->getParent()->replace(llExpr, new ir::Value<float>(1.0));
        }

        if (couplingIntergal) {
            this->nartt++;
            os << "\tdm(1)\%artti(1, " << nartt << ") = " << power    << " ! power\n";
            os << "\tdm(1)\%artti(2, " << nartt << ") = " << derOrder << " ! derivative\n";
            os << "\tdm(1)\%artti(3, " << nartt << ") = " << ieq      << " ! ieq\n";
            os << "\tdm(1)\%artti(4, " << nartt << ") = " << ivar     << " ! ivar\n";
            os << "\tcall " << couplingIntergal->getName() << "( &\n\t\t";
            emitExpr(os, expr);
            os << ", &\n";
            os << "\t\tdm(1)\%artt(1:grd(1)\%nr, 1:nt, 1:nt, " << nartt << "), &\n";
            os << "\t\tdm(1)\%leq(1:nt, " << ieq << "), &\n";
            os << "\t\tdm(1)\%lvar(1:nt, " << ivar << ")";
            os << ")\n";
        }
        if (llExpr) {
            auto replaceL = [] (ir::Identifier *id) {
                if (id->getName() == "l") {
                    assert(id->getParent());
                    id->getParent()->replace(id, new ir::Identifier("j"));
                }
            };
            auto replaceLp = [] (ir::UnaryExpr *ue) {
                if (ue->getOp() == '\'') {
                    if (auto id = dynamic_cast<ir::Identifier *>(ue->getExpr())) {
                        if (id->getName() == "l")
                            assert(ue->getParent());
                            ue->getParent()->replace(ue, new ir::Identifier("jj"));
                    }
                }
            };

            Analysis<ir::UnaryExpr> alp;
            alp.run(replaceLp, llExpr);

            Analysis<ir::Identifier> al;
            al.run(replaceL, llExpr);

            std::vector<ir::Identifier *> ids = getIds(llExpr);
            assert(ids.size() == 1);

            std::string idx = ids[0]->getName();
            char op = 'x';
            if (auto be = dynamic_cast<ir::BinExpr *>(llExpr->getParent())) {
                op = be->getOp();
            }

            std::string mat;
            int nmat;
            if (couplingIntergal) {
                mat = "artt";
                nmat = nartt;
            }
            else {
                assert(llExpr);
                mat = "art";
                this->nart++;
                nmat = nart;
            }
            os << "\tdo " << idx << "=1, nt\n";
            if (idx == "j") {
                os << "\t\tdm(1)\%" << mat << "(1:grd(1)\%nr, " << idx << ", 1:nt, " <<
                    nmat << ") = &\n\t\t\t";
                os << "dm(1)\%" << mat << "(1:grd(1)\%nr, " << idx << ", 1:nt, " <<
                    nmat << ") " << op << " &\n\t\t";
            }
            else if (idx == "jj") {
                os << "\t\tdm(1)\%" << mat << "(1:grd(1)\%nr, 1:nt, " << idx << ", " <<
                    nmat << ") = &\n\t\t\t";
                os << "dm(1)\%" << mat << "(1:grd(1)\%nr, 1:nt, " << idx << ", " <<
                    nmat << ") " << op << " &\n\t\t";
            }
            else {
                err() << "in llexpr\n";
                llExpr->display("unsupported ll expr");
                exit(EXIT_FAILURE);
            }
            os << "\tdble(";
            emitLVarExpr(os, llExpr, ivar);
            os << ")\n";
            os << "\tend do\n";
        }
        os << "\n";
        delete(expr);
    }
    else { // scalar term
        this->nas++;
        os << "\tdm(1)\%asi(1, " << nas << ") = " << power    << " ! power\n";
        os << "\tdm(1)\%asi(2, " << nas << ") = " << derOrder << " ! derivative\n";
        os << "\tdm(1)\%asi(3, " << nas << ") = " << ieq      << " ! ieq\n";
        os << "\tdm(1)\%asi(4, " << nas << ") = " << ivar     << " ! ivar " << var->getName() << "\n";
        os << "\tdm(1)\%as(" << this->nas << ") = ";
        emitScalarExpr(os, term);
        os << "\n\n";
    }
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
    this->nartt = 0;
    this->nart = 0;
    this->nas = 0;
    this->ieq = 0;
    this->nvar = 0;

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
        this->ieq++;
        ir::Expr *lhs = e->getLHS();
        ir::Expr *rhs = e->getRHS();
        ir::Expr *eq = new ir::BinExpr(rhs, '-', lhs);
        os << "\n\tsubroutine eq_" << ieq << "()\n\n";
        os << "\tuse model\n";
        os << "\tuse inputs\n";
        os << "\tuse integrales\n";
        os << "\timplicit none\n\n";
        os << "\tinteger i, j, jj\n\n";
        // this->emitEquation(os, lhs, rhs);
        std::vector<ir::Expr *> *terms = this->splitIntoTerms(eq);
        for (auto t: *terms) {
            this->emitTerm(os, t);
        }
        delete(terms);
        os << "\tend subroutine eq_" << ieq << "\n";
        // delete(eq);
    }

    this->emitInitA(os);
}

void TopBackEnd::emitInitA(std::ostream& os) {
    os << "\n\tsubroutine init_a()\n\n";
    os << "\tdm(1)\%nas = " << nas << "\n";
    os << "\tdm(1)\%nart = " << nart << "\n";
    os << "\tdm(1)\%nartt = " << nartt << "\n\n";

    for (int i=1; i<=this->ieq; i++) {
        os << "\tcall eq_" << i << "()\n";
    }

    os << "\n\tend subroutine init_a\n";
}

#undef unsupported

#endif
