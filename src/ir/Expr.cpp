#include "IR.h"

namespace ir {

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

std::list<Node *> BinaryExpr::getChildren() {
    std::list<Node *> children;
    children.push_back(leftOperand);
    children.push_back(binaryOperator);
    children.push_back(rightOperand);
    return children;
}

Expr& BinaryExpr::getRightOp() {
    return *this->rightOperand;
}

Expr& BinaryExpr::getLeftOp() {
    return *this->leftOperand;
}

BinaryOperator& BinaryExpr::getOp() {
    return *this->binaryOperator;
}

std::ostream& BinaryExpr::dump(std::ostream& os) {
    os << "(";
    leftOperand->dump(os);
    binaryOperator->dump(os);
    return rightOperand->dump(os) << ")";
}

std::ostream& BinaryExpr::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    binaryOperator->dump(os);
    os << "\"]\n";

    os << (long)this << " -> " << (long)leftOperand << "\n";
    os << (long)this << " -> " << (long)rightOperand << "\n";

    leftOperand->dumpDOT(os);
    rightOperand->dumpDOT(os);

return os;
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


bool UnaryExpr::isConst() {
    return operand->isConst();
}

bool UnaryExpr::isVar() {
    return operand->isVar();
}

Expr& UnaryExpr::getOp() {
    return *operand;
}

std::ostream& UnaryExpr::dump(std::ostream& os) {
    unaryOperator->dump(os);
    return operand->dump(os);
}

std::vector<int> UnaryExpr::getDim() {
    return operand->getDim();
}

std::ostream& UnaryExpr::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    unaryOperator->dump(os);
    os << "\"]\n";

    os << (long)this << " -> " << (long)operand << "\n";

    operand->dumpDOT(os);

    return os;
}

std::list<Node *> UnaryExpr::getChildren() {
    std::list<Node *> children;
    children.push_back(unaryOperator);
    children.push_back(operand);
    return children;
}

UnaryExpr::UnaryExpr(Expr *operand, UnaryOperator *op, Location loc) {
    this->operand = operand;
    unaryOperator = op;
    this->loc = loc;
}

bool Identifier::isConst() {
    return false;
}

bool Identifier::isVar() {
    return true;
}

std::ostream& Identifier::dump(std::ostream& os) {
    return os << name;
}

std::vector<int> Identifier::getDim() {
    std::vector<int> dim = std::vector<int>(1, -1);
    return dim;
}

std::ostream& Identifier::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    os << name;
    os << "\"]\n";

    return os;
}

std::list<Node *> Identifier::getChildren() {
    std::list<Node *> children;
    return children;
}

Identifier::Identifier(std::string *n, Location loc) : name(*n) {
    this->loc = loc;
}
Identifier::Identifier(std::string n, Location loc) : name(n) {
    this->loc = loc;
}

std::string Identifier::getName() {
    return name;
}

FunctionCall::FunctionCall(std::string *fName, Location loc) : Identifier(fName, loc) {
    arguments = NULL;
}

FunctionCall::FunctionCall(std::string *fName, ArgumentList *args, Location loc) : Identifier(fName, loc) {
    arguments = args;
}

ArgumentList& FunctionCall::getArgs() {
    return *arguments;
}

bool FunctionCall::isConst() {
    return false;
}

bool FunctionCall::isVar() {
    return false;
}

std::list<Node *> FunctionCall::getChildren() {
    std::list<Node *> children;
    for (auto c:*arguments)
        children.push_back(c);
    return children;
}


std::ostream& FunctionCall::dump(std::ostream& os) {
    os << name << "(";
    if (arguments != NULL) {
        int n = 0;
        for (auto arg:*arguments) {
            if (n++ > 0)
                os << ", ";
            arg->dump(os);
        }
    }
    return os << ")";
}

std::vector<int> FunctionCall::getDim() {
    return std::vector<int>(1, -1);
}

std::ostream& FunctionCall::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    os << name;
    os << "()\"]\n";

    for (auto arg:*arguments) {
        os << (long)this << " -> " << (long)arg << "\n";
    }

    for (auto arg:*arguments) {
        arg->dumpDOT(os);
    }

    return os << "\n";
}

ArrayExpr::ArrayExpr(std::string *aName, IndexRangeList *irl, Location loc) :
    Identifier(aName, loc) {
    indexRangeList = irl;
}

bool ArrayExpr::isConst() {
    return false;
}

bool ArrayExpr::isVar() {
    return false;
};

std::vector<int> ArrayExpr::getDim() {
    std::vector<int> ret = std::vector<int>();
    for (auto idx:*indexRangeList) {
        ret.push_back(idx->getDim());
    }
    return ret;
}

std::ostream& ArrayExpr::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    os << name << "[]\"]\n";
    for (auto ir:*indexRangeList) {
        os << (long)this << " -> " << (long)ir << "\n";
    }
    for (auto ir:*indexRangeList) {
        ir->dumpDOT(os);
    }
    return os << "\n";
}

std::list<Node *> ArrayExpr::getChildren() {
    std::list<Node *> children;
    for (auto c:*indexRangeList)
        children.push_back(c);
    return children;
}


}

