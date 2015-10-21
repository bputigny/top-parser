#ifndef IR_H
#define IR_H

#include "config.h"
#include "Printer.h"

#include <list>
#include <vector>
#include <string>
#include <typeinfo>
#include <iostream>

class SymTab;

///
/// Interface for classes to print their representation as dot graph
///
class DOT {
    public :
        virtual std::ostream& dumpDOT(std::ostream&) = 0;
};

namespace ir {

class Location : public std::string {
    public:
        Location() { }
        Location(std::string loc) : std::string(loc) { }
};

class Node : public DOT {
    protected:
        Location loc;
    public:
        std::string getLocation() {
            return loc;
        }
        virtual std::ostream& dump(std::ostream&) = 0;
        virtual std::list<Node *> getChildren() = 0;
        std::list<Node *> getLeafs() {
            std::list<Node *> children = getChildren();
            std::list<Node *> leafs = std::list<Node *>();
            if (children.empty()) {
                return std::list<Node *>(1, this);
            }
            for (auto c:children) {
                std::list<Node *> childLeafs = c->getLeafs();
                for (auto l:childLeafs) {
                    leafs.push_back(l);
                }
            }
            return leafs;
        }
        std::list<Node *> getLeafs(std::type_info& t) {
            std::list<Node *> leafs = getLeafs();
            std::list<Node *> selectedLeafs = std::list<Node *>();
            for (auto l:leafs)
                if (typeid(*l) == t)
                    selectedLeafs.push_back(l);
            return selectedLeafs;
        }
};

class Expr : public Node {
    public:
        virtual bool isConst() = 0;
        virtual bool isVar() = 0;
        virtual std::vector<int> getDim() = 0;
        virtual std::list<Node *> getChildren() = 0;
};

class Operator : public Node {
    protected:
        std::string op;
    public:
        Operator(std::string);
        std::string& getOp() { return op; };
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os) {
            return os << op;
        }
        std::ostream& dumpDOT(std::ostream& os) {
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
        Expr& getRightOp();
        Expr& getLeftOp();
        BinaryOperator& getOp();
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os);
        std::vector<int> getDim();
        std::ostream& dumpDOT(std::ostream& os);
};

class UnaryExpr : public Expr {
    protected:
        Expr *operand;
        UnaryOperator *unaryOperator;
    public:
        UnaryExpr(Expr *operand, UnaryOperator *op, Location loc = Location("undef"));
        bool isConst();
        bool isVar();
        Expr& getOp();
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os);
        std::vector<int> getDim();
        std::ostream& dumpDOT(std::ostream& os);
};

class Identifier : public Expr {
    protected:
        std::string name;
    public:
        Identifier(std::string *n, Location loc = Location("undef"));
        Identifier(std::string n, Location loc = Location("undef"));
        ~Identifier();
        std::string getName();
        bool isConst();
        bool isVar();
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os);
        std::vector<int> getDim();
        std::ostream& dumpDOT(std::ostream& os);
};

template <class T>
class Value : public Expr {
    T value;
    public:
    inline Value(T val) {
        value = val;
    }
    inline bool isConst(){
        return true;
    }
    inline bool isVar(){
        return false;
    }
    inline std::list<Node *> getChildren() {
        std::list<Node *> children;
        return children;
    }
    inline std::ostream& dump(std::ostream& os){
        return os << std::to_string(value);
    }
    inline T getVal(){
        return value;
    }
    inline std::vector<int> getDim(){
        return std::vector<int>(1, -1);
    }
    inline std::ostream& dumpDOT(std::ostream& os){
        os << (long)this << " [label=\"";
        os << value;
        os << "\"]\n";

        return os;
    }
    inline bool operator==(Value<T>& v){
        return this->value == v.value;
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
        ArgumentList& getArgs();
        bool isConst();
        bool isVar();;
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os);
        std::vector<int> getDim();
        std::ostream& dumpDOT(std::ostream& os);
};

