#include "IR.h"
#include "SymTab.h"
#include "Printer.h"

#include <functional>
#include <typeinfo>
#include <cassert>

namespace ir {

Operator::Operator(std::string o) : op(o) { }

std::list<Node *> Program::getChildren() {
    std::list<Node *> children;
    for (auto c:*decls)
        children.push_back(c);
    for (auto c:*eqs)
        children.push_back(c);
    for (auto c:*bcs)
        children.push_back(c);
    return children;
}

std::list<Node *> Operator::getChildren() {
    std::list<Node *> children;
    return children;
}

std::list<Node *> IndexRange::getChildren() {
    std::list<Node *> children;
    children.push_back(lowerBound);
    children.push_back(upperBound);
    return children;
}

std::list<Node *> Equation::getChildren() {
    std::list<Node *> children;
    children.push_back(leftHandSide);
    children.push_back(rightHandSide);
    return children;
}

std::list<Node *> Declaration::getChildren() {
    std::list<Node *> children;
    children.push_back(leftHandSide);
    children.push_back(expr);
    return children;
}

std::list<Node *> BoundaryCondition::getChildren() {
    std::list<Node *> children;
    children.push_back(boundaryCondition);
    children.push_back(location);
    return children;
}

IndexRangeList::IndexRangeList() : std::vector<IndexRange *>() {}

IndexRange::IndexRange(int lb, int ub) {
    lowerBound = new Value<int>(lb);
    upperBound = new Value<int>(ub);
}

IndexRange::IndexRange(Value<int> *lb, Value<int> *ub) :
    lowerBound(lb), upperBound(ub) { }

Equation::Equation(Expr *lhs, Expr *rhs) : leftHandSide(lhs), rightHandSide(rhs) { }

BoundaryCondition::BoundaryCondition(Equation *cond, Equation *loc) :
    boundaryCondition(cond), location(loc) {}

Declaration::Declaration(Node *lhs, Expr *rhs, Location loc) :
    leftHandSide(lhs), expr(rhs) {
        this->loc = loc;
    }

Node &Declaration::getLHS() {
    return *this->leftHandSide;
}

Expr &Declaration::getExpr() {
    return *this->expr;
}

DeclarationList::DeclarationList() : std::vector<Declaration *>() { }

EquationList::EquationList() : std::vector<Equation *>() { }

BoundaryConditionList::BoundaryConditionList() :
    std::vector<BoundaryCondition *>() { }

Program::Program(SymTab *params, DeclarationList *decls,
        EquationList *eqs, BoundaryConditionList *bcs) {
    this->decls = decls;
    this->eqs = eqs;
    this->bcs = bcs;
    this->symTab = params;
}

bool Symbol::operator==(Symbol &s) {
    return this->name == s.getName();
}

bool Symbol::operator<(Symbol &s) {
    return this->name < s.getName();
}
bool Symbol::operator>(Symbol &s) {
    return this->name < s.getName();
}

void Program::buildSymTab() {
    // add definition's LHS
    for (auto d:this->getDeclarations()) {
        assert(d);
        if (ir::Param *p = dynamic_cast<ir::Param *>(&d->getLHS())) {
            // Don't add parameter to symTab for they are already in
        }
        else if (ir::ArrayExpr *ae = dynamic_cast<ir::ArrayExpr *>(&d->getLHS())) {
            symTab->add(new ir::Array(ae->getName(), &d->getExpr()));
        }
        else if (ir::Identifier *s = dynamic_cast<ir::Identifier *>(&d->getLHS())) {
            symTab->add(new ir::Variable(s->getName(), &d->getExpr()));
        }
        else {
            std::cerr << *d;
            semantic_error("cannot be the LHS of a definition\n");
        }
    }

    std::function<void(ir::Expr &)> parseExpr;
    parseExpr = [&parseExpr, this] (ir::Expr &e) {
        if (ir::FunctionCall *fc = dynamic_cast<ir::FunctionCall *>(&e)) {
            if (symTab->search(fc->getName()) == NULL) {
                error("%s: undefined function '%s'\n",
                        fc->getLocation().c_str(),
                        fc->getName().c_str());
            }
        }
        else if (ir::ArrayExpr *ae = dynamic_cast<ir::ArrayExpr *>(&e)) {
            if (symTab->search(ae->getName()) == NULL) {
                error("%s: undefined array '%s'\n",
                        ae->getLocation().c_str(),
                        ae->getName().c_str());
            }
        }
        else if (ir::Identifier *s = dynamic_cast<ir::Identifier *>(&e)) {
            if (symTab->search(s->getName()) == NULL) {
                error("%s: undefined symbol '%s'\n",
                        s->getLocation().c_str(),
                        s->getName().c_str());
            }
        }
        else if (ir::BinaryExpr *be = dynamic_cast<ir::BinaryExpr *>(&e)) {
            parseExpr(be->getLeftOp());
            parseExpr(be->getRightOp());
        }
        else if (ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(&e)) {
            parseExpr(ue->getOp());
        }
        else if (dynamic_cast<ir::Value<int> *>(&e)) {
        }
        else if (dynamic_cast<ir::Value<float> *>(&e)) {
        }
        else {
            std::cerr << e;
            info("was not parse\n");
        }
    };

    // parse definition's RHS for undef id
    for (auto d:this->getDeclarations()) {
        ir::Expr &e = d->getExpr();
        parseExpr(e);
    }
}

void Program::computeVarSize() {
    for (auto s:getSymTab()) {
        if (s->getDef()) {
            std::vector<int> dim = s->getDef()->getDim();
            s->setDim(dim);
        }
    }
}

}

std::ostream &operator<<(std::ostream &os, ir::Symbol &s) {
    return s.dump(os);
}

std::ostream &operator<<(std::ostream &os, ir::Node &n) {
    return n.dump(os);
}
