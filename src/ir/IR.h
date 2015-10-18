#ifndef IR_H
#define IR_H

#include <vector>
#include <string>
#include <iostream>

class SymTab;

namespace ir {

class Location : public std::string {
    public:
        Location() { }
        Location(std::string loc) : std::string(loc) { }
};

class Node {
    protected:
        Location loc;
    public:
        std::string getLocation() {
            return loc;
        }
        virtual std::ostream &dump(std::ostream &) = 0;
};

class Expr : public Node {
    public:
        virtual bool isConst() = 0;
        virtual bool isVar() = 0;
};

class Operator : public Node {
    protected:
        std::string op;
    public:
        Operator(std::string);
        std::string &getOp() { return op; };
        std::ostream &dump(std::ostream &os) {
            return os << op;
        }
};

class UnaryOperator : public Operator {
    public:
        UnaryOperator(std::string s) : Operator(s) { }
};

class BinaryOperator : public Operator {
    public:
        BinaryOperator(std::string s) : Operator(s) { }
};

class BinaryExpr : public Expr {
    protected:
        Expr *leftOperand, *rightOperand;
        BinaryOperator *binaryOperator;

    public:
        BinaryExpr(Expr *lOp, BinaryOperator *op, Expr *rOp, Location loc=Location("undef"));
        bool isConst();
        bool isVar();
        Expr &getRightOp() { return *this->rightOperand; }
        Expr &getLeftOp() { return *this->leftOperand; }
        BinaryOperator &getOp() { return *this->binaryOperator; }
        std::ostream &dump(std::ostream &os) {
            os << "(";
            leftOperand->dump(os);
            binaryOperator->dump(os);
            return rightOperand->dump(os) << ")";
        }
};

class UnaryExpr : public Expr {
    protected:
        Expr *operand;
        UnaryOperator *unaryOperator;
    public:
        UnaryExpr(Expr *operand, UnaryOperator *op, Location loc = Location("undef"));
        bool isConst() { return operand->isConst(); }
        bool isVar() { return operand->isVar(); }
        Expr &getOp() { return *operand; }
        std::ostream &dump(std::ostream &os) {
            unaryOperator->dump(os);
            return operand->dump(os);
        }
};

class Identifier : public Expr {
    protected:
        std::string name;
    public:
        Identifier(std::string *n, Location loc = Location("unde"));
        Identifier(std::string n, Location loc = Location("unde"));
        ~Identifier();
        std::string getName();
        bool isConst() { return false; }
        bool isVar() { return true; }
        std::ostream &dump(std::ostream &os) {
            return os << name;
        }
};

class Value : public Expr {
    public:
        bool isConst() { return true; }
        bool isVar() { return false; }
        virtual std::ostream &dump(std::ostream &o) = 0;
};

class RealValue : public Value {
    double value;
    public:
        RealValue(double);
         std::ostream &dump(std::ostream &os) {
            return os << std::to_string(value);
        }
};

class IntValue : public Value {
    int value;
    public:
        IntValue(int);
        std::ostream &dump(std::ostream &os) {
            return os << std::to_string(value);
        }
};

class ArgumentList : public std::vector<Expr *> {
    public:
        ArgumentList() : std::vector<Expr *>() { }
        bool isConst() { return false; }
        bool isVar() { return false; }
};

class FunctionCall : public Identifier {
    protected:
        ArgumentList *arguments;
    public:
        FunctionCall(std::string *, Location loc = Location("undef"));
        FunctionCall(std::string *, ArgumentList *, Location loc = Location("undef"));
        ArgumentList &getArgs() { return *arguments; }
        bool isConst() { return false; }
        bool isVar() { return false; };
        std::ostream &dump(std::ostream &os) {
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
};

class IndexRange : public Node {
    protected:
        Node *lowerBound;
        Node *upperBound;
    public:
        IndexRange(Node *lb, Node *ub);
        Node &getLowerBound() { return *lowerBound; }
        Node &getUpperBound() { return *lowerBound; }
        std::ostream &dump(std::ostream &os) {
            lowerBound->dump(os);
            os << ":";
            return upperBound->dump(os);
        }
};

class IndexRangeList : public std::vector<IndexRange *>, public Node {
    public:
        IndexRangeList();
        std::ostream &dump(std::ostream &os) {
            return os << "IndexRangeList";
        }
};

class ArrayExpr : public Identifier {
    protected:
        IndexRangeList *indexRangeList;
    public:
        ArrayExpr(std::string *aName, IndexRangeList *irl, Location loc = Location("undef"))
            : Identifier(aName, loc) {
            indexRangeList = irl;
        }
        bool isConst() { return false; }
        bool isVar() { return false; };
};

class Equation : public Node {
    protected:
        Expr *leftHandSide;
        Expr *rightHandSide;
    public:
        Equation(Expr *lhs, Expr *rhs);
        Expr &getLHS() {
            return *leftHandSide;
        }
        Expr &getRHS() {
            return *rightHandSide;
        }
        std::ostream &dump(std::ostream &os) {
            leftHandSide->dump(os);
            os << " = ";
            return rightHandSide->dump(os);
        }
};

class BoundaryCondition : public Node {
    protected:
        Equation *boundaryCondition;
        Equation *location;
    public:
        BoundaryCondition(Equation *cond, Equation *loc);
        std::ostream &dump(std::ostream &os) {
            return os << "BC";
        }
};

class Declaration : public Node {
    protected:
        Node *leftHandSide;
        Expr *expr;
    public:
        Declaration(Node *, Expr *, Location loc = std::string("undef"));
        Node &getLHS();
        Expr &getExpr();
        std::ostream &dump(std::ostream &os) {
            leftHandSide->dump(os);
            os << " := ";
            return expr->dump(os);
        }
};

class DeclarationList : public std::vector<Declaration *>, public Node {
    public:
        DeclarationList();
        std::ostream &dump(std::ostream &os) {
            return os << "DeclList";
        }
};

class EquationList : public std::vector<Equation *>, public Node {
    public:
        EquationList();
        std::ostream &dump(std::ostream &os) {
            return os << "EquationList";
        }
};

class BoundaryConditionList : public std::vector<BoundaryCondition *>, public Node {
    public:
        BoundaryConditionList();
        std::ostream &dump(std::ostream &os) {
            return os << "BCList";
        }
};

class Program : public Node{

