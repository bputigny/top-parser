#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "config.h"
#include "IR.h"

#include <functional>

template<class T>
class Analysis {
    protected:
        ir::Program *prog;

    public:
        inline Analysis(ir::Program *prog) {
            this->prog = prog;
        }

        inline void run(std::function<void (T *)> check, ir::Node *root) {
            for (auto c:root->getChildren()) {
                run(check, c);
            }
            if (T *n = dynamic_cast<T *>(root)) {
                check(n);
            }
        }
};

#endif // ANALYSIS_H
