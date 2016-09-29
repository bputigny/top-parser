#ifndef BACKEND_H
#define BACKEND_H

#include "config.h"
#include "IR.h"

#include <iostream>

class Output {
    std::ostream& os;
    int lineLen;

    void checkLineLen();

    public:
        Output(std::ostream&);

        Output& operator<<(const char *);
        Output& operator<<(const char);
        Output& operator<<(std::string);
        Output& operator<<(float);
        Output& operator<<(int);
        Output& operator<<(unsigned long);

        ~Output();
};

class BackEnd {
    public:
        virtual void emitCode(Output& of) = 0;
};

#endif
