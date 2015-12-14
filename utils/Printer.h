#ifndef PRINTER_H
#define PRINTER_H

#include "config.h"
#include <string>
#include <iostream>

extern std::string *filename;
extern int yylineno;

class Printer {
    std::string pref;
    std::ostream &os;
    public:
        Printer(const std::string&, std::ostream&);
        std::ostream& stream();
        void print();
};

std::ostream& err();
std::ostream& warn();

#endif // PRINTER_H
