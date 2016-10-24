#include "TopBackEnd.h"
#include "Printer.h"
#include "Analysis.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdlib>

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

void TopBackEnd::emitExpr(ir::Expr *expr, FortranOutput& fo,
        int ivar, int ieq, bool emitLlExpr, std::string bcLocation) {

    ir::Expr *llExpr = extractLlExpr(expr);
    if (llExpr == expr && emitLlExpr == false) {
        fo << "1d0";
        return;
    }
    if (*expr == l) {
        if (emitLlExpr)
            fo << "dm(1)\%lvar(j, " << ivar << ")";
        else
            fo << "1d0";
        return;
    }
    if (*expr == ll) {
        if (emitLlExpr)
            fo << "dm(1)\%lvar(jj, " << ivar << ")";
        else
            fo << "1d0";
        return;
    }
    if (*expr == fp) {
        fo << "1d0";
        return;
    }

    if (auto be = dynamic_cast<ir::BinExpr *>(expr)) {
        fo << "(";
        emitExpr(be->getLeftOp(), fo, ivar, ieq, emitLlExpr, bcLocation);
        switch (be->getOp()) {
            case '^':
                fo << "**";
                break;
            default:
                fo << be->getOp();
        }
        emitExpr(be->getRightOp(), fo, ivar, ieq, emitLlExpr, bcLocation);
        fo << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(expr)) {
        fo << ue->getOp() << "(";
        emitExpr(ue->getExpr(), fo, ivar, ieq, emitLlExpr, bcLocation);
        fo << ")";
    }
    else if (auto fc = dynamic_cast<ir::FuncCall *>(expr)) {
        int narg = 0;
        fo << fc->name;
        fo << "(";
        for (auto a: *fc->getArgs()) {
            if (narg++ > 0)
                fo << ", ";
            emitExpr(a, fo, ivar, ieq, emitLlExpr, bcLocation);
        }
        fo << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(expr)) {
        if (isDef(id->name)) {
            fo << id->name;
            if (bcLocation != "" && isField(id->name)) {
                fo << "(" << bcLocation << ", 1:lres)";
            }
        }
        else {
            err << "`" << id->name << "\' is undefined\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto v = dynamic_cast<ir::Value<int> *>(expr)) {
        fo << v->getValue();
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(expr)) {
        fo << v->getValue() << "d0";
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
        if (e->name == "undef") {
            err << "equation without a name\n";
            exit(EXIT_FAILURE);
        }
        ir::Equation *newEq = new ir::Equation(e->name, eq, NULL, e->getBCs());
        list.push_back(newEq);
    }
    return list;
}

ir::FuncCall *isCoupling(ir::Expr *e) {
    if (auto fc = dynamic_cast<ir::FuncCall *>(e)) {
        if (std::strstr(fc->name.c_str(), "llm")) {
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
    std::string varName = var->name;
    int ivar = this->ivar(var->name);
    if (expr == NULL) {
        if (auto be = dynamic_cast<ir::BinExpr *>(t)) {
            if (be->getOp() != '*') {
                err << "terms should be products...\n";
                exit(EXIT_FAILURE);
            }
            if (auto id = dynamic_cast<ir::Identifier *>(be->getRightOp())) {
                if (isVar(id->name))
                    expr = be->getLeftOp();
                else
                    unsupported(t);
            }
            else if (auto de = dynamic_cast<ir::DiffExpr *>(be->getRightOp())) {
                ir::Identifier *id = dynamic_cast<ir::Identifier *>(de->getExpr());
                if (id != NULL && isVar(id->name))
                    expr = be->getLeftOp();
                else
                    unsupported(t);
            }
            else if (auto id = dynamic_cast<ir::Identifier *>(be->getLeftOp())) {
                if (isVar(id->name))
                    expr = be->getRightOp();
                else
                    unsupported(t);
            }
            else if (auto de = dynamic_cast<ir::DiffExpr *>(be->getLeftOp())) {
                ir::Identifier *id = dynamic_cast<ir::Identifier *>(de->getExpr());
                if (id != NULL && isVar(id->name))
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

#if 0
    if (llExpr) {
        ir::Expr *newNode = new ir::Value<float>(1.0);
        this->prog->replace(llExpr, newNode);
        if (expr == llExpr) {
            expr = newNode;
        }
    }
#endif

    return new Term(expr, llExpr,
            ir::Symbol(var->name),
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
        if (id->name == "r") {
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
        this->eqs[e->name] = std::list<Term *>();
        if (terms) {
            for (auto t: *terms) {
                Term *term = buildTerm(t);
                term->ieq = ieq;
                term->eqName = e->name;
                term->idx = computeTermIndex(term);
                this->eqs[e->name].push_back(term);
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
                    termBC->eqName = e->name;
                    termBC->idx = computeTermIndex(termBC);
                    termBC->eqLoc = getEqLocation(bc);
                    termBC->varLoc = getVarLocation(bc);
                    this->eqs[e->name].push_back(termBC);
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

        if (id->name == "dr") {
            ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(id);
            assert(fc);

            std::string derOrder;
            if (auto order = dynamic_cast<ir::Identifier *>(fc->getChildren()[1])) {
                derOrder = order->name;
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
        if (i->name == "l")
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
        if (this->isVar(id->name)) {
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
                    if (i->name == "fp") {
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
        if (id->name == "fp") {
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

void TopBackEnd::emitTerm(FortranOutput& fo, Term *term) {
    switch(term->getType()) {
        case AS:
        case ART:
            fo << "      " << term->getMatrix(FULL) << " = ";
            emitExpr(term->expr, fo, term->ivar, term->ieq, true);
            fo << "\n";
            break;
        case ARTT:
            if (auto fc = isCoupling(term->expr)) {
                ir::Expr *arg = dynamic_cast<ir::Expr *>(term->expr->getChildren()[0]);
                fo << "      call " << fc->name << "(";
                emitExpr(arg, fo, term->ivar, term->ieq, false);
                fo << ", ";
                fo << " &\n";
                fo << "            & ";
                fo << term->getMatrix(FULL);
                fo << ", &\n";
                fo << "            & ";
                fo << "dm(1)\%leq(1:nt, " << term->ieq << "), ";
                fo << " &\n";
                fo << "            & ";
                fo << "dm(1)\%lvar(1:nt, " << term->ivar << ")";
                fo << ")\n";
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
                fo << "      do j=1, nt\n";
                fo << "            " << term->getMatrix(T) <<
                    " = &\n";
                fo << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else if (term->getType() == ARTT ){
                fo << "      do j=1, nt\n";
                fo << "            " << term->getMatrix(T) <<
                    " = &\n";
                fo << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else {
                err << "llExpr with term not depending on l?\n";
                exit(EXIT_FAILURE);
            }
        }
        else if (term->llExpr->type == "ll") {
            fo << "      do jj=1, nt\n";
            fo << "            " << term->getMatrix(TT) << " = &\n";
            fo << "                  & " << term->getMatrix(TT) <<
                " " << term->llExpr->op << " &\n";
        }
        else {
            err << "\n";
            exit(EXIT_FAILURE);
        }
        emitExpr(term->llExpr->expr, fo, term->llExpr->ivar, term->ieq, true);
        fo << "\n      end do\n";
    }
}

void TopBackEnd::emitTerm(FortranOutput& fo, TermBC *term) {
    std::string bcLoc = term->varLoc;
    switch(term->getType()) {
        case ATBC:
            fo << "      " << term->getMatrix(FULL) << " = ";
            emitExpr(term->expr, fo, term->ivar, term->ieq, true, bcLoc);
            fo << "\n";
            break;
        case ATTBC:
            if (auto fc = isCoupling(term->expr)) {
                ir::Expr *arg = dynamic_cast<ir::Expr *>(term->expr->getChildren()[0]);
                fo << "      call " << fc->name << "bc(";
                emitExpr(arg, fo, term->ivar, term->ieq, false, bcLoc);
                fo << ", ";
                fo << " &" << "\n";
                fo << "            & ";
                fo << term->getMatrix(FULL);
                fo << ",";
                fo << " &" << "\n";
                fo << "            & ";
                fo << "dm(1)\%leq(1:nt, " << term->ieq << "), ";
                fo << " &" << "\n";
                fo << "            & ";
                fo << "dm(1)\%lvar(1:nt, " << term->ivar << ")";
                fo << ")\n";
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
                fo << "      do j=1, nt\n";
                fo << "            " << term->getMatrix(T) <<
                    " = &\n";
                fo << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else if (term->getType() == ATTBC ){
                fo << "      do j=1, nt\n";
                fo << "            " << term->getMatrix(T) <<
                    " = &\n";
                fo << "                  " << term->getMatrix(T) <<
                    " " << term->llExpr->op << " &\n";
            }
            else {
                err << "llExpr with term not depending on l?\n";
                exit(EXIT_FAILURE);
            }
        }
        else if (term->llExpr->type == "ll") {
            fo << "      do jj=1, nt\n";
            fo << "            " << term->getMatrix(TT) <<
                " = &\n";
            fo << "                  " << term->getMatrix(TT) <<
                " " << term->llExpr->op << " &\n";
        }
        else {
            err << "\n";
            exit(EXIT_FAILURE);
        }
        fo << "                  ";
        emitExpr(term->llExpr->expr, fo, term->llExpr->ivar, term->ieq, true, bcLoc);
        fo << "\n      end do\n";
    }
}

void Term::emitInitIndex(FortranOutput& fo) {
    fo << "      " << this->getMatrixI() << "(1, " << this->idx << ") = " <<
        this->power << " ! power\n";
    fo << "      " << this->getMatrixI() << "(2, " << this->idx << ") = " <<
        this->der << " ! der order\n";
    fo << "      " << this->getMatrixI() << "(3, " << this->idx << ") = " <<
        this->ieq << " ! ieq (eq: " << this->eqName << ")\n";
    fo << "      " << this->getMatrixI() << "(4, " << this->idx << ") = " <<
        this->ivar << " ! ivar (var: " << this->varName << ")\n";
}

void TermBC::emitInitIndex(FortranOutput& fo) {
    Term::emitInitIndex(fo);
    fo << "      " << this->getMatrixI() << "(5, " << this->idx << ") = " <<
        this->eqLoc << " ! eq location\n";
    fo << "      " << this->getMatrixI() << "(6, " << this->idx << ") = " <<
        this->varLoc << " ! var location\n";
}

int TopBackEnd::ivar(std::string name) {
    int i = 1;
    for (auto v: this->vars) {
        if (v->name == name)
            return i;
        i++;
    }
    err << "no such variable: `" << name << "\'\n";
    exit(EXIT_FAILURE);
}

int TopBackEnd::ieq(std::string name) {
    int i = 1;
    for (auto e: *this->prog->getEqs()) {
        if (e->name == name)
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

void TopBackEnd::emitUseModel(FortranOutput& fo) {
    fo << "      use model, only: ";
    int n = 0;
    for (auto id: *this->prog->getSymTab()) {
        if (isField(id->name) || isScal(id->name)) {
            if (n > 0) {
                fo << ", ";
            }
            n++;
            fo << id->name;
        }
    }
    fo << "\n";
}

void TopBackEnd::emitCode(FortranOutput& fo) {

    for (auto e: *prog->getEqs()) {
        fo << "!------------------------------------------------------------\n";
        fo << "! Indices for equation " << e->name << "\n";
        fo << "!------------------------------------------------------------\n";
        fo << "      subroutine eqi_" << e->name << "()\n\n";
        emitUseModel(fo);
        fo << "      use inputs\n";
        fo << "      implicit none\n";

        for (auto t: eqs[e->name]) {
            t->emitInitIndex(fo);
            fo << "\n";
        }

        fo << "      end subroutine eqi_" << e->name << "\n";
    }

    for (auto e: *prog->getEqs()) {
        fo << "!------------------------------------------------------------\n";
        fo << "! Coupling coefficients for equation " << e->name << "\n";
        fo << "!------------------------------------------------------------\n";
        fo << "      subroutine eq_" << e->name << "()\n\n";
        emitUseModel(fo);
        fo << "      use inputs\n";
        fo << "      use integrales\n";
        fo << "      implicit none\n";
        fo << "      integer i, j, jj\n";
        for (auto t: eqs[e->name]) {
            if (auto tbc = dynamic_cast<TermBC *>(t)) {
                this->emitTerm(fo, tbc);
            }
            else {
                this->emitTerm(fo, t);
            }
            fo << "\n";
        }
        fo << "      end subroutine eq_" << e->name << "\n\n";
    }

    this->emitInitA(fo);
}

void emitDeclRHS(FortranOutput& fo, ir::Expr *expr) {
    assert(expr);
    if (auto be = dynamic_cast<ir::BinExpr *>(expr)) {
        fo << "(";
        emitDeclRHS(fo, be->getLeftOp());
        fo << be->getOp();
        emitDeclRHS(fo, be->getRightOp());
        fo << ")";
    }
    else if (auto fc = dynamic_cast<ir::FuncCall *>(expr)) {
        int iarg = 0;
        fo << fc->name << "(";
        for (auto arg: *fc->getArgs()) {
            if (iarg++ > 0)
                fo << ", ";
            emitDeclRHS(fo, arg);
        }
        fo << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(expr)) {
        if (id->name == "lvar")
            fo << "dm(1)\%";
        fo << id->name;
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(expr)) {
        fo << val->getValue();
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(expr)) {
        fo << val->getValue() << "d0";
    }
    else {
        err << "skipped term!!\n";
        expr->display("skipped term:");
    }
}

void TopBackEnd::emitDecl(FortranOutput& fo, ir::Decl *decl,
        std::map<std::string, bool>& lvar_set,
        std::map<std::string, bool>& leq_set) {
    assert(decl);
    ir::Expr *lhs = decl->getLHS();
    if (auto fc = dynamic_cast<ir::FuncCall *>(lhs)) {
        if (fc->name == "lvar") {
            ir::Identifier *id = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0));
            assert(id);
            int ivar = this->ivar(id->name);
            fo << "      dm(1)\%lvar(1, " << ivar <<
                ") = ";
            emitDeclRHS(fo, decl->getDef());
            fo << " ! var: " << id->name << "\n";
            lvar_set[id->name] = true;
        }
        else if (fc->name == "leq") {
            ir::Identifier *id = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0));
            assert(id);
            int ieq = this->ieq(id->name);
            fo << "      dm(1)\%leq(1, " << ieq <<
                ") = ";
            emitDeclRHS(fo, decl->getDef());
            fo << " ! eq: " << id->name << "\n";
            leq_set[id->name] = true;
        }
        else {
            err << "function not allowed in LHS\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(lhs)) {
        fo << "      " << id->name << " = ";
        emitDeclRHS(fo, decl->getDef());
        fo << "\n";
    }
    else {
        err << "unsupported definition: " << *lhs << "\n";
        exit(EXIT_FAILURE);
    }
}

void TopBackEnd::emitInitA(FortranOutput& fo) {
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
            lvar_set[s->name] = false;
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
            inputs << "    " << type << ", save :: " << p->name << "\n";
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
                    p->name << "\'\n";
                exit(EXIT_FAILURE);
            }

            inputs << "        " << p->name << " = fetch('" << p->name <<
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
        leq_set[e->name] = false;;
    }

    fo << "\n      subroutine init_a()\n\n";

    fo << "      use model\n";
    fo << "      use integrales\n\n";

    fo << "      implicit none\n";
    fo << "      integer j, der_min, der_max\n\n";

    fo << "      grd(1)%mattype = mattype\n";
    fo << "      nr => grd(1)\%nr\n";
    fo << "      if (allocated(dm)) then\n";
    fo << "            call clear_all()\n";
    fo << "      endif\n\n";

    fo << "      allocate(dm(1))\n";
    fo << "      allocate(dmat(1))\n";
    fo << "      allocate(idm(1, 1))\n\n";


    fo << "      dm(1)\%nvar = " << nvar << "\n";
    fo << "      allocate(dm(1)\%lvar(nt, " << nvar << "))\n";
    fo << "      allocate(dm(1)\%leq(nt, " << neq << "))\n\n";

    for (auto d: *prog->getDecls()) {
        emitDecl(fo, d, lvar_set, leq_set);
    }
    for (auto v: vars) {
        fo << "      dm(1)%lvar(1, " << ivar(v->name) << ") = ";
        if (v->vectComponent == 3)
            fo << "abs(m) + 1 - iparity\n";
        else
            fo << "abs(m) + iparity\n";
    }

    // for (auto lv: lvar_set) {
    //     if (lv.second == false) {
    //         err << "lvar for variable `" << lv.first << "\' is not set\n";
    //         exit(EXIT_FAILURE);
    //     }
    // }
    for (auto le: leq_set) {
        if (le.second == false) {
            err << "leq for equation `" << le.first << "\' is not set\n";
            exit(EXIT_FAILURE);
        }
    }

    for (int i=1; i<=nvar; i++) {
        fo << "      do j=2, nt\n";
        fo << "            dm(1)\%lvar(j, " << i << ") = 2 + dm(1)\%lvar(j-1, " <<
            i << ")\n";
        fo << "      enddo\n";
    }
    fo << "\n";
    for (int i=1; i<=neq; i++) {
        fo << "      do j=2, nt\n";
        fo << "            dm(1)\%leq(j, " << i << ") = 2 + dm(1)\%leq(j-1, " <<
            i << ")\n";
        fo << "      enddo\n";
    }
    fo << "\n";


    fo << "      dm(1)\%offset = 0\n";
    fo << "      dm(1)\%nas = " << nas << "\n";
    fo << "      dm(1)\%nart = " << nart << "\n";
    fo << "      dm(1)\%nartt = " << nartt << "\n";
    fo << "      idm(1,1)\%natbc = " << natbc << "\n";
    fo << "      idm(1,1)\%nattbc = " << nattbc << "\n";

    fo << "      allocate(dm(1)\%as(dm(1)\%nas))\n";
    fo << "      allocate(dm(1)\%asi(4, dm(1)\%nas))\n";
    fo << "      dm(1)\%as = 0d0\n";
    fo << "      dm(1)\%asi = 0\n\n";

    fo << "      allocate(dm(1)\%art(grd(1)\%nr, nt, dm(1)\%nart))\n";
    fo << "      allocate(dm(1)\%arti(4, dm(1)\%nart))\n";
    fo << "      dm(1)\%art = 0d0\n";
    fo << "      dm(1)\%arti = 0\n\n";

    fo << "      allocate(dm(1)\%artt(grd(1)\%nr, nt, nt, dm(1)\%nartt))\n";
    fo << "      allocate(dm(1)\%artti(4, dm(1)\%nartt))\n";
    fo << "      dm(1)\%artt = 0d0\n";
    fo << "      dm(1)\%artti = 0\n\n";

    fo << "      allocate(idm(1, 1)\%atbc(nt, idm(1, 1)\%natbc))\n";
    fo << "      allocate(idm(1, 1)\%atbci(6, idm(1, 1)\%natbc))\n";
    fo << "      idm(1, 1)\%atbc = 0d0\n";
    fo << "      idm(1, 1)\%atbci = 0\n\n";

    fo << "      allocate(idm(1, 1)\%attbc(nt, nt, idm(1, 1)\%nattbc))\n";
    fo << "      allocate(idm(1, 1)\%attbci(6, idm(1, 1)\%nattbc))\n";
    fo << "      idm(1, 1)\%attbc = 0d0\n";
    fo << "      idm(1, 1)\%attbci = 0\n\n";

    fo << "      allocate(dm(1)\%var_name(" << nvar << "))\n";
    fo << "      allocate(dm(1)\%var_keep(" << nvar << "))\n";
    fo << "      allocate(dm(1)\%eq_name(" << neq << "))\n\n";
    fo << "      dm(1)\%var_keep = .true.\n\n";

    int ivar = 1;
    for (auto s: *this->prog->getSymTab()) {
        if (dynamic_cast<ir::Variable *>(s)) {
            fo << "      dm(1)\%var_name(" << ivar++ << ") = \'" <<
                s->name << "\'\n";
        }
    }

    int ieq = 1;
    for (auto e: *this->prog->getEqs()) {
        fo << "      dm(1)\%eq_name(" << ieq++ << ") = \'" << e->name << "\'\n";
    }

    for (auto e: *this->prog->getEqs()) {
        fo << "      call eqi_" << e->name << "()\n";
    }

    fo << "      power_max = " << this->powerMax << "\n";
    fo << "      a_dim = nt*grd(1)\%nr*dm(1)\%nvar\n";
    fo << "      dm(1)\%d_dim = a_dim\n";
    fo << "      call find_llmax()\n";
    fo << "      print*, 'Dimension of problem: ', a_dim\n";

    fo << "      call find_der_range(der_min, der_max)\n";
    fo << "      if (allocated(dm(1)\%vect_der)) deallocate(dm(1)\%vect_der)\n";
    fo << "      allocate(dm(1)\%vect_der(a_dim, der_min:der_max))\n";

    fo << "      if (.not.first_dmat) call clear_derive(dmat(1))\n";
    fo << "      call init_derive(dmat(1), grd(1)\%r, grd(1)\%nr, der_max, " <<
        "der_min, orderFD, dertype)\n";
    fo << "      first_dmat = .false.\n";
    fo << "      call init_var_list()\n";

    fo << "      ! lmax = maxval(dm(1)\%lvar)\n";
    fo << "      call initialisation(nt, nr, m, lres, llmax)\n\n";
    fo << "      call init_avg(dmat(1)\%derive(:, :, 0), dmat(1)\%lbder(0), " << 
        "dmat(1)\%ubder(0))\n\n";

    fo << "      do j=1, nt\n";
    fo << "            ! zeta(1, j) = 1d0\n";
    fo << "            ! r_map(1, j) = 1d0\n";
    fo << "      enddo\n";

    for (auto e: *this->prog->getEqs()) {
        fo << "      call eq_" << e->name << "()\n";
    }

    bool *eqHasModifyL0 = new bool[this->eqs.size()];
    for (int i=0; i<eqs.size(); i++)
        eqHasModifyL0[i] = false;

    std::map<std::string, bool> varLm0Null;
    for (auto v: this->vars) {
        if (v->vectComponent > 1)
            varLm0Null[v->name] = true;
        else
            varLm0Null[v->name] = false;
    }

    for (auto e: eqs) {
        bool eqNeedModifyL0 = true;
        for (auto t: e.second) {
            if (t->getType() == AS)
                eqNeedModifyL0 = false;
            if (auto fc = dynamic_cast<ir::FuncCall *>(t->expr)) {
                if (std::strstr(fc->name.c_str(), "Illm") && t->llExpr == NULL) {
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
                    // t->expr->display(e.first + ": modify_l0 on term: (var: " +
                    //        t->varName + "), (power: " + std::to_string(t->power) + ")");
                    modifyCalled = true;
                    varLm0Null[t->varName] = false; // do not set twice
                    fo << "      call modify_l0(" <<  t->getMatrix(FULL);
                    fo << ", ";
                    fo << "dm(1)\%lvar(1, " << t->ivar << ")";
                    fo << ")\n";
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
   fo << "      do j=1, nt\n";
   fo << "            zeta(1, j) = 0d0\n";
   fo << "            r_map(1, j) = 0d0\n";
   fo << "      enddo\n";

   fo << "      call integrales_clear()\n";

   fo << "      ! call dump_coef()\n";


   fo << "\n      end subroutine init_a\n";

}

std::string escapeLaTeX(const std::string& str) {
    std::string ret(str);
    int n = ret.find("_");
    if (n >= 0) {
        ret.replace(n, 1, "\\_");
    }
    return ret;
}

std::string getIntegralLaTeX(const std::string& coupling) {
    if (coupling == "Illm") {
        return "Y_{l'}^m \\{Y_l^m\\}^*";
    }
    if (coupling == "Jllm") {
        return "\\partial_{\\theta} Y_{l'}^m \\{Y_l^m\\}^*";
    }
    if (coupling == "Kllm") {
        return "D_{\\phi} Y_{l'}^m \\{Y_l^m\\}^*";
    }
    if (coupling == "Jllmc") {
        return "Y_{l'}^m \\partial_{\\theta} \\{Y_l^m\\}^*";
    }
    if (coupling == "Lllm") {
        return "\\partial_{\\theta} Y_{l'}^m \\partial_{\\theta} \\{Y_l^m\\}^*";
    }
    if (coupling == "Mllm") {
        return "D_{\\phi} Y_{l'}^m \\partial_{\\theta} \\{Y_l^m\\}^*";
    }
    if (coupling == "Kllmc") {
        return "Y_{l'}^m D_{\\phi} \\{Y_l^m\\}^*";
    }
    if (coupling == "Mllmc") {
        return "\\partial_{\\theta} Y_{l'}^m D_{\\phi} \\{Y_l^m\\}^*";
    }
    if (coupling == "Nllm") {
        return "D_{\\phi} Y_{l'}^m D_{\\phi} \\{Y_l^m\\}^*";
    }
    err << "unknown coupling integral \"" << coupling << "\"\n";
    exit(EXIT_FAILURE);
}

void emitTermLaTeX(LatexOutput& lo, Term *term) {
    auto emitPower = [&lo, term] () {
        if (term->power > 0)
            lo << "fp";
        if (term->power > 1)
            lo << "^" << term->power;
        if (term->power > 0)
            lo << " * ";
    };

    std::function<void (ir::Expr *)> emitExpr = [&lo, term, &emitExpr] (ir::Expr *e) {
        if (*e == l) {
            lo << "l";
            return;
        }
        if (*e == ll) {
            lo << "l'";
            return;
        }
        if (*e == fp) {
            lo << "1";
            return;
        }
        if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
            if (ue->getOp() != '-')
                unsupported(ue);
            lo << "-";
            emitExpr(ue->getExpr());
        }
        else if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
            if (be->getOp() == '/') {
                lo << "\\frac{";
                emitExpr(be->getLeftOp());
                lo << "}{";
                emitExpr(be->getRightOp());
                lo << "}";
            }
            else if (be->getOp() == '^') {
                emitExpr(be->getLeftOp());
                lo << "^{";
                emitExpr(be->getRightOp());
                lo << "}";

            }
            else {
                if (be->getLeftOp()->priority < be->priority) {
                    lo << "(";
                }
                emitExpr(be->getLeftOp());
                if (be->getLeftOp()->priority < be->priority)
                    lo << ")";
                if (be->getOp() == '*')
                    lo << "\\ ";
                else
                    lo << be->getOp();

                if (be->getRightOp()->priority < be->priority) {
                    lo << "(";
                }
                emitExpr(be->getRightOp());
                if (be->getRightOp()->priority < be->priority)
                    lo << ")";
            }
        }
        else if (auto fc = isCoupling(e)) {
            lo << "\\iint_{4 \\pi}";
            int narg = 0;
            for (auto a: *fc->getArgs()) {
                if (narg++ > 0)
                    lo << ", ";
                emitExpr(a);
            }
            lo << getIntegralLaTeX(fc->name);
            lo << "\\ d\\Omega";
        }
        else if (auto fc = dynamic_cast<ir::FuncCall *>(e)) {
            lo << escapeLaTeX(fc->name) << "\\Big(";
            int narg = 0;
            for (auto a: *fc->getArgs()) {
                if (narg++ > 0)
                    lo << ", ";
                emitExpr(a);
            }
            lo << "\\Big)";
        }
        else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
            lo << escapeLaTeX(id->name);
        }
        else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
            lo << val->getValue();
        }
        else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
            lo << val->getValue();
        }
        else {
            unsupported(e);
            exit(EXIT_FAILURE);
        }
    };

    auto emitVar = [&lo, term] () {
        std::string& der = term->der;
        if (der == "0") {
            lo << escapeLaTeX(term->var.name);
        }
        else {
            int der = atoi(term->der.c_str());
            if (der == 1) {
                lo << "\\frac{\\partial " << escapeLaTeX(term->var.name) <<
                    "}{\\partial r}";
            }
            else {
                lo << "\\frac{\\partial^{" << term->der << "} " <<
                    escapeLaTeX(term->var.name) <<
                    "}{\\partial r^{" << term->der << "}}";
            }
        }
    };

    emitPower();
    if (auto ue = dynamic_cast<ir::UnaryExpr *>(term->expr))
        emitExpr(ue->getExpr());
    else
        emitExpr(term->expr);
    lo << " &\\ * ";
    emitVar();
}

void TopBackEnd::emitLaTeX(LatexOutput& lo) {
    lo << "\\documentclass[10pt]{article}\n";
    lo << "\\usepackage{amsmath}\n";
    lo << "\\begin{document}\n";
    lo << "Filename: \\texttt{" << escapeLaTeX(this->prog->filename) << "}";
    for (auto e: *prog->getEqs()) {
        lo << "\\subsection*{" << escapeLaTeX(e->name) << "}\n";
        lo << "\\begin{align*}\n";
        int n = 0;
        for (auto t: eqs[e->name]) {
            if (t->getType() == AS ||
                    t->getType() == ART ||
                    t->getType() == ARTT) {
                if (auto ue = dynamic_cast<ir::UnaryExpr *>(t->expr)) {
                    if (ue->getOp() != '-') {
                        unsupported(ue);
                    }
                    lo << "&-";
                    lo << "\\\\\n";
                }
                else {
                    if (n > 0) {
                        lo << "&+";
                        lo << "\\\\\n";
                    }
                }
                emitTermLaTeX(lo, t);
                n++;
            }
        }
        lo << " & = 0\n";
        lo << "\\end{align*}\n";
    }
    lo << "\\end{document}\n";
}

#undef unsupported
