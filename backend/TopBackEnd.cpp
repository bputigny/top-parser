#include "TopBackEnd.h"
#include "Printer.h"
#include "Analysis.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#define unsupported(e) { \
    err << __FILE__ << ":" << __LINE__ << " unsupported expr\n"; \
    e->display("unsupported expr:"); \
    exit(EXIT_FAILURE); \
}

#define err \
    err <<  __FILE__ << ":" << __LINE__ << ": "

ir::Identifier fp("fp");
ir::Identifier l("l");
ir::UnaryExpr ll(&l, '\'');


LlExpr::LlExpr(int ivar, ir::Expr *expr) {
    this->ivar = ivar;
    this->expr = expr;

    if (auto be = dynamic_cast<ir::BinExpr *>(expr->getParent())) {
        this->op = be->getOp();
    }
    else {
        warn << "Could not find llExpr parents\n";
        this->op = '*';
        // exit(EXIT_FAILURE);
    }

    auto ids = getIds(expr);

    // if (ids.size() != 1) {
    //     err << "building llExpr from non compatible expression\n";
    //     expr->display();
    //     exit(EXIT_FAILURE);
    // }
    // else {
    if (expr->contains(ll))
        this->type = "ll";
    else if (expr->contains(l))
        this->type = "l";
    else {
        err << "llExpr does not contain l or l' terms\n";
        exit(EXIT_FAILURE);
    }
}

