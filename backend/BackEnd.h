#ifndef BACKEND_H
#define BACKEND_H

#include "config.h"
#include "IR.h"

#include <iostream>
#include <cstring>

class Output {
    protected:
        std::ostream& os;

    public:
        Output(std::ostream&);

        template <typename T>
        inline Output& operator<<(const T& t) {
            os << t;
            return (*this);;
        }

        ~Output();
};

class FortranOutput : public Output {
    protected:
        int lineLen;

        void checkLineLen();

    public:
        FortranOutput(std::ostream&);

        inline FortranOutput& operator<<(const std::string& str) {
            checkLineLen();
            lineLen += str.length();
            if (str[str.length()-1] == '\n')
                lineLen = 0;
            os << str;
            return (*this);
        }
        inline FortranOutput& operator<<(const char *str) {
            checkLineLen();
            lineLen += std::strlen(str);
            if (str[std::strlen(str)-1] == '\n')
                lineLen = 0;
            os << str;
            return (*this);
        }
        template <typename T>
        inline FortranOutput& operator<<(const T& t) {
            checkLineLen();
            lineLen += std::to_string(t).length();
            os << t;
            return (*this);
        }
};

class LatexOutput : Output {
    public:
        LatexOutput(std::ostream&);

        template <typename T>
        inline Output& operator<<(const T& t) {
            os << t;
            return (*this);;
        }
};

class BackEnd {
};

#endif