class IndexRange : public Node {
    protected:
        Value<int> *lowerBound;
        Value<int> *upperBound;
    public:
        IndexRange(Value<int> *lb, Value<int> *ub);
        IndexRange(int lb, int ub);
        Value<int>& getLowerBound() { return *lowerBound; }
        Value<int>& getUpperBound() { return *lowerBound; }
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os) {
            lowerBound->dump(os);
            os << ":";
            return upperBound->dump(os);
        }
        int getDim() {
            int dim = lowerBound->getVal()-upperBound->getVal()+1;
            return dim;
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << (long)this << " [label=\":\"]\n";
            if (*lowerBound == *upperBound) {
                os << (long)this << " -> " << (long)lowerBound << "\n";
                lowerBound->dumpDOT(os);
            }
            else {
                os << (long)this << " -> " << (long)lowerBound << "\n";
                os << (long)this << " -> " << (long)upperBound << "\n";
                lowerBound->dumpDOT(os);
                upperBound->dumpDOT(os);
            }
            return os << "\n";
        }
};

class IndexRangeList : public std::vector<IndexRange *> {
    public:
        IndexRangeList();
        std::ostream& dump(std::ostream& os) {
            return os << "IndexRangeList";
        }
};

class ArrayExpr : public Identifier {
    protected:
        IndexRangeList *indexRangeList;
    public:
        ArrayExpr(std::string *aName, IndexRangeList *irl,
                Location loc = Location("undef"));
        bool isConst();
        bool isVar();;
        std::list<Node *> getChildren();
        std::vector<int> getDim();
        std::ostream& dumpDOT(std::ostream& os);
};

class Equation : public Node {
    protected:
        Expr *leftHandSide;
        Expr *rightHandSide;
    public:
        Equation(Expr *lhs, Expr *rhs);
        Expr& getLHS() {
            return *leftHandSide;
        }
        Expr& getRHS() {
            return *rightHandSide;
        }
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os) {
            leftHandSide->dump(os);
            os << " = ";
            return rightHandSide->dump(os);
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << (long)this << " [label=\" = \"]\n";
            os << (long)this << " -> " << (long)leftHandSide << "\n";
            os << (long)this << " -> " << (long)rightHandSide << "\n";
            leftHandSide->dumpDOT(os);
            rightHandSide->dumpDOT(os);
            return os << "\n";
        }
};

class BoundaryCondition : public Node {
    protected:
        Equation *boundaryCondition;
        Equation *location;
    public:
        BoundaryCondition(Equation *cond, Equation *loc);
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os) {
            os << "BC:\n";
            os << "    ";
            this->boundaryCondition->dump(os);
            os  << " at ";
            this->location->dump(os);
            return os << "\n";
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << (long)this << " [label=\" BC \"]\n";
            os << (long)this << " -> " << (long)boundaryCondition << " [label=\"expr\"]\n";
            os << (long)this << " -> " << (long)location << " [label=\"at\"]\n";
            boundaryCondition->dumpDOT(os);
            location->dumpDOT(os);
            return os;
        }
};

class Declaration : public Node {
    protected:
        Node *leftHandSide;
        Expr *expr;
    public:
        Declaration(Node *, Expr *, Location loc = std::string("undef"));
        Node& getLHS();
        Expr& getExpr();
        std::list<Node *> getChildren();
        std::ostream& dump(std::ostream& os) {
            leftHandSide->dump(os);
            os << " := ";
            return expr->dump(os);
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << (long)this << " [label=\" := \"]\n";
            os << (long)this << " -> " << (long)leftHandSide << "\n";
            os << (long)this << " -> " << (long)expr << "\n";
            leftHandSide->dumpDOT(os);
            expr->dumpDOT(os);
            return os << "\n";
        }
};

class DeclarationList : public std::vector<Declaration *> {
    public:
        DeclarationList();
        std::ostream& dump(std::ostream& os) {
            return os << "DeclList";
        }
};

class EquationList : public std::vector<Equation *> {
    public:
        EquationList();
        std::ostream& dump(std::ostream& os) {
            return os << "EquationList";
        }
};

class BoundaryConditionList : public std::vector<BoundaryCondition *> {
    public:
        BoundaryConditionList();
        std::ostream& dump(std::ostream& os) {
            return os << "BCList";
        }
};

class Program : public Node {

