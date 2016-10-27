#include "IR.h"

#include <cassert>

namespace ir {

bool isScalar(Expr *e) {
    if (dynamic_cast<ScalarExpr *>(e))
        return true;
    return false;
}

bool isVect(Expr *e) {
    if (dynamic_cast<VectExpr *>(e))
        return true;
    return false;
}

ScalarExpr *scalar(Expr *e) {
    if (auto s = dynamic_cast<ScalarExpr *>(e))
        return s;
    err << "cannot cast vector expression to scalar expression\n";
    exit(EXIT_FAILURE);
}

ScalarExpr& scalar(Expr& e) {
    return dynamic_cast<ScalarExpr&>(e);
}

VectExpr *vector(Expr *e) {
    if (auto s = dynamic_cast<VectExpr *>(e))
        return s;
    err << "cannot cast scalar expression to vector expression\n";
    exit(EXIT_FAILURE);
}

VectExpr& vector(Expr& e) {
    return dynamic_cast<VectExpr&>(e);
}

Expr *Expr::copy() const {
    if (auto e = dynamic_cast<const IndexRange *>(this)) {
        return new IndexRange(*e);
    }
    else if (auto e = dynamic_cast<const BinExpr *>(this)) {
        return new BinExpr(*e);
    }
    else if (auto e = dynamic_cast<const DiffExpr *>(this)) {
        return new DiffExpr(*e);
    }
    else if (auto e = dynamic_cast<const ArrayExpr *>(this)) {
        return new ArrayExpr(*e);
    }
    else if (auto e = dynamic_cast<const FuncCall *>(this)) {
        return new FuncCall(*e);
    }
    else if (auto e = dynamic_cast<const Identifier *>(this)) {
        return new Identifier(*e);
    }
    else if (auto e = dynamic_cast<const UnaryExpr *>(this)) {
        return new UnaryExpr(*e);
    }
    else if (auto e = dynamic_cast<const Value<int> *>(this)) {
        return new Value<int>(*e);
    }
    else if (auto e = dynamic_cast<const Value<float> *>(this)) {
        return new Value<float>(*e);
    }
    else if (auto e = dynamic_cast<const VectExpr *>(this)) {
        return new VectExpr(*e);
    }
    err << "copy of node type not yet implemented\n";
    exit(EXIT_FAILURE);
}

Expr::Expr(Node *p) : Node(p), priority(5) {}
Expr::Expr(int priority, Node *p) : Node(p), priority(priority) {}
Expr::~Expr() { };

ScalarExpr::ScalarExpr(int priority, Node *p) : Expr(priority, p) { }
ScalarExpr::ScalarExpr(Node *p) : Expr(p) { }

int getPriority(char c) {
    switch(c) {
        case '^':
            return 3;
            break;
        case '*':
        case '/':
            return 2;
            break;
        case '+':
        case '-':
            return 1;
            break;
    }
    err << "unsupported binary operator: \"" << c << "\"";
    exit(EXIT_FAILURE);
}

BinExpr::BinExpr(ScalarExpr *lOp, char op, ScalarExpr *rOp, Node *p) : ScalarExpr(getPriority(op), p) {
    children.push_back(lOp);
    children.push_back(rOp);
    this->op = op;
}

BinExpr::BinExpr(const BinExpr& be) :
    BinExpr(*be.getLeftOp(),
            be.getOp(),
            *be.getRightOp()) { }

BinExpr::BinExpr(const ScalarExpr& lOp, char op, const ScalarExpr& rOp, Node *parent) :
    BinExpr(scalar(lOp.copy()), op, scalar(rOp.copy()), parent) {
    clearOnDelete = true;
    }

void BinExpr::dump(std::ostream &os) const {
    os << op;
}

ScalarExpr *BinExpr::getLeftOp() const {
    assert(dynamic_cast<ScalarExpr *>(children[0]));
    return dynamic_cast<ScalarExpr *>(children[0]);
}

ScalarExpr *BinExpr::getRightOp() const {
    assert(dynamic_cast<ScalarExpr *>(children[1]));
    return dynamic_cast<ScalarExpr *>(children[1]);
}

char BinExpr::getOp() const {
    return op;
}

bool BinExpr::operator==(Node& n) {
    try {
        BinExpr& be = dynamic_cast<BinExpr&>(n);
        return this->getOp() == be.getOp() &&
            *this->getLeftOp() == *be.getLeftOp() &&
            *this->getRightOp() == *be.getRightOp();

    }
    catch (std::bad_cast) {
        return false;
    }
}

BinExpr ScalarExpr::op(const ScalarExpr& s, char op) const {
    return BinExpr(*this, op, s);
}

VectExpr ScalarExpr::op(const VectExpr& v, char op) const {
    ir::BinExpr x1 = BinExpr(*this, op, *v.getX());
    ir::BinExpr x2 = BinExpr(*this, op, *v.getY());
    ir::BinExpr x3 = BinExpr(*this, op, *v.getZ());

    return VectExpr(x1, x2, x3);
}

BinExpr ScalarExpr::operator+(const ScalarExpr& s) const {
    return this->op(s, '+');
}
VectExpr ScalarExpr::operator+(const VectExpr& v) const {
    return this->op(v, '+');
}
BinExpr ScalarExpr::operator-(const ScalarExpr& s) const {
    return this->op(s, '-');
}
VectExpr ScalarExpr::operator-(const VectExpr& v) const {
    return this->op(v, '-');
}
BinExpr ScalarExpr::operator*(const ScalarExpr& s) const {
    return this->op(s, '*');
}
VectExpr ScalarExpr::operator*(const VectExpr& v) const {
    return this->op(v, '*');
}
BinExpr ScalarExpr::operator/(const ScalarExpr& s) const {
    return this->op(s, '/');
}
VectExpr ScalarExpr::operator/(const VectExpr& v) const {
    return this->op(v, '/');
}
BinExpr ScalarExpr::operator^(const ScalarExpr& s) const {
    return this->op(s, '^');
}
VectExpr ScalarExpr::operator^(const VectExpr& v) const {
    return this->op(v, '^');
}

VectExpr VectExpr::op(const ScalarExpr& s, char op) const {
    BinExpr v1 = BinExpr(*this->getX(), op, s);
    BinExpr v2 = BinExpr(*this->getY(), op, s);
    BinExpr v3 = BinExpr(*this->getZ(), op, s);

    return VectExpr(v1, v2, v3);
}

VectExpr VectExpr::op(const VectExpr& v, char op) const {
    BinExpr v1 = BinExpr(*this->getX(), op, *v.getX());
    BinExpr v2 = BinExpr(*this->getY(), op, *v.getY());
    BinExpr v3 = BinExpr(*this->getZ(), op, *v.getZ());

    return VectExpr(v1, v2, v3);
}

VectExpr VectExpr::operator+(const VectExpr& v) const {
    return this->op(v, '+');
}
VectExpr VectExpr::operator-(const VectExpr& v) const {
    return this->op(v, '-');
}

VectExpr VectExpr::operator*(const VectExpr& v) const {
    return this->op(v, '*');
}

VectExpr VectExpr::operator/(const VectExpr& v) const {
    return this->op(v, '/');
}

VectExpr VectExpr::operator^(const VectExpr& v) const {
    return this->op(v, '^');
}
VectExpr VectExpr::operator+(const ScalarExpr& s) const {
    return this->op(s, '+');
}
VectExpr VectExpr::operator-(const ScalarExpr& s) const {
    return this->op(s, '-');
}

VectExpr VectExpr::operator*(const ScalarExpr& s) const {
    return this->op(s, '*');
}

VectExpr VectExpr::operator/(const ScalarExpr& s) const {
    return this->op(s, '/');
}

VectExpr VectExpr::operator^(const ScalarExpr& s) const {
    return this->op(s, '^');
}

UnaryExpr::UnaryExpr(ScalarExpr *expr, char op, Node *p) : ScalarExpr(p) {
    children.push_back(expr);
    this->op = op;
}

UnaryExpr::UnaryExpr(const ScalarExpr& lOp, char op, Node *parent) :
    UnaryExpr(scalar(lOp.copy()), op, parent) {
        clearOnDelete = true;
    }

UnaryExpr::UnaryExpr(const UnaryExpr& ue) :
    UnaryExpr(scalar(ue.getExpr()->copy()),
            ue.getOp()) {}

ScalarExpr *UnaryExpr::getExpr() const {
    assert(dynamic_cast<ScalarExpr *>(children[0]));
    return dynamic_cast<ScalarExpr *>(children[0]);
}

char UnaryExpr::getOp() const {
    return op;
}

void UnaryExpr::dump(std::ostream& os) const {
    os << op; // << *this->getExpr();
}

bool UnaryExpr::operator==(Node& n) {
    try {
        UnaryExpr& ue = dynamic_cast<UnaryExpr&>(n);
        return getOp() == ue.getOp() &&
            *this->getExpr() == *ue.getExpr();
    }
    catch (std::bad_cast) {
        return false;
    }
}

DiffExpr::DiffExpr(Expr *expr, Identifier *id,
        std::string order, Node *p) : ScalarExpr(p) {
    this->children.push_back(expr);
    this->children.push_back(id);
    this->order = order;
}

DiffExpr::DiffExpr(const DiffExpr& de) :
    DiffExpr(de.getExpr()->copy(),
            new Identifier(*de.getVar()),
            de.getOrder()) { }

DiffExpr::DiffExpr(const Expr& e, const Identifier& id, std::string order, Node *p) :
    DiffExpr(e.copy(), dynamic_cast<Identifier *>(id.copy()), order, p) {
        clearOnDelete = true;
    }

Expr *DiffExpr::getExpr() const {
    assert(dynamic_cast<Expr *>(this->children[0]));
    return dynamic_cast<Expr *>(this->children[0]);
}

bool DiffExpr::operator==(Node& n) {
    try {
        DiffExpr& de = dynamic_cast<DiffExpr&>(n);
        return this->getVar() == de.getVar() &&
            *this->getExpr() == *de.getExpr() &&
            this->getOrder() == de.getOrder();
    }
    catch (std::bad_cast) {
        return false;
    }
}

Identifier *DiffExpr::getVar() const {
    assert(dynamic_cast<Identifier *>(this->children[1]));
    return dynamic_cast<Identifier *>(this->children[1]);
}

void DiffExpr::setOrder(std::string order) {
    this->order = order;
}

std::string DiffExpr::getOrder() const {
    return order;
}

Identifier::Identifier(std::string n, int vectComponent, Node *p) :
    ScalarExpr(p), name(n), vectComponent(vectComponent) { }

Identifier::Identifier(const Identifier& id) :
    Identifier(id.name, id.vectComponent) { }

void Identifier::dump(std::ostream& os) const {
    os << "ID: " << name;
}

bool Identifier::operator==(Node& n) {
    try {
        Identifier& id = dynamic_cast<Identifier&>(n);
        return this->name == id.name;
    }
    catch (std::bad_cast) {
        return false;
    }
}

FuncCall::FuncCall(std::string name, ExprLst *args, Node *p) : Identifier(name, 0, p) {
    for (auto c: *args) {
        children.push_back(c->copy());
    }
}

FuncCall::FuncCall(std::string name, Expr *arg, Node *p) : Identifier(name, 0, p) {
    children.push_back(arg);
}

FuncCall::FuncCall(const FuncCall& fc) :
    FuncCall(fc.name, new ExprLst(fc.getArgs())) { }

FuncCall::FuncCall(std::string name, const Expr& arg, Node *p) :
    FuncCall(name, arg.copy(), p) {
        clearOnDelete = true;
    }

void FuncCall::dump(std::ostream& os) const {
    os << "Func: " << name;
}

ExprLst FuncCall::getArgs() const {
    ExprLst ret;
    for (auto arg: children) {
        if (!dynamic_cast<Expr *>(arg)) {
            err << "arg is not an expression\n";
            exit(EXIT_FAILURE);
        }
        ret.push_back(dynamic_cast<Expr *>(arg));
    }
    return ret;
}

bool FuncCall::operator==(Node& n) {
    try {
        FuncCall& fc = dynamic_cast<FuncCall&>(n);
        bool sameArgs;
        if (this->name != fc.name)
            return false;
        if (this->getArgs().size() == fc.getArgs().size())
            sameArgs = true;
        else
            return false;
        for (int i=0; i<this->getArgs().size(); i++) {
            sameArgs = sameArgs && *this->getArgs()[i] == *fc.getArgs()[i];
        }
        return sameArgs;
    }
    catch (std::bad_cast) {
        return false;
    }
}

ArrayExpr::ArrayExpr(std::string name, ExprLst *indices, Node *p) : Identifier(name, 0, p) {
    for(auto c: *indices)
        children.push_back(c);
}

ArrayExpr::ArrayExpr(const ArrayExpr& ae) :
    ArrayExpr(ae.name, new ExprLst(ae.getIndices())) { }

ExprLst ArrayExpr::getIndices() const {
    ExprLst ret;
    for (auto c: children) {
        ret.push_back(dynamic_cast<Expr *>(c));
    }
    return ret;
}

bool ArrayExpr::operator==(Node& n) {
    err << "not yet implemented\n";
    return false;
}

IndexRange::IndexRange(ScalarExpr *lb, ScalarExpr *ub, Node *p) :
    BinExpr(lb, ':', ub, p) { }

IndexRange::IndexRange(const IndexRange& ir) :
    IndexRange(
            scalar(ir.getLB()->copy()),
            scalar(ir.getUB()->copy())) { }

ScalarExpr *IndexRange::getLB() const {
    assert(dynamic_cast<ScalarExpr *>(children[0]));
    return dynamic_cast<ScalarExpr *>(children[0]);
}

ScalarExpr *IndexRange::getUB() const {
    assert(dynamic_cast<ScalarExpr *>(children[1]));
    return dynamic_cast<ScalarExpr *>(children[1]);
}

void DiffExpr::dump(std::ostream &os) const {
    os << "Diff (order: " << this->order << ")";
}

VectExpr::VectExpr(ScalarExpr *x, ScalarExpr *y, ScalarExpr *z) : Expr() {
    children.push_back(x);
    children.push_back(y);
    children.push_back(z);
}

VectExpr::VectExpr(const ScalarExpr& x, const ScalarExpr& y, const ScalarExpr& z) :
    VectExpr(scalar(x.copy()), scalar(y.copy()), scalar(z.copy())) {
    clearOnDelete = true;
    }

VectExpr::VectExpr(const VectExpr& ve) :
    VectExpr(*ve.getX(), *ve.getY(), *ve.getZ()) { }

ScalarExpr *VectExpr::getX() const {
    return getComponent(0);
}

ScalarExpr *VectExpr::getY() const {
    return getComponent(1);
}

ScalarExpr *VectExpr::getZ() const {
    return getComponent(2);
}

ScalarExpr *VectExpr::getComponent(int n) const {
    assert(n >= 0 && n <= 3);
    if (auto c = dynamic_cast<ScalarExpr *>(children[n])) {
        return c;
    }
    else {
        err << "vector components should be expression\n";
        exit(EXIT_FAILURE);
    }
}

void VectExpr::dump(std::ostream& os) const {
    os << "[]";
}

bool VectExpr::operator==(Node& n) {
    try {
        VectExpr& ve = dynamic_cast<VectExpr&>(n);
        return
            *getX() == *ve.getX() &&
            *getY() == *ve.getY() &&
            *getZ() == *ve.getZ();
    }
    catch (std::bad_cast) {
        return false;
    }
}

BinExpr *div(Expr &e) {
    try {
        VectExpr& ve = dynamic_cast<VectExpr&>(e);
        ScalarExpr *x = ve.getX();
        ScalarExpr *y = ve.getY();
        ScalarExpr *z = ve.getZ();
        ScalarExpr *dx = new DiffExpr(x, new Identifier("r"));
        ScalarExpr *dy = new DiffExpr(y, new Identifier("theta"));
        ScalarExpr *dz = new DiffExpr(z, new Identifier("phi"));
        return new BinExpr(dx, '+', new BinExpr(dy, '+', dz));
    }
    catch (std::bad_cast) {
        err << "div can only be applied to vector expression\n";
        exit(EXIT_FAILURE);
    }
}

UnaryExpr operator-(ScalarExpr& s) {
    return UnaryExpr(s, '-');
}

VectExpr operator-(VectExpr& v) {
    UnaryExpr x = -(*v.getX());
    UnaryExpr y = -(*v.getY());
    UnaryExpr z = -(*v.getZ());
    return VectExpr(
            x,
            y,
            z);
}

BinExpr operator^(const ScalarExpr& s, int p) {
    return s ^ ir::Value<int>(p);
}

VectExpr operator^(const VectExpr& v, int p) {
    return v ^ ir::Value<int>(p);
}

BinExpr operator/(float f, const ScalarExpr& s) {
    return ir::Value<float>(f) / s;
}

VectExpr operator/(float f, const VectExpr& v) {
    return ir::Value<float>(f) / v;
}

BinExpr operator/(int i, const ScalarExpr& s) {
    return ir::Value<float>(i) / s;
}

VectExpr operator/(int i, const VectExpr& v) {
    return ir::Value<float>(i) / v;
}

FuncCall sin(const ScalarExpr& s) {
    return FuncCall("sin", s);
}

FuncCall cos(const ScalarExpr& s) {
    return FuncCall("cos", s);
}

Expr& operator+(const Expr& e1, const Expr& e2) {
    try {
        const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
        const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
        return *(s1 + s2).copy();
    }
    catch (std::bad_cast) {
        try {
            const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
            const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
            return *(v1 + v2).copy();
        }
        catch (std::bad_cast) {
            try {
                const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
                const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
                return *(s1 + v2).copy();
            }
            catch (std::bad_cast) {
                const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
                const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
                return *(v1 + s2).copy();
            }
        }
    }
}

Expr& operator-(const Expr& e1, const Expr& e2) {
    try {
        const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
        const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
        return *(s1 - s2).copy();
    }
    catch (std::bad_cast) {
        try {
            const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
            const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
            return *(v1 - v2).copy();
        }
        catch (std::bad_cast) {
            try {
                const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
                const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
                return *(s1 - v2).copy();
            }
            catch (std::bad_cast) {
                const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
                const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
                return *(v1 - s2).copy();
            }
        }
    }
}

Expr& operator*(const Expr& e1, const Expr& e2) {
    try {
        const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
        const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
        return *(s1 * s2).copy();
    }
    catch (std::bad_cast) {
        try {
            const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
            const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
            return *(v1 * v2).copy();
        }
        catch (std::bad_cast) {
            try {
                const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
                const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
                return *(s1 * v2).copy();
            }
            catch (std::bad_cast) {
                const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
                const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
                return *(v1 * s2).copy();
            }
        }
    }
}

Expr& operator/(const Expr& e1, const Expr& e2) {
    try {
        const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
        const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
        return *(s1 / s2).copy();
    }
    catch (std::bad_cast) {
        try {
            const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
            const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
            return *(v1 / v2).copy();
        }
        catch (std::bad_cast) {
            try {
                const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
                const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
                return *(s1 / v2).copy();
            }
            catch (std::bad_cast) {
                const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
                const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
                return *(v1 / s2).copy();
            }
        }
    }
}

Expr& operator^(const Expr& e1, const Expr& e2) {
    try {
        const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
        const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
        return *(s1 ^ s2).copy();
    }
    catch (std::bad_cast) {
        try {
            const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
            const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
            return *(v1 ^ v2).copy();
        }
        catch (std::bad_cast) {
            try {
                const ScalarExpr& s1 = dynamic_cast<const ScalarExpr&>(e1);
                const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
                return *(s1 ^ v2).copy();
            }
            catch (std::bad_cast) {
                const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
                const ScalarExpr& s2 = dynamic_cast<const ScalarExpr&>(e2);
                return *(v1 ^ s2).copy();
            }
        }
    }
}


VectExpr crossProduct(const Expr& e1, const Expr& e2) {
    try {
        const VectExpr& u = dynamic_cast<const VectExpr&>(e1);
        const VectExpr& v = dynamic_cast<const VectExpr&>(e2);

        ScalarExpr& u1 = *u.getX();
        ScalarExpr& u2 = *u.getY();
        ScalarExpr& u3 = *u.getZ();

        ScalarExpr& v1 = *v.getX();
        ScalarExpr& v2 = *v.getY();
        ScalarExpr& v3 = *v.getZ();

        return ir::VectExpr(
                u2*v3 - u3*v2,
                u3*v1 - u1*v3,
                u1*v2 - u2*v1);
    }
    catch (std::bad_cast) {
        err << "cross product con only be applied to vectors\n";
        exit(EXIT_FAILURE);
    }
}

BinExpr dotProduct(const Expr& e1, const Expr& e2) {
    try {
        const VectExpr& v1 = dynamic_cast<const VectExpr&>(e1);
        const VectExpr& v2 = dynamic_cast<const VectExpr&>(e2);
        return *v1.getX() * *v2.getX() +
            *v1.getY() * *v2.getY() +
            *v1.getZ() * *v2.getZ();
    }
    catch (std::bad_cast) {
        err << "dot product con only be applied to vectors\n";
        exit(EXIT_FAILURE);
    }
}

} // end namespace ir
