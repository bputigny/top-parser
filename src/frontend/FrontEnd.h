#ifndef FRONTEND_H
#define FRONTEND_H

#include "IR.h"
extern FILE *yyin;
extern int yyparse();
extern ir::Program *prog;

class FrontEnd {
    public:
        FrontEnd();
        ir::Program *parse(std::string &filename);
};

#endif
