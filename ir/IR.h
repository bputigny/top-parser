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
        std::string srcLoc;
        Node *parent;
        std::vector<Node *> children;
    public:
        Node(Node *par = NULL);
        virtual ~Node();
        Node *getParent();
        virtual void dump(std::ostream&) const;
        virtual void dumpDOT(std::ostream&, std::string title="", bool root = true) const;
        std::vector<Node *>& getChildren();
        std::vector<Node *> getLeafs();
        bool isLeaf();
        std::string getSourceLocation();
        void setSourceLocation(std::string);
        void clear();
        static int getNodeNumber();
        virtual Node *copy() = 0;
        void setParents();
        void setParent(ir::Node *);

        bool contains(ir::Node&);

        virtual bool operator==(ir::Node&) = 0;
        virtual bool operator!=(ir::Node&);
};

class Expr : public Node {
    public:
        Expr(Node *p = NULL);
        Expr(int priority, Node *p = NULL);
        virtual ~Expr();

        const int priority;
};

template <class T>
class Value : public Expr {
    T value;
    public:
        inline Value(T val, Node *p = NULL) {
            value = val;
        }
        inline T getValue() {
            return value;
        }
        inline void dump(std::ostream &os) const {
            os << "Val: " << value;
        }
        virtual inline Node *copy() {
            return new Value<T>(value);
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
};

class BinExpr : public Expr {
    protected:
        char op;

    public:
        BinExpr(Expr *lOp, char op, Expr *rOp, Node *parent = NULL);
        Expr *getRightOp() const;
        Expr *getLeftOp() const;
        char getOp();
        virtual void dump(std::ostream&) const;
        virtual Node *copy();
        bool operator==(Node&);
};

class UnaryExpr : public Expr {
    protected:
        char op;

    public:
        UnaryExpr(Expr *lOp, char op, Node *parent = NULL);
        Expr *getExpr() const;
        char getOp() const;
        virtual void dump(std::ostream&) const;
        virtual Node *copy();
        bool operator==(Node&);
};

class Symbol;
class DiffExpr : public Expr {
    protected:
        std::string order;

    public:
        DiffExpr(Expr *, Identifier *, std::string order = std::string("1"),
                Node *p = NULL);
        Expr *getExpr();
        Identifier *getVar();
        void setOrder(std::string);
        std::string getOrder();
        virtual void dump(std::ostream&) const;
        virtual Node *copy();
        bool operator==(Node&);
};

class ExprLst : public std::vector<Expr *> {
};

class Identifier : public Expr {
    protected:
        std::string name;

    public:
        Identifier(std::string *n, Node *parent = NULL);
        Identifier(std::string n, Node *parent = NULL);
        std::string getName();
        virtual void dump(std::ostream& os) const;
        virtual Node *copy();
        bool operator==(Node&);
};

class FuncCall : public Identifier {
    public:
        FuncCall(std::string, ExprLst *args = NULL, Node *p = NULL);
        FuncCall(std::string, Expr *arg, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        ExprLst *getArgs();
        virtual Node *copy();
        bool operator==(Node&);
};

class ArrayExpr : public Identifier {
    public:
        ArrayExpr(std::string name, ExprLst *indices, Node *p = NULL);
        ExprLst *getIndices();
        virtual Node *copy();
        bool operator==(Node&);
};

class Decl : public Node {
    /// this flag tells whether the declaration have a degree l = 0
    /// this obviously only makes sense for lvar(variable) declaration
    bool noLM0;

    public:
        Decl(Expr *, Expr *, bool = false);
        Expr *getLHS();
        Expr *getDef();
        virtual void dump(std::ostream& os) const;
        virtual Node *copy();
        bool operator==(Node&);
        bool getLM0();
};

class DeclLst : public std::list<Decl *> {
};

class BCLst;
class Equation : public Node {
    protected:
        std::string name;

    public:
        Equation(Expr *lhs, Expr *rhs, BCLst *bc, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        Expr *getLHS();
        Expr *getRHS();
        BCLst *getBCs();
        virtual Node *copy();
        bool operator==(Node&);
        void setName(std::string&);
        std::string& getName();
        void setBCs(ir::BCLst *);
};

class EqLst : public std::list<Equation *> {
};

class BC : public Node {
    protected:
        int eqLoc;  // this encodes the place to set the BC in the linear system
                    // (i.e, the line in the matrix)

    public:
        BC(Equation *cond, Equation *loc, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        virtual Node *copy();
        bool operator==(Node&);
        Equation *getLoc();
        Equation *getCond();
        void setEqLoc(int);
        int getEqLoc();
};

class BCLst : public std::list<BC *> {
};

class Program : public DOT {
    private:
        SymTab * symTab;
        DeclLst *decls;
        EqLst *eqs;

    public:
        Program(std::string, SymTab *, DeclLst *decls, EqLst *eqs);
        void buildSymTab();
        virtual void dumpDOT(std::ostream& os, std::string title, bool root = true) const;
        SymTab *getSymTab();
        EqLst *getEqs();
        DeclLst *getDecls();

        void replace(Node *, Node *);

        const std::string filename;
};

class IndexRange : public BinExpr {
    public:
        IndexRange(Expr *lb, Expr *ub, Node *p = NULL);
        Expr *getLB();
        Expr *getUB();
};

class IndexRangeLst : public std::list<IndexRange *> {
};

class Symbol {
    protected:
        std::string name;
        Expr *expr;
        bool internal;
    public:
        Symbol(std::string name, Expr *def = NULL, bool internal = false);
        virtual std::string& getName();
        Expr *getDef();
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
        Variable(std::string n, Expr *def = NULL, bool internal = false);
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

}

std::ostream& operator<<(std::ostream& , ir::Node&);

#endif
