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
        static int verbosity;
        static void init(int verbosity = 1);

        Printer(const std::string&, std::ostream&, int level);
        ~Printer();
        std::ostream& stream();

        std::ostream& operator<<(std::string&);
        std::ostream& operator<<(const char *);
};

extern Printer err, log, warn;

#endif // PRINTER_H
