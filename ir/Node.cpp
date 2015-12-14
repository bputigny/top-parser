#include "IR.h"
#include "SymTab.h"
#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <typeinfo>
#include <cassert>
#include <algorithm>

namespace ir {

int Node::nNode = 0;

int Node::getNodeNumber() {
    return Node::nNode;
}

Node::Node(Node *p) : children() {
    nNode ++;
    parent = p;
    srcLoc = "unknown";
}

Node *Node::getParent() {
    return parent;
}

void Node::dump(std::ostream& os) const {
    os << "Node " << (this);
}

void Node::dumpDOT(std::ostream& os, bool root) const {
    if (root) {
        os << "digraph ir {\n";
        os << "node [shape = Mrecord]\n";
    }
    os << (long)this << " [label=\"";
    this->dump(os);
    os << "\"]\n";

    for (auto c:children) {
        os << (long)this << " -> " << (long)c << "\n";
    }

    for (auto c:children) {
        c->dumpDOT(os, false);
    }
    if (root) {
        os << "}\n";
    }
}

std::vector<Node *>& Node::getChildren() {
    return children;
}

bool Node::isLeaf() {
    return children.empty();
}

std::string Node::getSourceLocation() {
    return srcLoc;
}

Node::~Node() {
    for(auto child: children) {
        delete(child);
    }
    nNode --;
}

void Node::replace(Node *child, Node *with) {
    std::replace(children.begin(), children.end(), child, with);
}

Program::Program(SymTab *symTab, DeclLst *decls, EqLst *eqs, BCLst *bcs) {
    this->symTab = symTab;
    this->decls = decls;
    this->eqs = eqs;
    this->bcs = bcs;
}

void Program::buildSymTab() {
    // First add definitions
    for (auto d:*decls) {
        if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(d->getLHS())) {
            assert(d->getDef());
            ir::Variable *var = new ir::Variable(id->getName(), d->getDef());
            symTab->add(var);
        }
        else {
            err() << "affecting expression to non variable type\n";
        }
    }

    // Look for undefined symbols
    for (auto e:*eqs) {
        Analysis<ir::Identifier> a = Analysis<ir::Identifier>(this);
        auto checkDefined = [this] (ir::Identifier *s) -> void {
            std::string name = s->getName();
            if (this->symTab->search(name) == NULL)
                err() << "undefined symbol: `" << name << "'\n";
        };
        a.run(checkDefined, e);
    }
}

void Program::dumpDOT(std::ostream& os, bool root) const {
    if (root) {
        os << "digraph prog {\n";
        os << "node [shape = Mrecord]\n";
        os << "root -> DEFs\n";
        os << "root -> EQs\n";
        os << "root -> BCs\n";
    }
    for(auto d: *decls) {
        os << "DEFs -> " << (long)d << "\n";
        d->dumpDOT(os, false);
    }

    for(auto e: *eqs) {
        os << "EQs -> " << (long)e << "\n";
        e->dumpDOT(os, false);
    }

    for(auto c: *bcs) {
        os << "BCs -> " << (long)c << "\n";
        c->dumpDOT(os, false);
    }
    if (root) {
        os << "}\n";
    }
}

SymTab *Program::getSymTab() {
    return symTab;
}

EqLst *Program::getEqs() {
    return eqs;
}

BCLst *Program::getBCs() {
    return bcs;
}

Expr::Expr(Node *p) : Node(p) {}

BinExpr::BinExpr(Expr *lOp, char op, Expr *rOp, Node *p) : Expr(p) {
    assert(lOp && rOp);
    children.push_back(lOp);
    children.push_back(rOp);
    this->op = op;
}

UnaryExpr::UnaryExpr(Expr *expr, char op, Node *p) : Expr(p) {
    children.push_back(expr);
    this->op = op;
}

Expr *UnaryExpr::getExpr() const {
    return dynamic_cast<Expr *>(children[0]);
}

void UnaryExpr::dump(std::ostream& os) const {
    os << op << *this->getExpr();
}

void BinExpr::dump(std::ostream &os) const {
    os << op;
}

Expr *BinExpr::getLeftOp() const {
    return dynamic_cast<Expr *>(children[0]);
}

Expr *BinExpr::getRightOp() const {
    return dynamic_cast<Expr *>(children[1]);
}

Identifier::Identifier(std::string *n, Node *p) : name(*n), Expr(p) { }

Identifier::Identifier(std::string n, Node *p) :  Identifier(&n, p) { }

std::string Identifier::getName() {
    return name;
}

void Identifier::dump(std::ostream& os) const {
    os << "ID: " << name;
}

Decl::Decl(Expr *lhs, Expr *rhs) {
    children.push_back(lhs);
    children.push_back(rhs);
}

void Decl::dump(std::ostream& os) const {
    os << ":=";
}

Expr *Decl::getLHS() {
    return dynamic_cast<Expr *>(children[0]);
}

Expr *Decl::getDef() {
    return dynamic_cast<Expr *>(children[1]);
}

Equation::Equation(Expr *lhs, Expr *rhs, Node *p) : Node(p) {
    children.push_back(lhs);
    children.push_back(rhs);
}

void Equation::dump(std::ostream& os) const {
    os << "=";
}

Expr *Equation::getLHS() {
    return dynamic_cast<Expr *>(children[0]);
}

Expr *Equation::getRHS() {
    return dynamic_cast<Expr *>(children[1]);
}

BC::BC(Equation *cond, Equation *loc, Node *p) : Node(p) {
    children.push_back(cond);
    children.push_back(loc);
}

void BC::dump(std::ostream& os) const {
    os << "BC";
}

FuncCall::FuncCall(std::string name, ExprLst *args, Node *p) : Identifier(name, p) {
    for (auto c: *args) {
        children.push_back(c);
    }
}

void FuncCall::dump(std::ostream& os) const {
    os << "Func: " << name;
}

ExprLst *FuncCall::getArgs() {
    ExprLst *ret = new ExprLst();
    for (auto arg: children) {
        ret->push_back(dynamic_cast<Expr *>(arg));
    }
    return ret;
}

ArrayExpr::ArrayExpr(std::string name, ExprLst *indices, Node *p) : Identifier(name, p) {
    for(auto c: *indices)
        children.push_back(c);
}

IndexRange::IndexRange(Expr *lb, Expr *ub, Node *p) : BinExpr(lb, ':', ub, p) { }

}

std::ostream& operator<<(std::ostream& os, ir::Node& node) {
    node.dump(os);
    return os;
}
