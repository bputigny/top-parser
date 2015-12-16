#include "IR.h"

#include <fstream>

int main() {

    Printer::init(2);
    std::ofstream file;

    ir::Expr *v0 = new ir::Identifier("v0");
    ir::Expr *v1 = new ir::Value<float>(1);
    ir::Expr *v2 = new ir::Value<float>(2);
    ir::Expr *v3 = new ir::Value<float>(3);
    ir::Expr *v4 = new ir::Value<float>(4);

    ir::BinExpr *e = new ir::BinExpr(
            v0,
            '+',
            new ir::BinExpr(
                v1,
                '+',
                new ir::BinExpr(
                    v2,
                    '+',
                    new ir::BinExpr(
                        v3,
                        '+',
                        v4))));

    log() << *e << "\n";

    ir::Node *n0 = e->getLeftOp();
    ir::Node *n1 = e->getRightOp();

    ir::Node *rOp = e->getRightOp();

    file.open("IR0.dot");
    e->dumpDOT(file, true);
    file.close();
    e->display("IR0.dot");

    e->replace(rOp, new ir::Value<float>(42));
    file.open("IR1.dot");
    e->dumpDOT(file, true);
    file.close();
    e->display("IR1.dot");

    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";
    log() << "delete rOp\n";
    rOp->clear();
    delete rOp;
    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";

    log() << "delete e\n";
    e->clear();
    delete e;
    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";
    return 0;
}
