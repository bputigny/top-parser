#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "config.h"
#include "IR.h"

#include <functional>

template<class T>
class Analysis {
    public:
        inline Analysis() { }

        inline void run(std::function<void (T *)> check, ir::Node *root) {
            if (T *n = dynamic_cast<T *>(root)) {
                check(n);
            }
            for (auto c:root->getChildren()) {
                run(check, c);
            }
        }
        inline ir::Node *run(std::function<ir::Node *(T *)> check, ir::Node *root) {
            if (T *n = dynamic_cast<T *>(root)) {
                check(n);
            }
            for (auto c:root->getChildren()) {
                run(check, c);
            }
            return NULL;
        }
};

std::vector<ir::Identifier *> getIds(ir::Expr *e, bool uniq=true);

// ir::Expr *factorize(ir::Expr *);
// ir::Expr *expand(ir::Expr *);
ir::Expr *diff(ir::Expr *, ir::Identifier *);
ir::Expr *diff(ir::Expr *, std::string);

#endif // ANALYSIS_H