void TopBackEnd::emitExpr(ir::Expr *expr, Output& o,
        int ivar, int ieq, bool emitLlExpr, std::string bcLocation) {

    if (*expr == l) {
        if (emitLlExpr)
            o << "dm(1)\%lvar(j, " << ivar << ")";
        else
            o << "1d0";
        return;
    }
    if (*expr == ll) {
        if (emitLlExpr)
            o << "dm(1)\%lvar(jj, " << ivar << ")";
        else
            o << "1d0";
        return;
    }
    if (*expr == fp) {
        o << "1d0";
        return;
    }

    if (auto be = dynamic_cast<ir::BinExpr *>(expr)) {
        o << "(";
        emitExpr(be->getLeftOp(), o, ivar, ieq, emitLlExpr, bcLocation);
        switch (be->getOp()) {
            case '^':
                o << "**";
                break;
            default:
                o << be->getOp();
        }
        emitExpr(be->getRightOp(), o, ivar, ieq, emitLlExpr, bcLocation);
        o << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(expr)) {
        o << ue->getOp() << "(";
        emitExpr(ue->getExpr(), o, ivar, ieq, emitLlExpr, bcLocation);
        o << ")";
    }
    else if (auto fc = dynamic_cast<ir::FuncCall *>(expr)) {
        int narg = 0;
        o << fc->getName();
        o << "(";
        for (auto a: *fc->getArgs()) {
            if (narg++ > 0)
                o << ", ";
            emitExpr(a, o, ivar, ieq, emitLlExpr, bcLocation);
        }
        o << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(expr)) {
        if (isDef(id->getName())) {
            o << id->getName();
            if (bcLocation != "" && isField(id->getName())) {
                o << "(" << bcLocation << ", 1:lres)";
            }
        }
        else {
            err << "`" << id->getName() << "\' is undefined\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto v = dynamic_cast<ir::Value<int> *>(expr)) {
        o << v->getValue();
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(expr)) {
        o << v->getValue() << "d0";
    }
    else {
        unsupported(expr);
    }
}

Term::Term(ir::Expr *expr, ir::Expr *llTerm, ir::Symbol var,
                int power, std::string der, int ivar, std::string varName) : var(var) {
    this->expr = expr;
    if (llTerm)
        this->llExpr = new LlExpr(ivar, llTerm);
    else
        this->llExpr = NULL;
    this->power = power;
    this->der = der;
    this->ivar = ivar;
    this->varName = varName;

    this->idx = 0;
}

TermBC::TermBC(Term t) : Term(t.expr, NULL, t.var, t.power, t.der, t.ivar, t.varName) {
    if (t.llExpr)
        this->llExpr = new LlExpr(t.ivar, t.llExpr->expr);
    varLoc = "1";
    eqLoc = "1";
}

std::string Term::getMatrixI() {
    switch (this->getType()) {
        case AS:
            return "dm(1)\%asi";
        case ART:
            return "dm(1)\%arti";
        case ARTT:
            return "dm(1)\%artti";
        case ATBC:
            return "idm(1, 1)\%atbci";
        case ATTBC:
            return "idm(1, 1)\%attbci";
    }
}

std::string Term::getMatrix(IndexType it) {
    std::string idx = std::to_string(this->idx);
    switch (this->getType()) {
        case AS:
            if (it != FULL) {
                err << "cannot access subscript of scalar terms\n";
                    exit(EXIT_FAILURE);
            }
            return "dm(1)\%as(" + idx + ")";
        case ART:
            switch(it) {
                case FULL:
                    return "dm(1)\%art(1:grd(1)\%nr, 1:nt, " + idx + ")";
                case T:
                    return "dm(1)\%art(1:grd(1)\%nr, j, " + idx + ")";
                default:
                    err << "cannot access subscript of `ART\' terms\n";
                    exit(EXIT_FAILURE);
            }
        case ARTT:
            switch(it) {
                case FULL:
                    return "dm(1)\%artt(1:grd(1)\%nr, 1:nt, 1:nt, " + idx + ")";
                case T:
                    return "dm(1)\%artt(1:grd(1)\%nr, j, 1:nt, " + idx + ")";
                case TT:
                    return "dm(1)\%artt(1:grd(1)\%nr, 1:nt, jj, " + idx + ")";
            }
        case ATBC:
            switch(it) {
                case FULL:
                    return "idm(1, 1)\%atbc(1:nt, " + idx + ")";
                case T:
                    return "idm(1, 1)\%atbc(j, " + idx + ")";
                default:
                    err << "cannot access subscript of `ATBC\' terms\n";
                    exit(EXIT_FAILURE);
            }
        case ATTBC:
            switch(it) {
                case FULL:
                    return "idm(1, 1)\%attbc(1:nt, 1:nt, " + idx + ")";
                case T:
                    return "idm(1, 1)\%attbc(j, 1:nt, " + idx + ")";
                case TT:
                    return "idm(1, 1)\%attbc(1:nt, jj, " + idx + ")";
            }
    }
    err << "unknown matrix type";
    exit(EXIT_FAILURE);
}

bool isZero(ir::Expr *e) {
    ir::Value<int> i0(0);
    ir::Value<float> f0(0);
    ir::UnaryExpr mi0(&i0, '-');
    ir::UnaryExpr mf0(&f0, '-');
    return (*e) == i0 ||
        (*e) == f0 ||
        (*e) == mi0 ||
        (*e) == mf0;
}

std::list<ir::Equation *> TopBackEnd::formatEquations() {
    std::list<ir::Equation *> list;

    for (auto e: *prog->getEqs()) {
        this->simplify(e->getLHS());
        this->simplify(e->getRHS());
        ir::Expr *lhs = e->getLHS();
        ir::Expr *rhs = e->getRHS();
        ir::Expr *eq = NULL;
        if (isZero(lhs)) {
            eq = rhs;
        }
        else if (isZero(rhs)) {
            eq = lhs;
        }
        else {
            eq = new ir::BinExpr(rhs, '-', lhs);
        }
        if (e->getName() == "undef") {
            err << "equation without a name\n";
            exit(EXIT_FAILURE);
        }
        ir::Equation *newEq = new ir::Equation(eq, NULL, e->getBCs());
        newEq->setName(e->getName());
        list.push_back(newEq);
    }
    return list;
}

ir::FuncCall *isCoupling(ir::Expr *e) {
    if (auto fc = dynamic_cast<ir::FuncCall *>(e)) {
        if (std::strstr(fc->getName().c_str(), "llm")) {
            return fc;
        }
    }
    return NULL;
}

TermType Term::getType() {
    if (isCoupling(expr)) {
        return ARTT;
    }
    else {
        if (llExpr) {
            // TODO: not sure it is correct: we can have a rtt term is the
            // llExpr depends on l'
            return ART;
        }
        else {
            return AS;
        }
    }
}

TermType TermBC::getType() {
    if (isCoupling(expr)) {
        return ATTBC;
    }
    else return ATBC;
}

void TopBackEnd::checkCoupling(ir::Expr *expr) {
    static int singlePrint = 0;
    ir::Expr *coupling = this->findCoupling(expr);
    if (coupling == NULL) {
        err << "malformed coupling expression\n";
        exit(EXIT_FAILURE);
    }
    if (singlePrint++ == 0) {
        warn << "check coupling correctly...\n";
    }
}

Term *TopBackEnd::buildTerm(ir::Expr *t) {
    t->setParents();
    ir::Identifier *var = this->findVar(t);
    if (!var) {
        err << "term without varaible?";
        exit(EXIT_FAILURE);
    }
    int power = this->findPower(t);
    if (power > this->powerMax)
        this->powerMax = power;
    std::string der = this->findDerivativeOrder(var);
    ir::Expr *expr = this->findCoupling(t);
    ir::Expr *llExpr = NULL;
    std::string varName = var->getName();
    int ivar = this->ivar(var->getName());
    if (expr == NULL) {
        if (auto be = dynamic_cast<ir::BinExpr *>(t)) {
            if (be->getOp() != '*') {
                err << "terms should be products...\n";
                exit(EXIT_FAILURE);
            }
            if (auto id = dynamic_cast<ir::Identifier *>(be->getRightOp())) {
                if (isVar(id->getName()))
                    expr = be->getLeftOp();
                else
                    unsupported(t);
            }
            else if (auto de = dynamic_cast<ir::DiffExpr *>(be->getRightOp())) {
                ir::Identifier *id = dynamic_cast<ir::Identifier *>(de->getExpr());
                if (id != NULL && isVar(id->getName()))
                    expr = be->getLeftOp();
                else
                    unsupported(t);
            }
            else if (auto id = dynamic_cast<ir::Identifier *>(be->getLeftOp())) {
                if (isVar(id->getName()))
                    expr = be->getRightOp();
                else
                    unsupported(t);
            }
            else if (auto de = dynamic_cast<ir::DiffExpr *>(be->getLeftOp())) {
                ir::Identifier *id = dynamic_cast<ir::Identifier *>(de->getExpr());
                if (id != NULL && isVar(id->getName()))
                    expr = be->getRightOp();
                else
                    unsupported(t);
            }
            else {
                err << "malformed term\n";
                t->display("malformed term");
                exit(EXIT_FAILURE);
            }
        }
        else if (auto ue = dynamic_cast<ir::UnaryExpr *>(t)) {
            Term *ret = buildTerm(ue->getExpr());
            if (ue->getOp() != '-')
                unsupported(ue);
            ret->expr = new ir::UnaryExpr(ret->expr, '-');
            return ret;
        }
        else if (auto de = dynamic_cast<ir::DiffExpr *>(t)) {
            if (auto id = dynamic_cast<ir::Identifier *>(de->getExpr())) {
                if (id == var)
                    expr = new ir::Value<float>(1.0);
                else
                    unsupported(t);
            }
            else 
                unsupported(t);
        }
        else if (auto id = dynamic_cast<ir::Identifier *>(t)) {
            if (id != var)
                unsupported(t);
            expr = new ir::Value<float>(1.0);
        }
        else {
            unsupported(t);
        }
        llExpr = extractLlExpr(expr);
    }
    else {
        checkCoupling(t);
        llExpr = dynamic_cast<ir::Expr *>(expr->getChildren()[0]);
        if (llExpr)
            llExpr = extractLlExpr(llExpr);
        else {
            err << "coupling integral expression is not a proper expression\n";
            exit(EXIT_FAILURE);
        }
        if (expr == NULL) {
            err << "coupling integral without an expression...\n";
            exit(EXIT_FAILURE);
        }
    }

#if 1
    if (llExpr) {
        ir::Expr *newNode = new ir::Value<float>(1.0);
        this->prog->replace(llExpr, newNode);
        if (expr == llExpr) {
            expr = newNode;
        }
    }
#endif

    return new Term(expr, llExpr,
            ir::Symbol(var->getName()),
            power, der, ivar, varName);
}

int TopBackEnd::computeTermIndex(Term *term) {
    switch (term->getType()) {
        case AS:
            this->nas++;
            return nas;
        case ART:
            this->nart++;
            return nart;
        case ARTT:
            this->nartt++;
            return nartt;
        case ATBC:
            this->natbc++;
            return natbc;
        case ATTBC:
            this->nattbc++;
            return nattbc;
    }
}

std::string getVarLocation(ir::BC *bc) {
    if (auto id = dynamic_cast<ir::Identifier *>(bc->getLoc()->getLHS())) {
        if (id->getName() == "r") {
            if (auto v = dynamic_cast<ir::Value<int> *>(bc->getLoc()->getRHS())) {
                if (v->getValue() == 0)
                    return "1";
                if (v->getValue() == 1)
                    return "nr";
            }
            else {
                err << "BC location should be given with regard to r\n";
                exit(EXIT_FAILURE);
            }
        }
        else {
            err << "BC should be `r = 0\' or `r = 1\' ";
            exit(EXIT_FAILURE);
        }
    }
    err << "BC should be `r = 0\' or `r = 1\' ";
    exit(EXIT_FAILURE);
}

std::string getEqLocation(ir::BC *bc) {
    if (bc->getEqLoc() == 0)
        return "1";
    else if (bc->getEqLoc() == 1)
        return "nr";
    else {
        err << "unsupported location in BC\n";
        exit(EXIT_FAILURE);
    }
}

void TopBackEnd::buildTermList(std::list<ir::Equation *> eqs) {
    int ieq = 0;

    this->nas = 0;
    this->nart = 0;
    this->nartt = 0;
    this->natbc = 0;
    this->nattbc = 0;

    for (auto e: eqs) {
        ieq++;
        std::vector<ir::Expr *> *terms = this->splitIntoTerms(e->getLHS());
        this->eqs[e->getName()] = std::list<Term *>();
        if (terms) {
            for (auto t: *terms) {
                Term *term = buildTerm(t);
                term->ieq = ieq;
                term->eqName = e->getName();
                term->idx = computeTermIndex(term);
                this->eqs[e->getName()].push_back(term);
            }

            for (auto bc: *e->getBCs()) {
                this->simplify(bc->getCond()->getLHS());
                this->simplify(bc->getCond()->getRHS());
                this->simplify(bc->getLoc()->getLHS());
                this->simplify(bc->getLoc()->getRHS());

                ir::Expr *lhs = bc->getCond()->getLHS();
                ir::Expr *rhs = bc->getCond()->getRHS();
                ir::Expr *eq = NULL;

                this->simplify(lhs);
                this->simplify(rhs);

                if (isZero(rhs)) {
                    eq = (ir::Expr *) lhs->copy();
                }
                else if (isZero(lhs)) {
                    eq = new ir::UnaryExpr((ir::Expr *) rhs->copy(), '-');
                }
                else {
                    eq = new ir::BinExpr(lhs, '-', rhs);
                }

                eq->setParents();
                std::vector<ir::Expr *> *terms = this->splitIntoTerms(eq);
                for (auto t: *terms) {
                    TermBC *termBC = new TermBC(*buildTerm(t));
                    termBC->ieq = ieq;
                    termBC->eqName = e->getName();
                    termBC->idx = computeTermIndex(termBC);
                    termBC->eqLoc = getEqLocation(bc);
                    termBC->varLoc = getVarLocation(bc);
                    this->eqs[e->getName()].push_back(termBC);
                }
            }

        }
        else  {
            err << "could not split equation into proper terms\n";
            exit(EXIT_FAILURE);
        }
        delete terms;
    }
}

TopBackEnd::TopBackEnd(ir::Program *p) {
    this->prog = p;
    this->nas = 0;
    this->nartt = 0;
    this->nart = 0;
    this->nvar = 0;
    this->natbc = 0;
    this->nattbc = 0;
    this->powerMax = 0;

    buildVarList();
    std::list<ir::Equation *> eqs = formatEquations();
    buildTermList(eqs);
}

TopBackEnd::~TopBackEnd() { }

// this takes derivative expressions (e.g., u''') and fold them into DiffExpr
// (e.g., DiffExpr(u, r, 3))
// this also transform FuncCall (dr(u, 3) into the specialized DiffExpr(u, r, 3)
void TopBackEnd::simplify(ir::Expr *expr) {
    Analysis<ir::Identifier> a;
    Analysis<ir::UnaryExpr> a2;
    assert(expr);
    auto foldDer = [this] (ir::Identifier *id) {
        ir::Expr *root = id;
        ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(id->getParent());
        int order = 0;

        if (*id == l || *id == ll) return;

        while (ue) {
            if (ue->getOp() == '\'') {
                order++;
                root = ue;
            }
            else {
                break;
            }
            ue = dynamic_cast<ir::UnaryExpr *>(ue->getParent());
        }
        if (order > 0) {
            ir::Node *newNode = new ir::DiffExpr(id,
                    new ir::Identifier("r"),
                    std::to_string(order));
            prog->replace(root, newNode);
        }
    };

    auto foldConst = [this, &expr] (ir::UnaryExpr *ue) {
        if (ue->getOp() == '-') {
            if (auto val = dynamic_cast<ir::Value<int> *>(ue->getExpr())) {
                ir::Value<int> *newNode = new ir::Value<int>(-val->getValue());
                prog->replace(ue, newNode);
            }
        }
    };

    auto foldDr = [this, &expr] (ir::Identifier *id) {

        if (id->getName() == "dr") {
            ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(id);
            assert(fc);

            std::string derOrder;
            if (auto order = dynamic_cast<ir::Identifier *>(fc->getChildren()[1])) {
                derOrder = order->getName();
            }
            else if (auto order = dynamic_cast<ir::Value<int> *>(fc->getChildren()[1])) {
                derOrder = std::to_string(order->getValue());
            }
            else {
                err << "derivative order should be an integer or a variable...";
                unsupported(fc);
            }
            if (auto derVar = dynamic_cast<ir::Identifier *>(fc->getChildren()[0])) {
                ir::Node *newNode = new ir::DiffExpr(
                        derVar,
                        new ir::Identifier("r"),
                        derOrder);
                prog->replace(fc, newNode);
            }
            else {
                err << "derivative of expression not yet supported...";
                unsupported(fc);
            }
        }
    };
    expr->setParents();
    a2.run(foldConst, expr);
    expr->setParents();
    a.run(foldDr, expr);
    expr->setParents();
    a.run(foldDer, expr);
}

bool TopBackEnd::isVar(std::string id) {
    ir::Symbol *s = this->prog->getSymTab()->search(id);
    if (s) {
        if (dynamic_cast<ir::Variable *>(s))
            return true;
    }
    return false;
}

bool TopBackEnd::isParam(std::string id) {
    ir::Symbol *s = this->prog->getSymTab()->search(id);
    if (s) {
        if (dynamic_cast<ir::Param *>(s))
            return true;
    }
    return false;
}

bool TopBackEnd::isField(std::string id) {
    ir::Symbol *s = this->prog->getSymTab()->search(id);
    if (s) {
        if (dynamic_cast<ir::Field *>(s))
            return true;
    }
    return false;
}

bool TopBackEnd::isScal(std::string id) {
    ir::Symbol *s = this->prog->getSymTab()->search(id);
    if (s) {
        if (dynamic_cast<ir::Scalar *>(s))
            return true;
    }
    return false;
}

bool TopBackEnd::isDef(std::string id) {
    return isField(id) || isScal(id) || isVar(id) || isParam(id);
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

    if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (auto ll = extractLlExpr(ue->getExpr())) {
            ir::UnaryExpr *newExpr = new ir::UnaryExpr(ll,
                    ue->getOp());
            newExpr->setParent(e->getParent());
            return newExpr;
        }
    }

    auto ids = getIds(e);
    if (ids.size() == 1) {
        if (*ids[0] == l || *ids[0] == ll) {
            return e;
        }
    }
    if (ids.size() == 0)
        return NULL;

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
        return extractLlExpr(ue->getExpr());
    }
    else if (auto fc = dynamic_cast<ir::FuncCall *>(e)) {
        for (auto arg: *fc->getArgs()) {
            if (extractLlExpr(arg)) {
                return fc;
            }
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (*id == l || *id == ll) {
            return id;
        }
        else {
            return NULL;
        }
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
                // delete(rRight);
                return rLeft;
            case '*':
                r = new std::vector<ir::Expr *>();
                r->push_back(be);
                return r;
            default:
                err << "unsupported operator `" << be->getOp() << "\'\n";
                exit(EXIT_FAILURE);
        }
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        switch(ue->getOp()) {
            case '-':
                r = this->splitIntoTerms(ue->getExpr());
                return r;
            default:
                err << "unsupported unary operator `" << ue->getOp() << "\'\n";
                e->display("unsupported unary operator");
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
    else if (auto de = dynamic_cast<ir::DiffExpr *>(e)) {
        r = new std::vector<ir::Expr *>();
        r->push_back(de);
        return r;
    }
    else {
        err << "skipped term\n";
        unsupported(e);
    }
    return NULL;
}

ir::Identifier *TopBackEnd::findVar(ir::Expr *e) {
    assert(e);
    int nvar = 0;
    ir::Identifier *ret = NULL;
    std::vector<ir::Identifier *> ids = getIds(e, false);
    for (auto id: ids) {
        if (this->isVar(id->getName())) {
            ret = id;
            nvar++;
        }
    }
    if (nvar > 1) {
        err << "non linear term are not allowed\n";
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
        if (auto fc = isCoupling(i)) {
            nfc++;
            ret = fc;
        }
    }
    if (nfc > 1) {
        err << "non linear term are not allowed\n";
        e->display("non linear term in:");
        exit(EXIT_FAILURE);
    }
    return ret;
}

std::string TopBackEnd::findDerivativeOrder(ir::Identifier *id) {
    assert(id);
    int order = 0;
    ir::Expr *e = id;
    assert(e);
    if (e->getParent() == NULL) {
        return "0";
    }
    assert(e->getParent());

    if (auto de = dynamic_cast<ir::DiffExpr *>(e->getParent())) {
        return de->getOrder();
    }

    while (e && e->getParent()) {
        if (auto ue = dynamic_cast<ir::UnaryExpr *>(e->getParent())) {
            if (ue->getOp() == '\'') {
                order++;
                e = dynamic_cast<ir::Expr *>(ue->getParent());
                assert(e);
            }
            else
                return std::to_string(order);
        }
        else {
            break;
        }
    }
    return std::to_string(order);
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
            case '^':
                if (auto v = dynamic_cast<ir::Value<int> *>(be->getRightOp())) {
                        return findPower(be->getLeftOp()) * v->getValue();
                }
                else
                    unsupported(e);
                break;
            case '/':
                ids = getIds(be->getRightOp());
                for (auto i: ids) {
                    if (i->getName() == "fp") {
                        err << "fp should not appear in the rhs of a div operator\n";
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
    else if (dynamic_cast<ir::DiffExpr *>(e)) {
        return 0;
    }
    else {
        unsupported(e);
    }
    return p;
}

void TopBackEnd::emitTerm(Output& os, Term *term) {
    switch(term->getType()) {
        case AS:
        case ART:
            os << "      " << term->getMatrix(FULL) << " = ";
            emitExpr(term->expr, os, term->ivar, term->ieq, true);
            os << "\n";
            break;
        case ARTT:
            if (auto fc = isCoupling(term->expr)) {
                ir::Expr *arg = dynamic_cast<ir::Expr *>(term->expr->getChildren()[0]);
                os << "      call " << fc->getName() << "(";
                emitExpr(arg, os, term->ivar, term->ieq, false);
                os << ", ";
                os << " &\n";
                os << "            & ";
                os << term->getMatrix(FULL);
                os << ", &\n";
                os << "            & ";
                os << "dm(1)\%leq(1:nt, " << term->ieq << "), ";
                os << " &\n";
                os << "            & ";
                os << "dm(1)\%lvar(1:nt, " << term->ivar << ")";
                os << ")\n";
            }
            else {
                err << "ARTT terms should always have coupling integral expression\n";
                exit(EXIT_FAILURE);
            }
            break;
        default:
            err << "BC term not handled\n";
            exit(EXIT_FAILURE);
    }

    if (term->llExpr) {
        if (term->llExpr->type == "l") {
            if (term->getType() == ART) {
                os << "      do j=1, nt\n";
                os << "            " << term->getMatrix(T) <<
                    " = &\n";
                os << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else if (term->getType() == ARTT ){
                os << "      do j=1, nt\n";
                os << "            " << term->getMatrix(T) <<
                    " = &\n";
                os << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else {
                err << "llExpr with term not depending on l?\n";
                exit(EXIT_FAILURE);
            }
        }
        else if (term->llExpr->type == "ll") {
            os << "      do jj=1, nt\n";
            os << "            " << term->getMatrix(TT) << " = &\n";
            os << "                  & " << term->getMatrix(TT) <<
                " " << term->llExpr->op << " &\n";
        }
        else {
            err << "\n";
            exit(EXIT_FAILURE);
        }
        emitExpr(term->llExpr->expr, os, term->llExpr->ivar, term->ieq, true);
        os << "\n      end do\n";
    }
}

void TopBackEnd::emitTerm(Output& os, TermBC *term) {
    std::string bcLoc = term->varLoc;
    switch(term->getType()) {
        case ATBC:
            os << "      " << term->getMatrix(FULL) << " = ";
            emitExpr(term->expr, os, term->ivar, term->ieq, true, bcLoc);
            os << "\n";
            break;
        case ATTBC:
            if (auto fc = isCoupling(term->expr)) {
                ir::Expr *arg = dynamic_cast<ir::Expr *>(term->expr->getChildren()[0]);
                os << "      call " << fc->getName() << "bc(";
                emitExpr(arg, os, term->ivar, term->ieq, false, bcLoc);
                os << ", ";
                os << " &" << "\n";
                os << "            & ";
                os << term->getMatrix(FULL);
                os << ",";
                os << " &" << "\n";
                os << "            & ";
                os << "dm(1)\%leq(1:nt, " << term->ieq << "), ";
                os << " &" << "\n";
                os << "            & ";
                os << "dm(1)\%lvar(1:nt, " << term->ivar << ")";
                os << ")\n";
            }
            else {
                err << "ARTT terms should always have coupling integral expression\n";
                term->expr->display();
                exit(EXIT_FAILURE);
            }
            break;
        default:
            err << "BC term not handled\n";
            exit(EXIT_FAILURE);
    }

    if (term->llExpr) {
        if (term->llExpr->type == "l") {
            if (term->getType() == ATBC) {
                os << "      do j=1, nt\n";
                os << "            " << term->getMatrix(T) <<
                    " = &\n";
                os << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else if (term->getType() == ATTBC ){
                os << "      do j=1, nt\n";
                os << "            " << term->getMatrix(T) <<
                    " = &\n";
                os << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else {
                err << "llExpr with term not depending on l?\n";
                exit(EXIT_FAILURE);
            }
        }
        else if (term->llExpr->type == "ll") {
            os << "      do jj=1, nt\n";
            os << "            " << term->getMatrix(TT) <<
                " = &\n";
            os << "                  " << term->getMatrix(TT) <<
                " " << term->llExpr->op << " &\n";
        }
        else {
            err << "\n";
            exit(EXIT_FAILURE);
        }
        os << "                  ";
        emitExpr(term->llExpr->expr, os, term->llExpr->ivar, term->ieq, true, bcLoc);
        os << "\n      end do\n";
    }
}

void Term::emitInitIndex(Output& o) {
    o << "      " << this->getMatrixI() << "(1, " << this->idx << ") = " <<
        this->power << " ! power\n";
    o << "      " << this->getMatrixI() << "(2, " << this->idx << ") = " <<
        this->der << " ! der order\n";
    o << "      " << this->getMatrixI() << "(3, " << this->idx << ") = " <<
        this->ieq << " ! ieq (eq: " << this->eqName << ")\n";
    o << "      " << this->getMatrixI() << "(4, " << this->idx << ") = " <<
        this->ivar << " ! ivar (var: " << this->varName << ")\n";
}

void TermBC::emitInitIndex(Output& o) {
    Term::emitInitIndex(o);
    o << "      " << this->getMatrixI() << "(5, " << this->idx << ") = " <<
        this->eqLoc << " ! eq location\n";
    o << "      " << this->getMatrixI() << "(6, " << this->idx << ") = " <<
        this->varLoc << " ! var location\n";
}

int TopBackEnd::ivar(std::string name) {
    int i = 1;
    for (auto v: this->vars) {
        if (v->getName() == name)
            return i;
        i++;
    }
    err << "no such variable: `" << name << "\'\n";
    exit(EXIT_FAILURE);
}

int TopBackEnd::ieq(std::string name) {
    int i = 1;
    for (auto e: *this->prog->getEqs()) {
        if (e->getName() == name)
            return i;
        i++;
    }
    err << "no such equation: `" << name << "\'\n";
    exit(EXIT_FAILURE);
}

void TopBackEnd::buildVarList() {
    for (auto sym: *prog->getSymTab()) {
        if (auto var = dynamic_cast<ir::Variable *>(sym)) {
            this->vars.push_back(var);
            nvar++;
        }
    }
}

void TopBackEnd::emitUseModel(Output& os) {
    os << "      use model, only: ";
    int n = 0;
    for (auto id: *this->prog->getSymTab()) {
        if (isField(id->getName()) || isScal(id->getName())) {
            if (n > 0) {
                os << ", ";
            }
            n++;
            os << id->getName();
        }
    }
    os << "\n";
}

void TopBackEnd::emitCode(Output& os) {

    for (auto e: *prog->getEqs()) {
        os << "!------------------------------------------------------------\n";
        os << "! Indices for equation " << e->getName() << "\n";
        os << "!------------------------------------------------------------\n";
        os << "      subroutine eqi_" << e->getName() << "()\n\n";
        emitUseModel(os);
        os << "      use inputs\n";
        os << "      implicit none\n";

        for (auto t: eqs[e->getName()]) {
            t->emitInitIndex(os);
            os << "\n";
        }

        os << "      end subroutine eqi_" << e->getName() << "\n";
    }

    for (auto e: *prog->getEqs()) {
        os << "!------------------------------------------------------------\n";
        os << "! Coupling coefficients for equation " << e->getName() << "\n";
        os << "!------------------------------------------------------------\n";
        os << "      subroutine eq_" << e->getName() << "()\n\n";
        emitUseModel(os);
        os << "      use inputs\n";
        os << "      use integrales\n";
        os << "      implicit none\n";
        os << "      integer i, j, jj\n";
        for (auto t: eqs[e->getName()]) {
            if (auto tbc = dynamic_cast<TermBC *>(t)) {
                this->emitTerm(os, tbc);
            }
            else {
                this->emitTerm(os, t);
            }
            os << "\n";
        }
        os << "      end subroutine eq_" << e->getName() << "\n\n";
    }

    this->emitInitA(os);
}

void emitDeclRHS(Output& os, ir::Expr *expr) {
    assert(expr);
    if (auto be = dynamic_cast<ir::BinExpr *>(expr)) {
        os << "(";
        emitDeclRHS(os, be->getLeftOp());
        os << be->getOp();
        emitDeclRHS(os, be->getRightOp());
        os << ")";
    }
    else if (auto fc = dynamic_cast<ir::FuncCall *>(expr)) {
        int iarg = 0;
        os << fc->getName() << "(";
        for (auto arg: *fc->getArgs()) {
            if (iarg++ > 0)
                os << ", ";
            emitDeclRHS(os, arg);
        }
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(expr)) {
        if (id->getName() == "lvar")
            os << "dm(1)\%";
        os << id->getName();
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(expr)) {
        os << val->getValue();
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(expr)) {
        os << val->getValue() << "d0";
    }
    else {
        err << "skipped term!!\n";
        expr->display("skipped term:");
    }
}

void TopBackEnd::emitDecl(Output& os, ir::Decl *decl,
        std::map<std::string, bool>& lvar_set,
        std::map<std::string, bool>& leq_set) {
    assert(decl);
    ir::Expr *lhs = decl->getLHS();
    if (auto fc = dynamic_cast<ir::FuncCall *>(lhs)) {
        if (fc->getName() == "lvar") {
            ir::Identifier *id = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0));
            assert(id);
            int ivar = this->ivar(id->getName());
            os << "      dm(1)\%lvar(1, " << ivar <<
                ") = ";
            emitDeclRHS(os, decl->getDef());
            os << " ! var: " << id->getName() << "\n";
            lvar_set[id->getName()] = true;
        }
        else if (fc->getName() == "leq") {
            ir::Identifier *id = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0));
            assert(id);
            int ieq = this->ieq(id->getName());
            os << "      dm(1)\%leq(1, " << ieq <<
                ") = ";
            emitDeclRHS(os, decl->getDef());
            os << " ! eq: " << id->getName() << "\n";
            leq_set[id->getName()] = true;
        }
        else {
            err << "function not allowed in LHS\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(lhs)) {
        os << "      " << id->getName() << " = ";
        emitDeclRHS(os, decl->getDef());
        os << "\n";
    }
    else {
        err << "unsupported definition: " << *lhs << "\n";
        exit(EXIT_FAILURE);
    }
}

void TopBackEnd::emitInitA(Output& os) {
    int nvar = 0;
    int neq = this->prog->getEqs()->size();

    std::map<std::string, bool> lvar_set;
    std::map<std::string, bool> leq_set;

    std::ofstream inputs("inputs.F90");
    inputs << "module inputs\n\n";
    inputs << "    use iso_c_binding\n";
    inputs << "    use mgetpar\n";
    inputs << "    use mod_grid, only: grd, nt, init_grid\n";
    inputs << "    implicit none\n";

    inputs << "    character*(4), save :: mattype\n";
    inputs << "    character*(4), save :: dertype\n";
    inputs << "    double precision, save :: shift\n";
    inputs << "    integer, save :: nsol\n";

    for (auto s: *this->prog->getSymTab()) {
        if (dynamic_cast<ir::Variable *>(s)) {
            nvar++;
            lvar_set[s->getName()] = false;
        }
        else if (auto p = dynamic_cast<ir::Param *>(s)) {
            std::string type;
            if (p->getType() == "int") {
                type = "integer";
            }
            else if (p->getType() == "double") {
                type = "double precision";
            }
            else if (p->getType() == "string") {
                type = "character(512)";
            }
            else {
                err << "unsupported type in definition\n";
                exit(EXIT_FAILURE);
            }
            inputs << "    " << type << ", save :: " << p->getName() << "\n";
        }
    }
    inputs << "\n";

    inputs << "contains\n\n";

    inputs << "    subroutine read_inputs(dati)\n\n";
    inputs << "        character(len=*), intent(in) :: dati\n\n";
    inputs << "        call init_grid(1)\n";
    inputs << "        call read_file(trim(dati))\n";
    inputs << "        grd(1)\%nr = fetch('nr', 0)\n";
    inputs << "        nt = fetch('nt', 0)\n";
    inputs << "        shift = fetch('shift', 0d0)\n";
    inputs << "        nsol = fetch('nsol', 4)\n\n";
    inputs << "        mattype = fetch('mattype', 'FULL')\n";
    inputs << "        dertype = fetch('dertype', 'CHEB')\n\n";
    for (auto s: *this->prog->getSymTab()) {
        if (auto p = dynamic_cast<ir::Param *>(s)) {
            std::string default_value;
            if (p->getType() == "double") {
                default_value = "0d0";
            }
            else if (p->getType() == "int") {
                default_value = "0";
            }
            else if (p->getType() == "string") {
                default_value = "\"\"";
            }
            else {
                err << "Unsupported type in definition of parameter `" <<
                    p->getName() << "\'\n";
                exit(EXIT_FAILURE);
            }

            inputs << "        " << p->getName() << " = fetch('" << p->getName() <<
                "', " << default_value << ")\n";
        }
    }
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine write_inputs(i)\n\n";
    inputs << "        integer, intent(in) :: i\n";
    inputs << "\n";
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine write_stamp(i)\n\n";
    inputs << "        integer, intent(in) :: i\n";
    inputs << "\n";
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine init_default()\n\n";
    inputs << "\n";
    inputs << "    end subroutine\n\n";

    inputs << "end module inputs\n";
    inputs.close();

    for (auto e: *prog->getEqs()) {
        leq_set[e->getName()] = false;;
    }

    os << "\n      subroutine init_a()\n\n";

    os << "      use model\n";
    os << "      use integrales\n\n";

    os << "      implicit none\n";
    os << "      integer j, der_min, der_max\n\n";

    os << "      grd(1)%mattype = mattype\n";
    os << "      nr => grd(1)\%nr\n";
    os << "      if (allocated(dm)) then\n";
    os << "            call clear_all()\n";
    os << "      endif\n\n";

    os << "      allocate(dm(1))\n";
    os << "      allocate(dmat(1))\n";
    os << "      allocate(idm(1, 1))\n\n";


    os << "      dm(1)\%nvar = " << nvar << "\n";
    os << "      allocate(dm(1)\%lvar(nt, " << nvar << "))\n";
    os << "      allocate(dm(1)\%leq(nt, " << neq << "))\n\n";

    for (auto d: *prog->getDecls()) {
        emitDecl(os, d, lvar_set, leq_set);
    }

    for (auto lv: lvar_set) {
        if (lv.second == false) {
            err << "lvar for variable `" << lv.first << "\' is not set\n";
            exit(EXIT_FAILURE);
        }
    }
    for (auto le: leq_set) {
        if (le.second == false) {
            err << "leq for equation `" << le.first << "\' is not set\n";
            exit(EXIT_FAILURE);
        }
    }

    for (int i=1; i<=nvar; i++) {
        os << "      do j=2, nt\n";
        os << "            dm(1)\%lvar(j, " << i<< ") = 2 + dm(1)\%lvar(j-1, " <<
            i << ")\n";
        os << "      enddo\n";
    }
    os << "\n";
    for (int i=1; i<=neq; i++) {
        os << "      do j=2, nt\n";
        os << "            dm(1)\%leq(j, " << i<< ") = 2 + dm(1)\%leq(j-1, " <<
            i << ")\n";
        os << "      enddo\n";
    }
    os << "\n";


    os << "      dm(1)\%offset = 0\n";
    os << "      dm(1)\%nas = " << nas << "\n";
    os << "      dm(1)\%nart = " << nart << "\n";
    os << "      dm(1)\%nartt = " << nartt << "\n";
    os << "      idm(1,1)\%natbc = " << natbc << "\n";
    os << "      idm(1,1)\%nattbc = " << nattbc << "\n";

    os << "      allocate(dm(1)\%as(dm(1)\%nas))\n";
    os << "      allocate(dm(1)\%asi(4, dm(1)\%nas))\n";
    os << "      dm(1)\%as = 0d0\n";
    os << "      dm(1)\%asi = 0\n\n";

    os << "      allocate(dm(1)\%art(grd(1)\%nr, nt, dm(1)\%nart))\n";
    os << "      allocate(dm(1)\%arti(4, dm(1)\%nart))\n";
    os << "      dm(1)\%art = 0d0\n";
    os << "      dm(1)\%arti = 0\n\n";

    os << "      allocate(dm(1)\%artt(grd(1)\%nr, nt, nt, dm(1)\%nartt))\n";
    os << "      allocate(dm(1)\%artti(4, dm(1)\%nartt))\n";
    os << "      dm(1)\%artt = 0d0\n";
    os << "      dm(1)\%artti = 0\n\n";

    os << "      allocate(idm(1, 1)\%atbc(nt, idm(1, 1)\%natbc))\n";
    os << "      allocate(idm(1, 1)\%atbci(6, idm(1, 1)\%natbc))\n";
    os << "      idm(1, 1)\%atbc = 0d0\n";
    os << "      idm(1, 1)\%atbci = 0\n\n";

    os << "      allocate(idm(1, 1)\%attbc(nt, nt, idm(1, 1)\%nattbc))\n";
    os << "      allocate(idm(1, 1)\%attbci(6, idm(1, 1)\%nattbc))\n";
    os << "      idm(1, 1)\%attbc = 0d0\n";
    os << "      idm(1, 1)\%attbci = 0\n\n";

    os << "      allocate(dm(1)\%var_name(" << nvar << "))\n";
    os << "      allocate(dm(1)\%var_keep(" << nvar << "))\n";
    os << "      allocate(dm(1)\%eq_name(" << neq << "))\n\n";
    os << "      dm(1)\%var_keep = .true.\n\n";

    int ivar = 1;
    for (auto s: *this->prog->getSymTab()) {
        if (dynamic_cast<ir::Variable *>(s)) {
            os << "      dm(1)\%var_name(" << ivar++ << ") = \'" <<
                s->getName() << "\'\n";
        }
    }

    int ieq = 1;
    for (auto e: *this->prog->getEqs()) {
        os << "      dm(1)\%eq_name(" << ieq++ << ") = \'" << e->getName() << "\'\n";
    }

    for (auto e: *this->prog->getEqs()) {
        os << "      call eqi_" << e->getName() << "()\n";
    }

    os << "      power_max = " << this->powerMax << "\n";
    os << "      a_dim = nt*grd(1)\%nr*dm(1)\%nvar\n";
    os << "      dm(1)\%d_dim = a_dim\n";
    os << "      call find_llmax()\n";
    os << "      print*, 'Dimension of problem: ', a_dim\n";

    os << "      call find_der_range(der_min, der_max)\n";
    os << "      if (allocated(dm(1)\%vect_der)) deallocate(dm(1)\%vect_der)\n";
    os << "      allocate(dm(1)\%vect_der(a_dim, der_min:der_max))\n";

    os << "      if (.not.first_dmat) call clear_derive(dmat(1))\n";
    os << "      call init_derive(dmat(1), grd(1)\%r, grd(1)\%nr, der_max, " <<
        "der_min, orderFD, dertype)\n";
    os << "      first_dmat = .false.\n";
    os << "      call init_var_list()\n";

    os << "      ! lmax = maxval(dm(1)\%lvar)\n";
    os << "      call initialisation(nt, nr, m, lres, llmax)\n\n";
    os << "      call init_avg(dmat(1)\%derive(:, :, 0), dmat(1)\%lbder(0), " << 
        "dmat(1)\%ubder(0))\n\n";

    os << "      do j=1, nt\n";
    os << "            ! zeta(1, j) = 1d0\n";
    os << "            ! r_map(1, j) = 1d0\n";
    os << "      enddo\n";

    for (auto e: *this->prog->getEqs()) {
        os << "      call eq_" << e->getName() << "()\n";
    }

    bool *eqHasModifyL0 = new bool[this->eqs.size()];
    for (int i=0; i<eqs.size(); i++)
        eqHasModifyL0[i] = false;

    std::map<std::string, bool> varLm0Null;
    for (auto v: this->vars)
        varLm0Null[v->getName()] = false;

    for (auto d: *this->prog->getDecls()) {
        if (d->getLM0()) { // this should only be true for lvar(v) = expr no_lm0
            if (auto lvar = dynamic_cast<ir::FuncCall *>(d->getLHS())) {
                if (auto id = dynamic_cast<ir::Identifier *>(lvar->getArgs()->at(0))) {
                    varLm0Null[id->getName()] = true;
                }
            }
        }
    }

    for (auto e: eqs) {
        bool eqNeedModifyL0 = true;
        for (auto t: e.second) {
            if (t->getType() == AS)
                eqNeedModifyL0 = false;
            if (auto fc = dynamic_cast<ir::FuncCall *>(t->expr)) {
                if (std::strstr(fc->getName().c_str(), "Illm") && t->llExpr == NULL) {
                    eqNeedModifyL0 = false;
                }
            }
        }
        if (eqNeedModifyL0) {
            bool modifyCalled = false;
            for (auto t: e.second) {
                if (varLm0Null[t->varName] && modifyCalled == false
#if 1
                        && (e.first == "eq" + t->varName)
#endif
                        ) {
                    t->expr->display(e.first + ": modify_l0 on term: (var: " +
                           t->varName + "), (power: " + std::to_string(t->power) + ")");
                    modifyCalled = true;
                    varLm0Null[t->varName] = false; // do not set twice
                    os << "      call modify_l0(" <<  t->getMatrix(FULL);
                    os << ", ";
                    os << "dm(1)\%lvar(1, " << t->ivar << ")";
                    os << ")\n";
                    break;
                }
            }
            if (modifyCalled == false) {
                err << "could not find a spot to insert modify_l0 in `" <<
                e.first << "\'\n";
                exit(EXIT_FAILURE);
            }
        }

    }
   os << "      do j=1, nt\n";
   os << "            zeta(1, j) = 0d0\n";
   os << "            r_map(1, j) = 0d0\n";
   os << "      enddo\n";

   os << "      call integrales_clear()\n";

   os << "      call dump_coef()\n";


   os << "\n      end subroutine init_a\n";

}

#undef unsupported
