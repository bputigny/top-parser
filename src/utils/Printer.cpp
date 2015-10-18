#include "Printer.h"
#include "config.h"

Printer::Printer(const std::string pref, FILE *f, bool exit) {
    this->prefix = pref;
    this->file = f;
    this->exit= exit;
}

void Printer::operator()(const char *str, ...)  {
    fprintf(this->file, "[%s] ", prefix.c_str());
    va_list args; va_start(args, str);
    vfprintf(this->file, str, args);
    va_end (args);
    if (this->exit)
        std::exit(EXIT_FAILURE);
}

Printer debug("debug", stderr);
Printer info("info", stderr);
Printer error("error", stderr, true);
Printer semantic_error("semantic error", stderr, true);

// #undef pack_args
