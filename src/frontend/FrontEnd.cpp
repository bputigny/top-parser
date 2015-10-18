#include "FrontEnd.h"
#include "Printer.h"

FrontEnd::FrontEnd() {}

ir::Program *FrontEnd::parse(std::string &filename) {
    yyin = fopen(filename.c_str(), "r");
    if (!yyin) {
        error("cannot open input file `%s'\n", filename.c_str());
    }
    yyparse();
    return prog;
}
