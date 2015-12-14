#include "IR.h"

namespace ir {

BinaryExpr::BinaryExpr(Expr *lOp, BinaryOperator op, Expr *rOp, Node *p) :
    leftOperand(lOp), rightOperand(rOp), op(op), Expr(p) { }

bool BinaryExpr::isConst() {
    return this->leftOperand->isConst() && this->rightOperand->isConst();
}

bool BinaryExpr::isVar() {
    return false;
}

std::list<Node *> BinaryExpr::getChildren() {
    std::list<Node *> children;
    children.push_back(leftOperand);
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
    return this->op;
}

std::ostream& BinaryExpr::dump(std::ostream& os) {
    os << "(";
    leftOperand->dump(os);
    os << op;
    return rightOperand->dump(os) << ")";
}

std::ostream& BinaryExpr::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    os << op;
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

std::ostream& UnaryExpr::dump(std::ostream& os) {
    os << op;
    return operand->dump(os);
}

std::vector<int> UnaryExpr::getDim() {
    return operand->getDim();
}

std::ostream& UnaryExpr::dumpDOT(std::ostream& os) {
    os << (long)this << " [label=\"";
    os << op;
    os << "\"]\n";

    os << (long)this << " -> " << (long)operand << "\n";

    operand->dumpDOT(os);

    return os;
}

std::list<Node *> UnaryExpr::getChildren() {
    return std::list<Node *>(1, operand);
}

UnaryExpr::UnaryExpr(Expr *operand, UnaryOperator op, Node *p) : Expr(p), op(op) {
    this->operand = operand;
}

Expr& UnaryExpr::getOp() {
    return *this->operand;
}

Identifier::Identifier(std::string *n, Node *p) : name(*n), Expr(p) { }

Identifier::Identifier(std::string n, Node *p) :  Identifier(&n, p) { }

Identifier::~Identifier() { }

bool Identifier::isConst() {
    return false;
}

bool Identifier::isVar() {
    return true;
}

bool Identifier::isLeaf() {
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

std::string Identifier::getName() {
    return name;
}

FunctionCall::FunctionCall(std::string *fName, Node *p) :
    Identifier(fName, p) {
    arguments = NULL;
}

FunctionCall::FunctionCall(std::string *fName, ArgumentList *args, Node *p) :
    Identifier(fName, p) {
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

ArrayExpr::ArrayExpr(std::string *aName, IndexRangeList *irl, Node *p) :
    Identifier(aName, p) {
    indexRangeList = irl;
}

ArrayExpr::ArrayExpr(std::string aName, IndexRangeList *irl, Node *p) :
    Identifier(aName, p) {
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

std::ostream& ArrayExpr::dump(std::ostream& os) {
    int n = 0;
    os << name << "[";
    for (auto irl:*indexRangeList) {
        if (n++ > 0)
            os << ", ";
    }
    os << "]";
    return os;
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