    protected:
        DeclarationList *decls;
        EquationList *eqs;
        BoundaryConditionList *bcs;
        SymTab *symTab;
    public:
        Program(SymTab *params, DeclarationList *decls,
                EquationList *eqs, BoundaryConditionList *bcs);
        DeclarationList &getDeclarations() {
            return *decls;
        }
        SymTab &getSymTab() {
            return *symTab;
        }
        EquationList &getEquations() {
            return *eqs;
        }
        BoundaryConditionList &getBCs() {
            return *bcs;
        }
        std::ostream &dump(std::ostream &os) {
            os << "Program:\n";
            os << " - Decls:\n";
            for (auto d:getDeclarations()) {
                os << "    ";
                d->dump(os);
                os << "\n";
            }
            os << " - Equations:\n";
            for (auto e:getEquations()) {
                os << "    ";
                e->dump(os);
                os << "\n";
            }
            os << " - BC:\n";
            for (auto bc:getBCs()) {
                os << "    ";
                bc->dump(os);
                os << "\n";
            }
            return os;
        }
};

class Symbol {
    protected:
        std::string name;
        Expr *def;
        bool internal;
        bool defined;
    public:
        Symbol(std::string n, Expr *def = NULL, bool internal = false) : name(n) {
            this->def = def;
            this->internal = internal;
        }
        std::string getName() {
            return name;
        }
        Expr *getDef() { return def; }
        bool isInternal() { return internal; }
        bool isDefined() { return defined; }
        bool operator==(Symbol &s);
        bool operator>(Symbol &s);
        bool operator<(Symbol &s);
        virtual std::ostream &dump(std::ostream &) = 0;
};

class Param : public Symbol {
    protected:
        std::string type;
    public:
        Param(std::string n, std::string t, Expr *def = NULL) : Symbol(n, def) {
            type = t;
        }
        std::string getType() {
            return type;
        }
        std::ostream &dump(std::ostream &os) {
            return os << name;
        }
};

class Variable : public Symbol {
    public:
        Variable(std::string n, Expr *def = NULL, bool internal = false) :
            Symbol(n, def, internal) { }
        std::ostream &dump(std::ostream &os) {
            return os << name;
        }
};

class Array : public Variable {
    public:
        Array(std::string n, Expr *def = NULL, bool internal = false) :
            Variable(n, def, internal) { }
};

class Function : public Symbol {
    public:
        Function(std::string n) : Symbol(n, NULL, true) { }
        std::ostream &dump(std::ostream &os) {
            return os << name << "()";
        }
};

}

std::ostream &operator<<(std::ostream &, ir::Symbol &);
std::ostream &operator<<(std::ostream &, ir::Identifier &);
std::ostream &operator<<(std::ostream &, ir::Node &);

#endif
