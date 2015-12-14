#ifndef DOT_H
#define DOT_H

#include <ostream>

///
/// Interface for classes to print their representation as dot graph
///
class DOT {
    public :
        virtual void dumpDOT(std::ostream&, bool) const = 0;
};


#endif // DOT_H
