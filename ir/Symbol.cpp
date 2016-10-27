#include "IR.h"

namespace ir {

Symbol::Symbol(std::string n, Expr *def, bool internal) : name(n) {
    this->expr = def;
    this->internal = internal;
}

Symbol::~Symbol() {
    if (expr)
        delete expr;
}

Expr *Symbol::getDef() {
    return expr;
}

bool Symbol::isInternal() {
    return internal;
}

Param::Param(std::string n, std::string t, Expr *def) : Symbol(n, def, internal) {
    type = t;
}

std::string Param::getType() {
    return type;
}

Variable::Variable(std::string n, int vc, Expr *def, bool internal) :
    Symbol(n, def, internal), vectComponent(vc) { }

Array::Array(std::string n, int ndim, Expr *def, bool internal) :
    Variable(n, 0, def, internal), ndim(ndim) { }

Function::Function(std::string name, int nparams) :
    Symbol(name, NULL, true) { 
        this->nparams = nparams;
}

Field::Field(std::string name) :
    Symbol(name, NULL, false) {}

Scalar::Scalar(std::string name) :
    Symbol(name, NULL, false) {}

}
