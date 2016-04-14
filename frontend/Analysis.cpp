#include "Analysis.h"

#include <cassert>

std::vector<ir::Identifier *> getIds(ir::Expr *e) {
    std::vector<ir::Identifier *> ret;
    Analysis<ir::Identifier> a;
    auto addIdentifiers = [&ret] (ir::Identifier *id) {
        bool add = true;
        // check if id is already in ret
        for (auto i: ret) {
            if (i->getName() == id->getName())
                add = false;
        }
        if (add)
            ret.push_back(id);
    };
    a.run(addIdentifiers, e);
    return ret;
}

