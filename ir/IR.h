#ifndef IR_H
#define IR_H

#include "config.h"
#include "SymTab.h"
#include "Printer.h"

#include <list>
#include <vector>
#include <string>
#include <typeinfo>
#include <iostream>

namespace ir {

///
/// Base class to represent program's AST
///
class Node : public DOT {
    private:
        static int nNode;

    protected:
        Node *parent;
        std::vector<Node *> children;
        bool clearOnDelete;

    public:
        Node(Node *par = NULL);
        virtual ~Node();
        Node *getParent() const;
        virtual void dump(std::ostream&) const;
        virtual void dumpDOT(std::ostream&,
                std::string title="", bool root = true) const;
        std::vector<Node *>& getChildren();
        std::string srcLoc;
        void setSourceLocation(std::string);
        void clear();
        static int getNodeNumber();
        void setParents();
        void setParent(ir::Node *);

        bool contains(ir::Node&);

        virtual bool operator==(ir::Node&) = 0;
        virtual bool operator!=(ir::Node&);
};

class BinExpr;
class Expr : public Node {
    public:
        Expr(Node *p = NULL);
        Expr(int priority, Node *p = NULL);
        virtual ~Expr();

        const int priority;

        Expr *copy() const;
};

class VectExpr;
class ScalarExpr : public Expr {
    private:
        BinExpr op(const ScalarExpr&, char) const;
        VectExpr op(const VectExpr&, char) const;

    public:
        ScalarExpr(Node *p = NULL);
        ScalarExpr(int priority, Node *p = NULL);

        BinExpr operator+(const ScalarExpr&) const;
        BinExpr operator-(const ScalarExpr&) const;
        BinExpr operator*(const ScalarExpr&) const;
        BinExpr operator/(const ScalarExpr&) const;
        BinExpr operator^(const ScalarExpr&) const;

        VectExpr operator+(const VectExpr&) const;
        VectExpr operator-(const VectExpr&) const;
        VectExpr operator*(const VectExpr&) const;
        VectExpr operator/(const VectExpr&) const;
        VectExpr operator^(const VectExpr&) const;
};

class VectExpr : public Expr {
    private:
        VectExpr op(const ScalarExpr&, char) const;
        VectExpr op(const VectExpr&, char) const;

    public:
        VectExpr(ScalarExpr *, ScalarExpr *, ScalarExpr *);
        VectExpr(const ScalarExpr&, const ScalarExpr&, const ScalarExpr&);
        VectExpr(const VectExpr&);

        ScalarExpr *getX() const;
        ScalarExpr *getY() const;
        ScalarExpr *getZ() const;
        ScalarExpr *getComponent(int) const;
        virtual void dump(std::ostream& os) const;
        bool operator==(Node&);

        VectExpr operator+(const VectExpr&) const;
        VectExpr operator-(const VectExpr&) const;
        VectExpr operator*(const VectExpr&) const;
        VectExpr operator/(const VectExpr&) const;
        VectExpr operator^(const VectExpr&) const;

        VectExpr operator+(const ScalarExpr&) const;
        VectExpr operator-(const ScalarExpr&) const;
        VectExpr operator*(const ScalarExpr&) const;
        VectExpr operator/(const ScalarExpr&) const;
        VectExpr operator^(const ScalarExpr&) const;
};

template <class T>
class Value : public ScalarExpr {
    T value;
    public:
        inline Value(T val, Node *p = NULL) {
            value = val;
        }
        inline Value(const Value<T>& v) : Value<T>(v.getValue()) { }
        inline T getValue() const {
            return value;
        }
        inline void dump(std::ostream &os) const {
            os << "Val: " << value;
        }
        inline bool operator==(Node& node) {
            try {
                Value& v = dynamic_cast<Value<T>&>(node);
                return value == v.value;
            }
            catch (std::bad_cast) {
                return false;
            }
        }
        inline Value<T> operator=(T v) {
            return Value(v);
        }
};

class BinExpr : public ScalarExpr {
    protected:
        char op;

    public:
        BinExpr(ScalarExpr *lOp, char op, ScalarExpr *rOp, Node *parent = NULL);
        BinExpr(const ScalarExpr& lOp, char op, const ScalarExpr& rOp, Node *parent = NULL);
        BinExpr(const BinExpr&);

        ScalarExpr *getRightOp() const;
        ScalarExpr *getLeftOp() const;
        char getOp() const;
        virtual void dump(std::ostream&) const;
        bool operator==(Node&);
};

class UnaryExpr : public ScalarExpr {
    protected:
        char op;

    public:
        UnaryExpr(ScalarExpr *lOp, char op, Node *parent = NULL);
        UnaryExpr(const ScalarExpr& lOp, char op, Node *parent = NULL);
        UnaryExpr(const UnaryExpr&);

        ScalarExpr *getExpr() const;
        char getOp() const;
        virtual void dump(std::ostream&) const;
        bool operator==(Node&);
};

class Symbol;
class DiffExpr : public ScalarExpr {
    protected:
        std::string order;

    public:
        DiffExpr(Expr *, Identifier *, std::string order = std::string("1"),
                Node *p = NULL);
        DiffExpr(const Expr&, const Identifier&, std::string order = std::string("1"),
                Node *p = NULL);
        DiffExpr(const DiffExpr&);
        Expr *getExpr() const;
        Identifier *getVar() const;
        void setOrder(std::string);
        std::string getOrder() const;
        virtual void dump(std::ostream&) const;
        bool operator==(Node&);
};

class ExprLst : public std::vector<Expr *> {
};

class Identifier : public ScalarExpr {
    protected:

