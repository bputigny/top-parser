#include "IR.h"
#include "SymTab.h"
#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <typeinfo>
#include <cassert>
#include <algorithm>

namespace ir {

bool Node::operator!=(Node& n) {
    return !((*this) == n);
}

bool Node::contains(ir::Node& node) {
    if (*this == node) {
        return true;
    }
    for (auto c: this->getChildren()) {
        if (c->contains(node))
            return true;
    }
    return false;
}

void Program::replace(Node *n0, Node *n1) {
    // TODO: check derived types of n0 and n1 are compatible (e.g. do not
    // replace an equation with an expression)

    assert(n0);
    assert(n1);

    std::function<void (ir::Node *)> replaceNode = [n0, n1, &replaceNode] (ir::Node *n) {
        assert(n);
        for (auto c: n->getChildren()) {
            assert(c);
            replaceNode(c);
            c->setParent(n);
        }
        std::replace(n->getChildren().begin(), n->getChildren().end(), n0, n1);
    };

    for (auto e: this->getEqs()) {
        replaceNode(e);
        for (auto bc: *e->getBCs()) {
            replaceNode(bc);
        }
    }
    for (auto e: this->getEqs()) {
        e->setParents();
        for (auto bc: *e->getBCs()) {
            bc->setParents();
        }
    }
}

int Node::nNode = 0;

int Node::getNodeNumber() {
    return Node::nNode;
}

Node::Node(Node *p) : children(), srcLoc("unknown") {
    nNode++;
    parent = p;
    clearOnDelete = false;
}

Node *Node::getParent() const {
    return parent;
}

void Node::dump(std::ostream& os) const {
    os << "Node " << (this);
}

void Node::setParent(ir::Node *p) {
    this->parent = this;
}

void Node::setParents() {
    for (auto c: children) {
        c->setParents();
        c->parent = this;
    }
}

void Node::dumpDOT(std::ostream& os, std::string title, bool root) const {
    if (root) {
        os << "digraph ir {\n";
        if (title != "") {
            os << "graph [label=\"" << title << "\", labelloc=t, fontsize=20];\n";
        }
        os << "node [shape = Mrecord]\n";
    }
    os << (long) this << " [label=\"";
    this->dump(os);
    os << "\"]\n";

    for (auto c:children) {
        os << (long) this << " -> " << (long) c << "\n";
    }

    for (auto c:children) {
        c->dumpDOT(os, title, false);
    }
    if (root) {
        os << "}\n";
    }
}

std::vector<Node *>& Node::getChildren() {
    return children;
}

void Node::clear() {
    for(auto c: children) {
        c->clear();
        // if (this->clearOnDelete)
        delete c;
    }
    children.clear();
}

Node::~Node() {
    if (clearOnDelete) {
        this->clear();
    }
    nNode--;
}

Program::Program(std::string filename, SymTab *symTab, DeclLst *decls, EqLst *eqs) :
filename(filename) {
    this->symTab = symTab;
    this->decls = decls;
    this->eqs = eqs;
}

Program::~Program() {
    for (auto d: *decls) {
        d->clear();
        delete d;
    }
    for (auto e: *eqs) {
        e->clear();
        delete e;
    }
    delete symTab;
    delete decls;
    delete eqs;
}

void Program::buildSymTab() {
    // First add definitions
    for (auto d: *decls) {
        if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(d->getLHS())) {
            ir::Variable *var = new ir::Variable(id->name, id->vectComponent,
                    d->getDef());
            symTab->add(var);
        }
        else {
            err << "affecting expression to non variable type\n";
        }
    }

    // Look for undefined symbols
    for (auto e:*eqs) {
        Analysis<ir::Identifier> a = Analysis<ir::Identifier>();
        auto checkDefined = [this] (ir::Identifier *s) -> void {
            std::string name = s->name;
            if (this->symTab->search(name) == NULL)
                err << "undefined symbol: `" << name << "'\n";
        };
        a.run(checkDefined, e);
    }
}

