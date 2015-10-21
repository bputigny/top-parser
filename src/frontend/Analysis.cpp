#include "config.h"

#include "Analysis.h"
#include "Printer.h"

#include <functional>
#include <cassert>

DeclAnalysis::DeclAnalysis(ir::Declaration &d) : decl(d) { }

DeclAnalysis::~DeclAnalysis() { }

bool DeclAnalysis::isConst() {
    return decl.getExpr().isConst();
}

ProgramAnalysis::~ProgramAnalysis() {}

///
/// Build Symbol Table of declarations of the program
///
void ProgramAnalysis::buildSymTab() {
    SymTab &symTab = prog.getSymTab();

    // add definition's LHS
    for (auto d:this->prog.getDeclarations()) {
        assert(d);
        if (ir::Param *p = dynamic_cast<ir::Param *>(&d->getLHS())) {
            // Don't add parameter to symTab for they are already in
        }
        else if (ir::ArrayExpr *ae = dynamic_cast<ir::ArrayExpr *>(&d->getLHS())) {
            symTab.add(new ir::Array(ae->getName(), &d->getExpr()));
        }
        else if (ir::Identifier *s = dynamic_cast<ir::Identifier *>(&d->getLHS())) {
            symTab.add(new ir::Variable(s->getName(), &d->getExpr()));
        }
        else {
            std::cerr << *d;
            semantic_error("cannot be the LHS of a definition\n");
        }
    }

    std::function<void(ir::Expr &)> parseExpr;
    parseExpr = [&parseExpr, &symTab] (ir::Expr &e) {
        if (ir::FunctionCall *fc = dynamic_cast<ir::FunctionCall *>(&e)) {
            if (symTab.search(fc->getName()) == NULL) {
                error("%s: undefined function '%s'\n",
                        fc->getLocation().c_str(),
                        fc->getName().c_str());
            }
        }
        else if (ir::ArrayExpr *ae = dynamic_cast<ir::ArrayExpr *>(&e)) {
            if (symTab.search(ae->getName()) == NULL) {
                error("%s: undefined array '%s'\n",
                        ae->getLocation().c_str(),
                        ae->getName().c_str());
            }
        }
        else if (ir::Identifier *s = dynamic_cast<ir::Identifier *>(&e)) {
            if (symTab.search(s->getName()) == NULL) {
                error("%s: undefined symbol '%s'\n",
                        s->getLocation().c_str(),
                        s->getName().c_str());
            }
        }
        else if (ir::BinaryExpr *be = dynamic_cast<ir::BinaryExpr *>(&e)) {
            parseExpr(be->getLeftOp());
            parseExpr(be->getRightOp());
        }
        else if (ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(&e)) {
            parseExpr(ue->getOp());
        }
        else if (dynamic_cast<ir::Value<int> *>(&e)) {
        }
        else if (dynamic_cast<ir::Value<float> *>(&e)) {
        }
        else {
            std::cerr << e;
            info("was not parse\n");
        }
    };

    // parse definition's RHS for undef id
    for (auto d:this->prog.getDeclarations()) {
        ir::Expr &e = d->getExpr();
        parseExpr(e);
    }
}

std::list<ir::Variable> *ExprAnalysis::getVariables(ir::Expr &expr) {

    std::list<ir::Variable> *list = new std::list<ir::Variable>();

    if (ir::BinaryExpr *be = dynamic_cast<ir::BinaryExpr *>(&expr))  {
        std::list<ir::Variable> *leftVars = getVariables(be->getLeftOp());
        std::list<ir::Variable> *rightVars = getVariables(be->getRightOp());
        for (auto v:*leftVars) {
            list->push_back(v);
        }
        for (auto v:*rightVars) {
            list->push_back(v);
        }
    }
    else if (ir::UnaryExpr *ue = dynamic_cast<ir::UnaryExpr *>(&expr))  {
        std::list<ir::Variable> *vars = getVariables(ue->getOp());
        for (auto v:*vars) {
            try {
                ir::Variable &var = dynamic_cast<ir::Variable &>(v);
                list->push_back(var);
            }
            catch (std::bad_cast) { }
        }
    }
    else if (ir::FunctionCall *fc = dynamic_cast<ir::FunctionCall *>(&expr))  {
        for (auto arg:fc->getArgs()) {
            std::list<ir::Variable> *vars = getVariables(*arg);
            for (auto v:*vars) {
                try {
                    ir::Variable &var = dynamic_cast<ir::Variable &>(v);
                    list->push_back(v);
                }
                catch (std::bad_cast) { }
            }
        }
    }
    else if (ir::Identifier *id = dynamic_cast<ir::Identifier *>(&expr))  {
        ir::Symbol *s = symTab.search(id->getName());
        if (s == NULL) error("undefined sym: '%s'\n", id->getName().c_str());
        if (s->isInternal()) {
            try {
                ir::Variable &var = dynamic_cast<ir::Variable &>(*s);
                list->push_back(var);
            }
            catch (std::bad_cast) { }
        }
        else {
            ir::Expr *def = s->getDef();
            if (def) {
                std::list<ir::Variable> *vars = getVariables(*def);
                for (auto v:*vars) {
                    try {
                        ir::Variable &var = dynamic_cast<ir::Variable &>(v);
                        list->push_back(v);
                    }
                    catch (std::bad_cast) { }
                }
            }
            else { // This is a parameter or a variable, only add vars
                try {
                    ir::Variable &v = dynamic_cast<ir::Variable &>(*s);
                    list->push_back(v);
                }
                catch (std::bad_cast) { }
            }
        }
    }
    list->sort();
    list->unique();

    // std::cout << "deps of: " << *expr << ":";
    // for (auto v:*list)
    //     std::cout << " " << v;
    // std::cout << "\n";

    return list;
}

void ProgramAnalysis::computeVarSize() {
    for (auto s:prog.getSymTab()) {
        if (s->getDef()) {
            std::vector<int> dim = s->getDef()->getDim();
            s->setDim(dim);
        }
    }
}

