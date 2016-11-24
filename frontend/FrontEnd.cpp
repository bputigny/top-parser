#include "FrontEnd.h"
#include "Printer.h"

std::string *filename = NULL;

FrontEnd::FrontEnd() {}

ir::Program *FrontEnd::parse(char *file) {
    std::string f = std::string(file);
    return parse(f);
}

ir::Program *FrontEnd::parse(const std::string& file) {
    std::string f = std::string(file);
    return parse(f);
}

ir::Program *FrontEnd::parse(std::string &file) {
    filename = &file;
    yyin = fopen(file.c_str(), "r");
    if (!yyin) {
        logger::err << "cannot open input file `" << file << "'\n";
    }
    yyparse();
    return prog;
}
