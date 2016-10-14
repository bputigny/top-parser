#include "IR.h"

#include <fstream>

int main() {

    Printer::init(2);
    std::ofstream file;

#if 0

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

    ir::Node *rOp = e->getRightOp();

    // e->display("IR0");

    ir::Node *newNode = new ir::Value<float>(42);
    ir::Node::replace(&rOp, &newNode);
    // e->display("IR1");

    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";
    log() << "delete rOp\n";
    rOp->clear();
    delete rOp;
    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";

    log() << "delete e\n";
    e->clear();
    delete e;
    log() << "allocated Nodes: " << ir::Node::getNodeNumber() << "\n";

#endif

    {
    ir::Expr *x = new ir::Identifier("x");
    ir::Expr *v1 = new ir::Value<float>(1.0);
    ir::Expr *v2 = new ir::Value<float>(2.0);
    ir::Expr *v3 = new ir::Value<float>(3.0);
    ir::Expr *v4 = new ir::Identifier("y");

    ir::Expr *root = new ir::BinExpr(x, '+', new ir::BinExpr(v1, '+', v2));

    ir::EqLst *eqs = new ir::EqLst();
    eqs->push_back(new ir::Equation(root, v3, NULL));
    ir::Program *p = new ir::Program(NULL, NULL, eqs);

    // for (auto e: *eqs) {
    //     e->display("expr");
    // }

    root->display("root");
    p->replace(v1, v4);
    root->display("root replaced");

    p->replace(v4, v1);
    root->display("root back inplace");


    // for (auto e: *eqs) {
    //     e->display("replaced");
    // }

    }

#if 1
    ir::Value<int> i0 = ir::Value<int>(0);
    ir::Value<int> i1 = ir::Value<int>(0);
    ir::Value<float> f0 = ir::Value<float>(0);

    ir::Identifier id("id");
    ir::Identifier l("l");
    ir::Identifier id2("id");

    ir::UnaryExpr ll(new ir::Identifier("l"), '\'');
    ir::UnaryExpr ll2(new ir::Identifier("l"), '\'');
    ir::UnaryExpr ll3(new ir::Identifier("ll"), '\'');

    std::cout << "0.0 == 0: " << (i0 == f0) << "\n";
    std::cout << "0 == 0: " << (i0 == i1) << "\n";
    std::cout << "id == id2: " << (id == id2) << "\n";
    std::cout << "id == l: " << (id == l) << "\n";

    std::cout << "l' == l': " << (ll == ll2) << "\n";
    std::cout << "l' == l': " << (ll == ll3) << "\n";

#endif

    return 0;
}
