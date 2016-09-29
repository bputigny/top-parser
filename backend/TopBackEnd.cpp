#include "TopBackEnd.h"
#include "Printer.h"
#include "Analysis.h"

#include <cassert>

TopBackEnd::TopBackEnd(ir::Program *p) {
    this->prog = p;
    this->nas = 0;
    this->nartt = 0;
    this->nart = 0;
    this->nvar = 0;
    this->natbc = 0;
    this->nattbc = 0;

    // for (auto d: *prog->getDecls()) {
    //     if (auto id = dynamic_cast<ir::Identifier *>(d->getLHS())) {
    //         if (!isDef(id->getName())) {
    //             // localVars.push_back(d);
    //         }
    //     }
    //     else {
    //         err() << "only variable definition are allowed\n";
    //         exit(EXIT_FAILURE);
    //     }
    // }
}

TopBackEnd::~TopBackEnd() { }

#define unsupported(e) { \
    err() << __FILE__ << ":" << __LINE__ << " unsupported expr\n"; \
    e->display("unsupported expr:"); \
    exit(EXIT_FAILURE); \
}

// this takes derivative expressions (e.g., u''') and fold them into DiffExpr
// (e.g., DiffExpr(u, r, 3))
// this also transform FuncCall (dr(u, 3) into the specialized DiffExpr(u, r, 3)
void TopBackEnd::simplify(ir::Expr *expr) {
    Analysis<ir::Identifier> a;
    assert(expr);
    auto foldDer = [this] (ir::Identifier *id) {
        ir::Expr *root = id;
        ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(id->getParent());
        int order = 0;

        if (id->getName() == "l") return;

        while (ue) {
            if (ue->getOp() == '\'') {
                order++;
                root = ue;
                ue = dynamic_cast<ir::UnaryExpr *>(ue->getParent());
            }
            else {
                break;
            }
        }
        if (order > 0) {
            ir::Node *newNode = new ir::DiffExpr(id,
                    new ir::Identifier("r"),
                    std::to_string(order));
            prog->replace(root, newNode);
        }
    };
    auto foldDr = [this, &expr] (ir::Identifier *id) {
        if (id->getName() != "l") {
            std::string derOrder = this->findDerivativeOrder(id);
            if (std::stoi(derOrder) > 0) {
                ir::Node *root = id;
                for (int i=0; i<std::stoi(derOrder); i++) {
                    assert(root->getParent());
                    root = root->getParent();
                }
                assert(root);
                ir::Node *newNode = new ir::DiffExpr(id,
                        new ir::Identifier("r"),
                        derOrder);
                prog->replace(root, newNode);
            }
        }
        if (id->getName() == "dr") {
            ir::FuncCall *fc = dynamic_cast<ir::FuncCall *>(id);
            assert(fc);

            std::string derOrder;
            if (auto *order = dynamic_cast<ir::Identifier *>(fc->getChildren()[1])) {
                derOrder = order->getName();
            }
            else if (auto *order = dynamic_cast<ir::Value<int> *>(fc->getChildren()[1])) {
                derOrder = std::to_string(order->getValue());
            }
            else {
                err() << "derivative order should be an integer or a variable...";
                unsupported(fc);
            }
            if (auto *derVar = dynamic_cast<ir::Identifier *>(fc->getChildren()[0])) {
                ir::Node *newNode = new ir::DiffExpr(
                        derVar,
                        new ir::Identifier("r"),
                        derOrder);
                prog->replace(fc, newNode);
            }
            else {
                err() << "derivative of expression not yet supported...";
                unsupported(fc);
            }
        }
    };
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
                // delete(rRight);
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
            case '\'':
                r = new std::vector<ir::Expr *>();
                r->push_back(ue);
                return r;
            default:
                err() << "unsupported unary operator `" << ue->getOp() << "\'\n";
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
        warn() << "skipped term\n";
        e->display("skipped term:");
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

std::string TopBackEnd::findDerivativeOrder(ir::Identifier *id) {
    assert(id);
    int order = 0;
    ir::Expr *e = id;
    assert(e);
    if (e->getParent() == NULL)
        return "0";
    assert(e->getParent());
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
        else if (auto de = dynamic_cast<ir::DiffExpr *>(e->getParent())) {
            return de->getOrder();
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
    else if (dynamic_cast<ir::DiffExpr *>(e)) {
        return 0;
    }
    else {
        unsupported(e);
    }
    return p;
}

void TopBackEnd::emitScalarBCExpr(Output& os, ir::Expr *e, int ivar) {
    assert(e);
    e->setParents();
    if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (findVar(id)) {
            os << "1d0";
        }
        else {
            if (isDef(id->getName())) {
                os << id->getName();
            }
            else {
                if (id->getName() == "l") {
                    os << "dm(1)%lvar(j, " << ivar << ")";
                }
                else {
                    err() << (*id) << " is undefined\n";
                    unsupported(e);
                }
            }
        }
    }
    else if (auto de = dynamic_cast<ir::DiffExpr *>(e)) {
        if (findVar(de)) {
            os << "1d0";
        }
        else {
            unsupported(e);
        }
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue() << "d0";
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue() << "d0";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() == '-') {
            os << "-(";
            emitScalarBCExpr(os, ue->getExpr(), ivar);
            os << ")";
        }
        else {
            unsupported(e);
        }
    }
    else if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitScalarBCExpr(os, be->getLeftOp(), ivar);
        os << be->getOp();
        emitScalarBCExpr(os, be->getRightOp(), ivar);
        os << ")";
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitScalarExpr(Output& os, ir::Expr *e) {
    assert(e);
    e->setParents();
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
            err() << "no derivative terms allowed in scalar expr\n";
            e->display("unsupported derivative term:");
            exit(EXIT_FAILURE);
        }
        os << "-(";
        emitScalarExpr(os, ue->getExpr());
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (id->getName() == "fp" || this->isVar(id->getName())) {
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
        os << val->getValue() << "d0";
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue() << "d0";
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitExpr(Output& os, ir::Expr *e, bool isBC, int bcLoc) {
    assert(e);
    if (auto be = dynamic_cast<ir::BinExpr *>(e)) {
        os << "(";
        emitExpr(os, be->getLeftOp(), isBC, bcLoc);
        if (be->getOp() == '^')
            os << "**";
        else
            os << be->getOp();
        emitExpr(os, be->getRightOp(), isBC, bcLoc);
        os << ")";
    }
    else if (auto ue = dynamic_cast<ir::UnaryExpr *>(e)) {
        if (ue->getOp() != '-') {
            err() << "no derivative terms allowed here (expr isBC: " << isBC << ")\n";
            unsupported(e);
            exit(EXIT_FAILURE);
        }
        os << "-(";
        emitExpr(os, ue->getExpr(), isBC, bcLoc);
        os << ")";
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(e)) {
        if (!isDef(id->getName())) {
            err() << "`" << id->getName() << "\' is undefined\n";
            exit(EXIT_FAILURE);
        }
        os << id->getName();
        if (isBC && isField(id->getName())) {
            if (bcLoc != 0 && bcLoc != 1) {
                err() << "unsupported BC location:" << bcLoc << "\n";
                exit(EXIT_FAILURE);
            }
            std::string loc = (bcLoc==0)?"1":"nr";
            os << "(" << loc << ", :)";
        }
    }
    else if (auto val = dynamic_cast<ir::Value<int> *>(e)) {
        os << val->getValue() << "d0";
    }
    else if (auto val = dynamic_cast<ir::Value<float> *>(e)) {
        os << val->getValue() << "d0";
    }
    else {
        unsupported(e);
    }
}

void TopBackEnd::emitLVarExpr(Output& os, ir::Expr *e, int ivar) {
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

void TopBackEnd::emitBCTerm(Output& os, ir::Expr *term, int ieq, int eqLoc, int varLoc) {
    assert(term);
    term->setParents();
    ir::Identifier *var = this->findVar(term);
    int power = 0;
    std::string derOrder = "0";
    ir::FuncCall *couplingIntergal = NULL;
    int ivar = 0;
    std::string eqLocStr, varLocStr;

    if (eqLoc == 0) {
        eqLocStr = "1";
    }
    else if (eqLoc == 1) {
        eqLocStr = "nr";
    }
    else {
        err() << "unsupported equation location\n";
        exit(EXIT_FAILURE);
    }

    if (varLoc == 0) {
        varLocStr = "1";
    }
    else if (varLoc == 1) {
        varLocStr = "nr";
    }
    else {
        err() << "unsupported BC location\n";
        exit(EXIT_FAILURE);
    }

    if (var) {
        power = this->findPower(term);
        derOrder = this->findDerivativeOrder(var);
        couplingIntergal = this->findCoupling(term);
        ivar = this->ivar(var->getName());
        int n = 0;
        std::string mat = "";

        if (couplingIntergal) {
            this->nattbc++;
            n = this->nattbc;
            mat = "attbci";
        }
        else {
            this->natbc++;
            n = this->natbc;
            mat = "atbci";
        }
        os << "\tidm(1, 1)\%" << mat << "(1, " << n << ") = "
            << power << " ! power\n";
        os << "\tidm(1, 1)\%" << mat << "(2, " << n << ") = "
            << derOrder << " ! der\n";
        os << "\tidm(1, 1)\%" << mat << "(3, " << n << ") = "
            << ieq << " ! ieq\n";
        os << "\tidm(1, 1)\%" << mat << "(4, " << n << ") = "
            << ivar << " ! ivar (var: " << var->getName() << ")\n";
        os << "\tidm(1, 1)\%" << mat << "(5, " << n << ") = "
            << eqLocStr << " ! eqloc\n";
        os << "\tidm(1, 1)\%" << mat << "(6, " << n << ") = "
            << varLocStr << " ! varloc\n";

        if (couplingIntergal) {
            ir::Expr *integralExpr = couplingIntergal->getArgs()->at(0);
            ir::Expr *expr = (ir::Expr *) integralExpr->copy();
            assert(couplingIntergal->getArgs()->size() == 1);
            os << "\tcall " << couplingIntergal->getName() << "bc( &\n\t\t";
            emitExpr(os, expr, true, varLoc);
            os << ", &\n";
            os << "\t\tidm(1, 1)\%attbc(1:nt, 1:nt, " <<
                nattbc << "), &\n";
            os << "\t\tdm(1)\%leq(1:nt, " << ieq << "), &\n";
            os << "\t\tdm(1)\%lvar(1:nt, " << ivar << ")";
            os << ")\n";
        }
        else {
            os << "\tidm(1, 1)\%atbc(1:nt, " << this->natbc << ") = ";
            emitScalarBCExpr(os, term, ivar);
            os << "\n";
        }
    }
    else {
        err() << "no variable in BC\n";
        term->display("term without var:");
        exit(EXIT_FAILURE);
    }
}

void TopBackEnd::emitTerm(Output& os, ir::Expr *term, int ieq) {
    assert(term);
    term->setParents();
    ir::Identifier *var = this->findVar(term);
    int power = 0;
    std::string derOrder = "0";
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
            // expr = (ir::Expr *) integralExpr->copy();
            expr = (ir::Expr *) integralExpr;
            assert(couplingIntergal->getArgs()->size() == 1);
        }
        else {
            // expr = (ir::Expr *) term->copy();
            expr = (ir::Expr *) term;
        }
        expr->setParents();
        llExpr = extractLlExpr(expr);
        ir::Node *newNode = new ir::Value<float>(1.0);
        if (llExpr) {
            prog->replace(llExpr, newNode);
        }

        if (couplingIntergal) {
            this->nartt++;
            os << "\tdm(1)\%artti(1, " << nartt << ") = " << power    << " ! power\n";
            os << "\tdm(1)\%artti(2, " << nartt << ") = " << derOrder << " ! derivative\n";
            os << "\tdm(1)\%artti(3, " << nartt << ") = " << ieq      << " ! ieq\n";
            os << "\tdm(1)\%artti(4, " << nartt << ") = " << ivar     << " ! ivar (var: " << var->getName() << ")\n";
            os << "\tcall " << couplingIntergal->getName() << "( &\n\t\t";
            emitExpr(os, expr);
            os << ", &\n";
            os << "\t\tdm(1)\%artt(1:grd(1)\%nr, 1:nt, 1:nt, " << nartt << "), &\n";
            os << "\t\tdm(1)\%leq(1:nt, " << ieq << "), &\n";
            os << "\t\tdm(1)\%lvar(1:nt, " << ivar << ")";
            os << ")\n";
        }
        if (llExpr) {
            prog->replace(newNode, llExpr); // set back llExpr
            // newNode->clear();
            // delete newNode;
            auto replaceL = [this] (ir::Identifier *id) {
                if (id->getName() == "l") {
                    ir::Node *newNode = new ir::Identifier("j");
                    prog->replace(id, newNode);
                }
            };
            auto replaceLp = [this] (ir::UnaryExpr *ue) {
                if (ue->getOp() == '\'') {
                    if (auto id = dynamic_cast<ir::Identifier *>(ue->getExpr())) {
                        if (id->getName() == "l")
                            assert(ue->getParent());
                        ir::Node *newNode = new ir::Identifier("jj");
                        prog->replace(ue, newNode);
                    }
                }
            };

            Analysis<ir::UnaryExpr> alp;
            alp.run(replaceLp, llExpr);

            Analysis<ir::Identifier> al;
            al.run(replaceL, llExpr);

            std::vector<ir::Identifier *> ids = getIds(llExpr);
            if (ids.size() != 1) {
                err() << "ll expr should only have a single identifier `l\'\n";
            }
            assert(ids.size() == 1);

            std::string idx = ids[0]->getName();
            char op = '*';

            if (auto be = dynamic_cast<ir::BinExpr *>(llExpr->getParent())) {
                if (be->getOp() != '*') {
                    err() << "llExpr should be factors only\n";
                }
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
                os << "\t\tdm(1)\%" << mat << "(1:grd(1)\%nr, " << idx << ", ";
                if (mat == "artt")
                    os << "1:nt, ";
                os << nmat << ") = &\n\t\t\t";
                os << "dm(1)\%" << mat << "(1:grd(1)\%nr, " << idx << ", ";
                if (mat == "artt")
                    os << "1:nt, ";
                os << nmat << ") " << op << " &\n\t\t";
            }
            else if (idx == "jj") {
                os << "\t\tdm(1)\%" << mat << "(1:grd(1)\%nr, ";
                if (mat == "artt")
                    os << "1:nt, ";
                os << idx << ", " <<
                    nmat << ") = &\n\t\t\t";
                os << "dm(1)\%" << mat << "(1:grd(1)\%nr, ";
                if (mat == "artt")
                    os << "1:nt, ";
                os << idx << ", " <<
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
        // delete(expr);
    }
    else { // scalar term
        this->nas++;
        os << "\tdm(1)\%asi(1, " << nas << ") = " << power    << " ! power\n";
        os << "\tdm(1)\%asi(2, " << nas << ") = " << derOrder << " ! derivative\n";
        os << "\tdm(1)\%asi(3, " << nas << ") = " << ieq      << " ! ieq\n";
        os << "\tdm(1)\%asi(4, " << nas << ") = " << ivar     << " ! ivar (var: " << var->getName() << ")\n";
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

int TopBackEnd::ieq(std::string name) {
    int i = 1;
    for (auto e: *this->prog->getEqs()) {
        if (e->getName() == name)
            return i;
        i++;
    }
    err() << "no such equation: `" << name << "\'\n";
    exit(EXIT_FAILURE);
}

void TopBackEnd::emitCode(Output& os) {
    this->nartt = 0;
    this->nart = 0;
    this->nas = 0;
    this->nvar = 0;

    for (auto sym: *prog->getSymTab()) {
        if (auto var = dynamic_cast<ir::Variable *>(sym)) {
            this->vars.push_back(var);
            nvar++;
        }
    }

    int ieq = 0;
    for (auto e:*prog->getEqs()) {
        ieq++;
        this->simplify(e->getLHS());
        this->simplify(e->getRHS());
        ir::Expr *lhs = e->getLHS();
        ir::Expr *rhs = e->getRHS();
        ir::Expr *eq = new ir::BinExpr(rhs, '-', lhs);
        if (e->getName() == "undef") {
            err() << "equation without a name\n";
            exit(EXIT_FAILURE);
        }
        os << "\tsubroutine eq_" << e->getName() << "()\n\n";
        os << "\tuse model, only: ";
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
        os << "\tuse inputs\n";
        os << "\tuse integrales\n";
        os << "\timplicit none\n\n";
        os << "\tinteger i, j, jj\n\n";

        std::vector<ir::Expr *> *terms = this->splitIntoTerms(eq);
        for (auto t: *terms) {
            this->emitTerm(os, t, ieq);
        }
        // delete(terms);

        os << "\tend subroutine eq_" << e->getName() << "\n";

        os << "\n\tsubroutine bc_" << e->getName() << "()\n\n";
        os << "\tuse model\n";
        os << "\tuse inputs\n";
        os << "\tuse integrales\n";
        os << "\timplicit none\n\n";
        os << "\tinteger i, j, jj\n\n";

        for (auto bc: *e->getBCs()) {
            this->simplify(bc->getCond()->getLHS());
            this->simplify(bc->getCond()->getRHS());
            this->simplify(bc->getLoc()->getLHS());
            this->simplify(bc->getLoc()->getRHS());
            this->emitBC(os, bc, ieq);
        }

        os << "\tend subroutine bc_" << e->getName() << "\n\n";
    }

    this->emitInitA(os);
}

void TopBackEnd::emitBC(Output& os, ir::BC *bc, int ieq) {
    ir::Expr *lhs = bc->getCond()->getLHS();
    ir::Expr *rhs = bc->getCond()->getRHS();
    ir::Expr *eq = NULL;

    if (auto v = dynamic_cast<ir::Value<int> *>(rhs)) {
        if (v->getValue() == 0) {
            eq = (ir::Expr *) lhs->copy();
        }
        else
            eq = new ir::BinExpr(lhs, '-', rhs);
    }
    else if (auto v = dynamic_cast<ir::Value<float> *>(rhs)) {
        if (v->getValue() == 0)
            eq = (ir::Expr *) lhs->copy();
        else
            eq = new ir::BinExpr(lhs, '-', rhs);
    }
    else {
        eq = new ir::BinExpr(lhs, '-', rhs);
    }

    eq->setParents();

    std::vector<ir::Expr *> *terms = this->splitIntoTerms(eq);
    assert(terms);
    for (auto t: *terms) {
        if (auto id = dynamic_cast<ir::Identifier *>(bc->getLoc()->getLHS())) {
            if (id->getName() == "r") {
                if (auto v = dynamic_cast<ir::Value<int> *>(bc->getLoc()->getRHS())) {
                    int bcLoc = v->getValue();
                    this->emitBCTerm(os, t, ieq, bc->getEqLoc(), bcLoc);
                }
            }
            else {
                err() << "BC should be `r = 0\' or `r = 1\' ";
                unsupported(t);
            }
        }
        else {
            err() << "unsupported BC location\n";
            bc->getLoc()->display("unsupported BC location");
            exit(EXIT_FAILURE);
        }
    }
    // delete(terms);
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
        err() << "skipped term!!\n";
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
            os << "\tdm(1)\%lvar(1, " << ivar <<
                ") = ";
            emitDeclRHS(os, decl->getDef());
            os << " ! var: " << id->getName() << "\n";
            lvar_set[id->getName()] = true;
        }
        else if (fc->getName() == "leq") {
            ir::Identifier *id = dynamic_cast<ir::Identifier *>(fc->getArgs()->at(0));
            assert(id);
            int ieq = this->ieq(id->getName());
            os << "\tdm(1)\%leq(1, " << ieq <<
                ") = ";
            emitDeclRHS(os, decl->getDef());
            os << " ! eq: " << id->getName() << "\n";
            leq_set[id->getName()] = true;
        }
        else {
            err() << "function not allowed in LHS\n";
            exit(EXIT_FAILURE);
        }
    }
    else if (auto id = dynamic_cast<ir::Identifier *>(lhs)) {
        os << "\t" << id->getName() << " = ";
        emitDeclRHS(os, decl->getDef());
        os << "\n";
    }
    else {
        err() << "unsupported definition: " << *lhs << "\n";
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
    inputs << "    use mod_grid, only: grd, nt, init_grid\n";

    inputs << "    character*(4), save :: mattype\n";
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
            else {
                type = "type error";
            }
            inputs << "    " << type << ", save :: " << p->getName() << "\n";
        }
    }
    inputs << "\n";

    inputs << "contains\n\n";

    inputs << "    subroutine read_inputs(dati)\n";
    inputs << "    character(len=*), intent(in) :: dati\n";
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine write_inputs(i)\n";
    inputs << "    integer, intent(in) :: i\n";
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine write_stamp(i)\n";
    inputs << "    integer, intent(in) :: i\n";
    inputs << "    end subroutine\n\n";

    inputs << "    subroutine init_default()\n";
    inputs << "    end subroutine\n\n";

    inputs << "end module inputs\n";
    inputs.close();

    for (auto e: *prog->getEqs()) {
        leq_set[e->getName()] = false;;
    }

    os << "\n\tsubroutine init_a()\n\n";

    os << "\tuse model\n";
    os << "\tuse integrales\n\n";

    os << "\timplicit none\n";
    os << "\tinteger j\n\n";

    os << "\tgrd(1)%mattype = mattype\n";
    os << "\tnr => grd(1)\%nr\n";
    os << "\tif (allocated(dm)) then\n";
    os << "\t\tcall clear_all()\n";
    os << "\tendif\n\n";

    os << "\tallocate(dm(1))\n";
    os << "\tallocate(dmat(1))\n";
    os << "\tallocate(idm(1, 1))\n\n";


    os << "\tallocate(dm(1)\%lvar(nt, " << nvar << "))\n";
    os << "\tallocate(dm(1)\%leq(nt, " << neq << "))\n\n";

    for (auto d: *prog->getDecls()) {
        emitDecl(os, d, lvar_set, leq_set);
    }

    for (auto lv: lvar_set) {
        if (lv.second == false) {
            err() << "lvar for variable `" << lv.first << "\' is not set\n";
            exit(EXIT_FAILURE);
        }
    }
    for (auto le: leq_set) {
        if (le.second == false) {
            err() << "leq for equation `" << le.first << "\' is not set\n";
            exit(EXIT_FAILURE);
        }
    }

    for (int i=1; i<=nvar; i++) {
        os << "\tdo j=2, nt\n";
        os << "\t\tdm(1)%lvar(j, " << i<< ") = 2 + dm(1)\%lvar(j-1, " << i<< ")\n";
        os << "\tenddo\n";
    }
    os << "\n";
    for (int i=1; i<=neq; i++) {
        os << "\tdo j=2, nt\n";
        os << "\t\tdm(1)%leq(j, " << i<< ") = 2 + dm(1)\%leq(j-1, " << i<< ")\n";
        os << "\tenddo\n";
    }
    os << "\n";


    os << "\tdm(1)\%offset = 0\n";
    os << "\tdm(1)\%nas = " << nas << "\n";
    os << "\tdm(1)\%nart = " << nart << "\n";
    os << "\tdm(1)\%nartt = " << nartt << "\n";
    os << "\tidm(1,1)\%natbc = " << natbc << "\n";
    os << "\tidm(1,1)\%nattbc = " << nattbc << "\n";

    os << "\tallocate(dm(1)\%as(dm(1)\%nas))\n";
    os << "\tallocate(dm(1)\%asi(4, dm(1)\%nas))\n";
    os << "\tdm(1)\%as = 0d0\n";
    os << "\tdm(1)\%asi = 0\n\n";

    os << "\tallocate(dm(1)\%art(grd(1)\%nr, nt, dm(1)\%nart))\n";
    os << "\tallocate(dm(1)\%arti(4, dm(1)\%nart))\n";
    os << "\tdm(1)\%art = 0d0\n";
    os << "\tdm(1)\%arti = 0\n\n";

    os << "\tallocate(dm(1)\%artt(grd(1)\%nr, nt, nt, dm(1)\%nartt))\n";
    os << "\tallocate(dm(1)\%artti(4, dm(1)\%nartt))\n";
    os << "\tdm(1)\%art = 0d0\n";
    os << "\tdm(1)\%arti = 0\n\n";

    os << "\tallocate(idm(1, 1)\%atbc(nt, idm(1, 1)\%natbc))\n";
    os << "\tallocate(idm(1, 1)\%atbci(6, idm(1, 1)\%natbc))\n";
    os << "\tidm(1, 1)\%atbc = 0d0\n";
    os << "\tidm(1, 1)\%atbci = 0\n\n";

    os << "\tallocate(idm(1, 1)\%attbc(nt, nt, idm(1, 1)\%nattbc))\n";
    os << "\tallocate(idm(1, 1)\%attbci(6, idm(1, 1)\%nattbc))\n";
    os << "\tidm(1, 1)\%atbc = 0d0\n";
    os << "\tidm(1, 1)\%attbc = 0\n\n";

    os << "\tallocate(dm(1)\%var_name(" << nvar << "))\n";
    os << "\tallocate(dm(1)\%eq_name(" << neq << "))\n\n";

    int ivar = 1;
    for (auto s: *this->prog->getSymTab()) {
        if (dynamic_cast<ir::Variable *>(s)) {
            os << "\tdm(1)\%var_name(" << ivar++ << ") = \'" << s->getName() << "\'\n";
        }
    }

    int ieq = 1;
    for (auto e: *this->prog->getEqs()) {
        os << "\tdm(1)\%eq_name(" << ieq++ << ") = \'" << e->getName() << "\'\n";
    }

    for (auto e: *this->prog->getEqs()) {
        os << "\tcall eq_" << e->getName() << "()\n";
        os << "\tcall bc_" << e->getName() << "()\n";
    }

    os << "\n\tend subroutine init_a\n";
}

#undef unsupported
