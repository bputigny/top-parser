#ifndef PRINTER_H
#define PRINTER_H

#include "config.h"
#include <string>
#include <iostream>
#include <fstream>

extern std::string *filename;
extern int yylineno;

class Printer {
    std::string pref;
    std::ostream &os;
    std::ofstream devnull;
    int level;

    public:
        Printer(const std::string&, std::ostream&, int level);
        static int verbosity;
        ~Printer();
        static void init(int verbosity = 0);
        std::ostream& stream();
        void print();
};

std::ostream& err();
std::ostream& log();
std::ostream& warn();

#endif // PRINTER_H