    public:
        Identifier(std::string n, int vectComponent = 0, Node *parent = NULL);
        Identifier(const Identifier&);

        const std::string name;
        virtual void dump(std::ostream& os) const;
        bool operator==(Node&);
        const int vectComponent;
};

class FuncCall : public Identifier {
    public:
        FuncCall(std::string name, ExprLst *args = NULL, Node *p = NULL);
        FuncCall(std::string name, Expr *arg, Node *p = NULL);
        FuncCall(std::string name, const Expr& arg, Node *p = NULL);
        FuncCall(const FuncCall&);

        virtual void dump(std::ostream& os) const;
        ExprLst getArgs() const;
        bool operator==(Node&);
};

class ArrayExpr : public Identifier {
    public:
        ArrayExpr(std::string name, ExprLst *indices, Node *p = NULL);
        ArrayExpr(std::string name, ScalarExpr *index, Node *p = NULL);
        ArrayExpr(std::string name, ScalarExpr& index, Node *p = NULL);
        ArrayExpr(const ArrayExpr&);

        ExprLst getIndices() const;
        bool operator==(Node&);
};

class Decl : public Node {
    /// this flag tells whether the declaration have a degree l = 0
    /// this obviously only makes sense for lvar(variable) declaration

    public:
        Decl(Expr *, Expr *);

        Expr *getLHS() const;
        Expr *getDef() const;
        virtual void dump(std::ostream& os) const;
        bool operator==(Node&);
};

class DeclLst : public std::list<Decl *> { };

class BCLst;
class Equation : public Node {
    public:
        Equation(std::string name, Expr *lhs, Expr *rhs, BCLst *bc, Node *p = NULL);
        Equation(std::string name, const Expr& lhs, const Expr& rhs,
                BCLst *bc, Node *p = NULL);
        Equation(std::string name, Equation &eq);

        const std::string name;
        virtual void dump(std::ostream& os) const;
        Expr *getLHS() const;
        Expr *getRHS() const;
        BCLst *getBCs() const;
        bool operator==(Node&);
        void setBCs(ir::BCLst *);
};

class EqLst : public std::list<Equation *> { };

class BC : public Node {
    protected:
        int eqLoc;  // this encodes the place to set the BC in the linear system
                    // (i.e, the line in the matrix)

    public:
        BC(Equation *cond, Equation *loc, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        bool operator==(Node&);
        Equation *getLoc() const;
        Equation *getCond() const;
        void setEqLoc(int);
        int getEqLoc() const;
};

class BCLst : public std::list<BC *> {
};

class Program : public DOT {
    private:
        SymTab *symTab;
        DeclLst *decls;
        EqLst *eqs;

    public:
        Program(std::string, SymTab *, DeclLst *decls, EqLst *eqs);
        ~Program();
        void buildSymTab();
        virtual void dumpDOT(std::ostream& os, std::string title, bool root = true) const;
        SymTab& getSymTab();
        EqLst& getEqs();
        DeclLst& getDecls();

        void replace(Node *, Node *);

        const std::string filename;
};

class IndexRange : public BinExpr {
    public:
        IndexRange(ScalarExpr *lb, ScalarExpr *ub, Node *p = NULL);
        IndexRange(const IndexRange&);
        ScalarExpr *getLB() const;
        ScalarExpr *getUB() const;
};

class IndexRangeLst : public std::list<IndexRange *> {
};

class Symbol {
    protected:
        Expr *expr;
        bool internal;
    public:
        Symbol(std::string name, Expr *def = NULL, bool internal = false);
        ~Symbol();
        const std::string name;
        virtual Expr *getDef();
        bool isInternal();
};

class Param : public Symbol {
    protected:
        std::string type;
    public:
        Param(std::string n, std::string t, Expr *def = NULL);
        std::string getType();
};

class Variable : public Symbol {
    public:
        Variable(std::string n, int vc = 0, Expr *def = NULL, bool internal = false);
        const int vectComponent;
};

class Array : public Variable {
    protected:
        int ndim;
    public:
        Array(std::string n, int ndim, Expr *def = NULL, bool internal = false);
};

class Function : public Symbol {
    protected:
        int nparams;
    public:
        Function(std::string n, int nparams);
};

class Field : public Symbol {
    public:
        Field(std::string n);
};

class Scalar : public Symbol {
    public:
        Scalar(std::string n);
};

VectExpr crossProduct(const Expr&, const Expr&);
BinExpr dotProduct(const Expr&, const Expr&);

UnaryExpr operator-(ScalarExpr&);
VectExpr operator-(VectExpr&);

BinExpr operator/(float, const ScalarExpr&);
VectExpr operator/(float, const VectExpr&);
BinExpr operator/(int, const ScalarExpr&);
VectExpr operator/(int, const VectExpr&);
BinExpr operator^(const ScalarExpr&, int);
VectExpr operator^(const VectExpr&, int);

Expr& operator+(const Expr&, const Expr&);
Expr& operator-(const Expr&, const Expr&);
Expr& operator*(const Expr&, const Expr&);
Expr& operator/(const Expr&, const Expr&);
Expr& operator^(const Expr&, const Expr&);

ScalarExpr *scalar(Expr *);
VectExpr *vector(Expr *);
ScalarExpr& scalar(Expr&);
VectExpr& vector(Expr&);

bool isScalar(Expr *);
bool isVect(Expr *);

FuncCall sin(const ScalarExpr&);
FuncCall cos(const ScalarExpr&);

} // end namespace ir

std::ostream& operator<<(std::ostream&, ir::Node&);

#endif
