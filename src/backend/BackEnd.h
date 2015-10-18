#ifndef BACKEND_H
#define BACKEND_H

#include "IR.h"

#include <iostream>

class BackEnd {
    public:
        virtual void emitCode(std::ostream &os) = 0;
};

#endif
