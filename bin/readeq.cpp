#include "config.h"
#include "IR.h"
#include "parser.hpp"
#include "Printer.h"
#include "FrontEnd.h"
#include "TopBackEnd.h"

#include <iostream>
#include <iomanip>
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
    fprintf(stderr, "Usage: %s <input>\n", bin);
}

void help(char *bin) {
    usage(bin);
    std::cerr << "OPTIONS:\n";
    std::cerr << std::left << std::setw(16) << "  -o filename" <<
        "\toutput file name\n";
    std::cerr << std::setw(16) << "  -f" <<
        "\tforce: override output file (if exists)\n";
    std::cerr << std::setw(16) << "  -h" <<
        "\thelp: display this help\n";
    std::cerr << std::setw(16) << "  -v level" <<
        "\tset verbosity level\n";
    std::cerr << std::setw(16) << "  -l filename" <<
        "\twrites equation into LaTeX format\n";
    std::cerr << std::setw(16) << "  -d n" <<
        "\tdimension (n should be 1 or 2)\n";
    std::cerr << std::setw(16) << "  -r filename" <<
        "\trenaming file for LaTeX output\n";
    std::cerr << std::setw(16) << "  -t derivative_type" <<
        "\tradial derivative type (CHEB or FD)\n";
}

int main(int argc, char* argv[]) {
    std::string *outFileName = NULL, *latexFileName = NULL;
    std::string renameFile("");
    char c;
    int nfile = 0;
    bool force = false, latex = false;
    int dim = 2;
    std::string derTypeOpt("not set");
    DerivativeType derType;

    logger::Printer::init();

    while ((c = getopt(argc, argv, "o:fhv:l:d:r:t:")) != EOF) {
        switch (c) {
        case 'h':
            help(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            force = true;
            break;
        case 'o':
            outFileName = new std::string(optarg);
            break;
        case 'v':
            logger::Printer::init(atoi(optarg));
            break;
        case 'l':
            latex = true;
            latexFileName = new std::string(optarg);
            break;
        case 'r':
            renameFile = std::string(optarg);
            break;
        case 'd':
            dim = atoi(optarg);
            if (dim != 1 && dim != 2) {
                help(argv[0]);
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            derTypeOpt = std::string(optarg);
            if (derTypeOpt == "CHEB") {
                derType = CHEB;
            }
            else if (derTypeOpt == "FD") {
                derType = FD;
            }
            else {
                logger::err << "unknown derivative type: `" << derTypeOpt << "'\n";
                exit(EXIT_FAILURE);
            }
            break;
        }
    }
    if (optind < argc) {
        while ((optind + nfile) < argc) {
            filename = new std::string(argv[optind + nfile]);
            yyin = fopen(filename->c_str(), "r");
            if (!yyin) {
                logger::err << "cannot open input file `" << *filename << "'\n";
                exit(EXIT_FAILURE);
            }
            nfile++;
        }
    }
    if (nfile != 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }


    if (!force && access(optarg, F_OK) != -1) {
        logger::err << "File `" << optarg << "' already exists\n";
        exit(EXIT_FAILURE);
    }

    // if (derTypeOpt == "not set") {
    //     logger::err << "derivative type not set\n";
    //     exit(EXIT_FAILURE);
    // }

    FrontEnd fe;
    ir::Program *p = fe.parse(*filename);
    TopBackEnd topBackEnd(p, derType, dim);
    FortranOutput *o;
    std::ofstream ofs;
    if (outFileName) {
        ofs.open(*outFileName);
        o = new FortranOutput(ofs);
    }
    else {
        o = new FortranOutput(std::cout);
    }

    if (latex) {
        LatexOutput *lo;
        std::ofstream lofs;
        lofs.open(*latexFileName);
        lo = new LatexOutput(lofs);
        topBackEnd.emitLaTeX(*lo, renameFile);
        lofs.close();
    }

    topBackEnd.emitCode(*o);

    delete o;

    fclose(yyin);
    delete filename;
    return 0;
}
