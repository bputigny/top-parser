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
/// Base class to represent programs AST
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
        Node *getParent();
        virtual void dump(std::ostream&) const;
        virtual void dumpDOT(std::ostream&, bool root = true) const;
        std::vector<Node *>& getChildren();
        std::vector<Node *> getLeafs();
        bool isLeaf();
        std::string getSourceLocation();
        void setSourceLocation(std::string);
        ~Node();
        void replace(Node *, Node *);
        static int getNodeNumber();
};

class Expr : public Node {
    public:
        Expr(Node *p = NULL);
};

template <class T>
class Value : public Expr {
    T value;
    public:
        inline Value(T val, Node *p = NULL) : Expr(p) {
            value = val;
        }
        inline T getValue() {
            return value;
        }
        void dump(std::ostream &os) const {
            os << "Val: " << value;
        }
};

class BinExpr : public Expr {
    protected:
        char op;

    public:
        BinExpr(Expr *lOp, char op, Expr *rOp, Node *parent = NULL);
        Expr *getRightOp() const;
        Expr *getLeftOp() const;
        virtual void dump(std::ostream&) const;
};

class UnaryExpr : public Expr {
    protected:
        char op;

    public:
        UnaryExpr(Expr *lOp, char op, Node *parent = NULL);
        Expr *getExpr() const;
        char getOp() const;
        virtual void dump(std::ostream&) const;
};

class ExprLst : public std::list<Expr *> {
};

class Identifier : public Expr {
    protected:
        std::string name;
    public:
        Identifier(std::string *n, Node *parent = NULL);
        Identifier(std::string n, Node *parent = NULL);
        std::string getName();
        virtual void dump(std::ostream& os) const;
};

class FuncCall : public Identifier {
    public:
        FuncCall(std::string, ExprLst *args = NULL, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        ExprLst *getArgs();
};

class ArrayExpr : public Identifier {
    public:
        ArrayExpr(std::string name, ExprLst *indices, Node *p = NULL);
};

class Decl : public Node {
    public:
        Decl(Expr *, Expr *);
        Expr *getLHS();
        Expr *getDef();
        virtual void dump(std::ostream& os) const;
};

class DeclLst : public std::list<Decl *> {
};

class Equation : public Node {
    public:
        Equation(Expr *lhs, Expr *rhs, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
        Expr *getLHS();
        Expr *getRHS();
};

class EqLst : public std::list<Equation *> {
};

class BC : public Node {
    public:
        BC(Equation *cond, Equation *loc, Node *p = NULL);
        virtual void dump(std::ostream& os) const;
};

class BCLst : public std::list<BC *> {
};

class Program : public DOT {
    private:
        SymTab * symTab;
        DeclLst *decls;
        EqLst *eqs;
        BCLst *bcs;
    public:
        Program(SymTab *, DeclLst *decls, EqLst *eqs, BCLst *bcs);
        void buildSymTab();
        virtual void dumpDOT(std::ostream& os, bool root = true) const;
        SymTab *getSymTab();
        EqLst *getEqs();
        BCLst *getBCs();
};

class IndexRange : public BinExpr {
    public:
        IndexRange(Expr *lb, Expr *ub, Node *p = NULL);
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

}

std::ostream& operator<<(std::ostream& , ir::Node&);

#endif
