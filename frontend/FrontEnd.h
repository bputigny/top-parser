#ifndef FRONTEND_H
#define FRONTEND_H

#include "config.h"
#include "IR.h"
extern FILE *yyin;
extern int yyparse();
extern ir::Program *prog;

class FrontEnd {
    public:
        FrontEnd();
        ir::Program *parse(char *filename);
        ir::Program *parse(std::string& filename);
        ir::Program *parse(const std::string& filename);
};

#endif
