#ifndef PRINTER_H
#define PRINTER_H

#include <string>
#include <cstdlib>
#include <cstdio>

extern "C" {
#include <stdarg.h>
}

extern std::string *filename;
extern int yylineno;

class Printer {
    std::string prefix;
    FILE *file;
    bool exit;
public:
    Printer(const std::string pref, FILE *f, bool exit = false);
    void operator()(const char *str, ...);
};

extern Printer info;
extern Printer debug;
extern Printer error;
extern Printer semantic_error;

#endif // PRINTER_H
