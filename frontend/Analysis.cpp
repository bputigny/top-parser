#include "Analysis.h"

#include <cassert>

std::vector<ir::Identifier *> getIds(ir::Expr *e, bool uniq) {
    std::vector<ir::Identifier *> ret;
    Analysis<ir::Identifier> a;
    auto addIdentifiers = [&ret, &uniq] (ir::Identifier *id) {
        bool add = true;
        // check if id is already in ret
        if (uniq) {
            for (auto i: ret) {
                if (i->name == id->name)
                    add = false;
            }
        }
        if (add)
            ret.push_back(id);
    };
    a.run(addIdentifiers, e);
    return ret;
}