void Program::dumpDOT(std::ostream& os, std::string title, bool root) const {
    if (root) {
        os << "digraph prog {\n";
        os << "node [shape = Mrecord]\n";
        os << "root -> DEFs\n";
        os << "root -> EQs\n";
        os << "root -> BCs\n";
    }
    for(auto d: *decls) {
        os << "DEFs -> " << (long) d << "\n";
        d->dumpDOT(os, title, false);
    }

    for(auto e: *eqs) {
        os << "EQs -> " << (long) e << "\n";
        e->dumpDOT(os, title, false);
    }

    // for(auto c: *bcs) {
    //     os << "BCs -> " << (long)c << "\n";
    //     c->dumpDOT(os, title, false);
    // }
    if (root) {
        os << "}\n";
    }
}

SymTab& Program::getSymTab() {
    return *symTab;
}

EqLst& Program::getEqs() {
    return *eqs;
}

DeclLst& Program::getDecls() {
    return *decls;
}

Decl::Decl(Expr *lhs, Expr *rhs) {
    children.push_back(lhs);
    children.push_back(rhs);
}

void Decl::dump(std::ostream& os) const {
    os << ":=";
}

bool Decl::operator==(Node& n) {
    err << "not yet implemented\n";
    return false;
}

Expr *Decl::getLHS() const {
    assert(dynamic_cast<Expr *>(children[0]));
    return dynamic_cast<Expr *>(children[0]);
}

Expr *Decl::getDef() const {
    assert(dynamic_cast<Expr *>(children[1]));
    return dynamic_cast<Expr *>(children[1]);
}

Equation::Equation(std::string name,
        Expr *lhs, Expr *rhs, BCLst *bcs, Node *p) : Node(p), name(name) {
    assert(lhs && rhs);
    if ((isScalar(lhs) && isScalar(rhs)) ||
            (isVect(lhs) && isVect(rhs))) {
        children.push_back(lhs);
        children.push_back(rhs);
        if (bcs) {
            for (auto bc:*bcs) {
                children.push_back(bc);
            }
        }
    }
    else {
        err << "both sides of equation should be of same type (scalar or vectorial)\n";
        lhs->display();
        if (rhs)
            rhs->display();
        exit(EXIT_FAILURE);
    }
}

Equation::Equation(std::string name, const Expr& lhs, const Expr& rhs,
        BCLst *bcs, Node *p) : Equation(name, lhs.copy(), rhs.copy(), bcs, p) {
}

Equation::Equation(std::string name, Equation &eq) :
    Node(eq.getParent()), name(name) {
        children.push_back(eq.getLHS());
        children.push_back(eq.getRHS());
        for (auto bc: *eq.getBCs())
            children.push_back(bc);
    }

void Equation::setBCs(ir::BCLst *bcs) {
    for (auto b: *bcs) {
        children.push_back(b);
    }
}

void Equation::dump(std::ostream& os) const {
    os << "=";
}

Expr *Equation::getLHS() const {
    return dynamic_cast<Expr *>(children[0]);
}

Expr *Equation::getRHS() const {
    return dynamic_cast<Expr *>(children[1]);
}

bool Equation::operator==(Node& n) {
    err << "not yet implemented\n";
    return false;
}

BCLst *Equation::getBCs() const {
    BCLst *bcs = new BCLst();
    int n = 0;
    for (auto c: children) {
        if (auto bc = dynamic_cast<ir::BC *>(c)) {
            bcs->push_back(bc);
            n ++;
        }
    }
    return bcs;
}

BC::BC(Equation *cond, Equation *loc, Node *p) : Node(p) {
    children.push_back(cond);
    children.push_back(loc);
}

void BC::dump(std::ostream& os) const {
    os << "BC";
}

Equation *BC::getLoc() const {
    return dynamic_cast<Equation *>(children[1]);
}

Equation *BC::getCond() const {
    return dynamic_cast<Equation *>(children[0]);
}

void BC::setEqLoc(int loc) {
    this->eqLoc = loc;
}

int BC::getEqLoc() const {
    return this->eqLoc;
}

bool BC::operator==(Node& n) {
    err << "not yet implemented\n";
    return false;
}

} // end namespace ir

std::ostream& operator<<(std::ostream& os, ir::Node& node) {
    node.dump(os);
    return os;
}
