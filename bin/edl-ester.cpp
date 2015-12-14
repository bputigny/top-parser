#include "config.h"
#include "IR.h"
#include "parser.hpp"
#include "Printer.h"
#include "FrontEnd.h"
#include "EsterBackEnd.h"

#include <iostream>
#include <fstream>
#include <string>
#include <list>

extern "C" {
#include <unistd.h>
}


extern int yylineno;
extern int yylex();
extern int yyerror(const char *s);

extern FILE *yyin;

void usage(char *bin) {
    fprintf(stderr, "Usage: %s <input> [-o <output] [-f]\n", bin);
}

void help(char *bin) {
    usage(bin);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o output_file %*s output file name", 8, "");
    fprintf(stderr, " (default: standard output)\n");
    fprintf(stderr, "  -f %*s force: override output file (if exists)\n",
            20, "");
    fprintf(stderr, "  -h %*s help: display this help\n", 20, "");
}

int main (int argc, char* argv[]) {
    std::string *outFileName = NULL;
    char c;
    int nfile = 0;
    int force = 0;

    while ((c = getopt(argc, argv, "o:fh")) != EOF) {
        switch (c) {
        case 'h':
            help(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            force = 1;
            break;
        case 'o':
            if (!force && access(optarg, F_OK) != -1 ) {
                err() << "File `" << optarg << "' already exists\n";
            }
            outFileName = new std::string(optarg);
            break;
        }
    }
    if (optind < argc) {
        while ((optind + nfile) < argc) {
            filename = new std::string(argv[optind + nfile]);
            yyin = fopen(filename->c_str(), "r");
            if (!yyin) {
                err() << "cannot open input file `" << filename << "'\n";
            }
            nfile++;
        }
    }
    if (nfile != 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    FrontEnd fe;
    ir::Program *p = fe.parse(*filename);
    EsterBackEnd esterBackEnd(*p);
    if (outFileName) {
        std::ofstream ofs;
        ofs.open(*outFileName);
        esterBackEnd.emitCode(ofs);
        ofs.close();
    }
    else {
        esterBackEnd.emitCode(std::cout);
    }

    fclose(yyin);
    delete filename;
    return 0;
}
