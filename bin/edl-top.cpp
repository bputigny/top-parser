#include "config.h"
#include "IR.h"
#include "parser.hpp"
#include "Printer.h"
#include "FrontEnd.h"
#include "TopBackEnd.h"

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
    fprintf(stderr, "Usage: %s <input> [-o <output] [-f] [-v level]\n", bin);
}

void help(char *bin) {
    usage(bin);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  -o output_file %*s output file name", 8, "");
    fprintf(stderr, " (default: standard output)\n");
    fprintf(stderr, "  -f %*s force: override output file (if exists)\n",
            20, "");
    fprintf(stderr, "  -h %*s help: display this help\n", 20, "");
    fprintf(stderr, "  -v level %*s set verbosity level ", 14, "");
    fprintf(stderr, "\n");
}

int main(int argc, char* argv[]) {
    std::string *outFileName = NULL;
    char c;
    int nfile = 0;
    int force = 0;

    while ((c = getopt(argc, argv, "o:fhv:")) != EOF) {
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
                warn() << "File `" << optarg << "' already exists\n";
            }
            outFileName = new std::string(optarg);
            break;
        case 'v':
            Printer::init(atoi(optarg));
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
    TopBackEnd topBackEnd(p);
    Output *o;
    std::ofstream ofs;
    if (outFileName) {
        ofs.open(*outFileName);
        o = new Output(ofs);
    }
    else {
        o = new Output(std::cout);
    }
    topBackEnd.emitCode(*o);

    delete o;

    fclose(yyin);
    delete filename;
    return 0;
}
