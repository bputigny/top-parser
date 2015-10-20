#include "IR.h"
#include "SymTab.h"
#include "Printer.h"

#include <typeinfo>

namespace ir {

Operator::Operator(std::string o) : op(o) { }

BinaryExpr::BinaryExpr(Expr *lOp, BinaryOperator *op, Expr *rOp, Location loc) :
    leftOperand(lOp), rightOperand(rOp), binaryOperator(op) {
        this->loc = loc;
}

bool BinaryExpr::isConst() {
    return this->leftOperand->isConst() && this->rightOperand->isConst();
}

bool BinaryExpr::isVar() {
    return false;
}

std::vector<int> BinaryExpr::getDim() {
    std::vector<int> leftDim = leftOperand->getDim();
    std::vector<int> rightDim = rightOperand->getDim();
    std::vector<int> ret = std::vector<int>();

    if (leftDim.size() != rightDim.size()) {
        if (leftDim[0] == -1)
            return rightDim;
        if (rightDim[0] == -1)
            return leftDim;
        std::cerr << "dimension of operand " << *leftOperand <<
            " and " << *rightOperand << " differs ("<< leftDim.size() <<
            " /= " << rightDim.size() << ")\n";
        exit(1);
    }

    for (int i=0; i<leftDim.size(); i++) {
        if (leftDim[i] == rightDim[i])
            ret.push_back(leftDim[i]);
        else if (leftDim[i] == -1) {
            ret.push_back(rightDim[i]);
        }
        else if (rightDim[i] == -1) {
            ret.push_back(leftDim[i]);
        }
        else {
            std::cerr << "size of dim " << i << " of operand " << *leftOperand <<
                " and " << *rightOperand << " differs\n";
            exit(1);
        }
    }
    return ret;
}

IndexRangeList::IndexRangeList() : std::vector<IndexRange *>() {}

IndexRange::IndexRange(int lb, int ub) {
    lowerBound = new Value<int>(lb);
    upperBound = new Value<int>(ub);
}

IndexRange::IndexRange(Value<int> *lb, Value<int> *ub) :
    lowerBound(lb), upperBound(ub) { }

FunctionCall::FunctionCall(std::string *fName, Location loc) : Identifier(fName, loc) {
    arguments = NULL;
}

FunctionCall::FunctionCall(std::string *fName, ArgumentList *args, Location loc) : Identifier(fName, loc) {
    arguments = args;
}

UnaryExpr::UnaryExpr(Expr *operand, UnaryOperator *op, Location loc) {
    this->operand = operand;
    unaryOperator = op;
    this->loc = loc;
}

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

Identifier::Identifier(std::string *n, Location loc) : name(*n) {
    this->loc = loc;
}
Identifier::Identifier(std::string n, Location loc) : name(n) {
    this->loc = loc;
}

std::string Identifier::getName() {
    return name;
}

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

}

std::ostream &operator<<(std::ostream &os, ir::Symbol &s) {
    return s.dump(os);
}

std::ostream &operator<<(std::ostream &os, ir::Node &n) {
    return n.dump(os);
}