    protected:
        DeclarationList *decls;
        EquationList *eqs;
        BoundaryConditionList *bcs;
        SymTab *symTab;
    public:
        Program(SymTab *params, DeclarationList *decls,
                EquationList *eqs, BoundaryConditionList *bcs);
        DeclarationList& getDeclarations() {
            return *decls;
        }
        SymTab& getSymTab() {
            return *symTab;
        }
        EquationList& getEquations() {
            return *eqs;
        }
        BoundaryConditionList& getBCs() {
            return *bcs;
        }
        std::list<Node *> getChildren();
        void buildSymTab();
        void computeVarSize();
        std::ostream& dump(std::ostream& os) {
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
        std::ostream& dumpDOT(std::ostream& os) {
            os << "digraph ir {\n";
            os << "node [shape = Mrecord]\n";
            os << (long)this << " [label=\"root\"]\n";

            for (auto decl:*decls) {
                os << (long)this << " -> " << (long)decl << "\n";
            }
            for (auto decl:*decls) {
                decl->dumpDOT(os);
            }

            for (auto eq:*eqs) {
                os << (long)this << " -> " << (long)eq << "\n";
            }
            for (auto eq:*eqs) {
                eq->dumpDOT(os);
            }


            for (auto bc:*bcs) {
                os << (long)this << " -> " << (long)bc << "\n";
            }
            for (auto bc:*bcs) {
                bc->dumpDOT(os);
            }

            return os << "}\n";
        }
};

class Symbol : public DOT {
    protected:
        std::string name;
        Expr *def;
        bool internal;
        bool defined;
        std::vector<int> dim;
    public:
        Symbol(std::string n, Expr *def = NULL, bool internal = false) : name(n) {
            this->def = def;
            this->internal = internal;
            this->dim = std::vector<int>(1, -1);
        }
        std::string getName() {
            return name;
        }
        Expr *getDef() { return def; }
        bool isInternal() { return internal; }
        bool isDefined() { return defined; }
        bool operator==(Symbol& s);
        bool operator>(Symbol& s);
        bool operator<(Symbol& s);
        virtual std::ostream& dump(std::ostream&) = 0;
        void setDim(std::vector<int> dim) {
            this->dim = dim;
        }
        std::vector<int> getDim() {
            return this->dim;
        }
        std::ostream& dumpDOT(std::ostream& os) {
            if (internal)
                os << " </td></tr><tr><td border=\"1\"> internal ";
            else
                os << " </td></tr><tr><td border=\"1\"> user ";
            if (defined)
                os << " </td></tr><tr><td border=\"1\"> defined ";
            else
                os << " </td></tr><tr><td border=\"1\"> undef ";
            os << " </td></tr><tr><td border=\"1\"";
            if (getDef())
                os << " port=\"" << (long)this << "\">";
            else
                os << ">";
            os << "<b>" << name << "</b>";
            return os << "</td></tr>\n";
        }
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
        std::ostream& dump(std::ostream& os) {
            return os << name;
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << "<tr><td border=\"1\"> param " << type;
            return Symbol::dumpDOT(os);
        }
};

class Variable : public Symbol {
    public:
        Variable(std::string n, Expr *def = NULL, bool internal = false) :
            Symbol(n, def, internal) { }
        std::ostream& dump(std::ostream& os) {
            return os << name;
        }
        std::ostream& dumpDOT(std::ostream& os) {
            int n = 0;
            os << "<tr><td border=\"1\"> var size: ";
            std::vector<int> dim = getDim();
            if (dim == std::vector<int>(1, -1)) {
                os << " full ";
            }
            else {
                for (auto s:getDim()) {
                    if (n++ > 0)
                        os << "x";
                    os << s;
                }
            }
            return Symbol::dumpDOT(os);
        }
};

class Array : public Variable {
    public:
        Array(std::string n, Expr *def = NULL, bool internal = false) :
            Variable(n, def, internal) { }
        std::ostream& dumpDOT(std::ostream& os) {
            os << "<tr><td border=\"1\"> array ";
            return Symbol::dumpDOT(os);
        }
};

class Function : public Symbol {
    public:
        Function(std::string n, bool internal = false) :
            Symbol(n, NULL, internal) { }
        std::ostream& dump(std::ostream& os) {
            return os << name << "()";
        }
        std::ostream& dumpDOT(std::ostream& os) {
            os << "<tr><td border=\"1\"> func ";
            return Symbol::dumpDOT(os);
        }
};

}

std::ostream& operator<<(std::ostream& , ir::Symbol&);
std::ostream& operator<<(std::ostream& , ir::Identifier&);
std::ostream& operator<<(std::ostream& , ir::Node&);

#endif
