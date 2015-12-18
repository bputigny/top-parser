#ifndef DOT_H
#define DOT_H

#include <ostream>

///
/// Interface for classes to print their representation as dot graph
///
class DOT {
    private:
        static void displayFile(const std::string& file);
    public:
        virtual void dumpDOT(std::ostream&, std::string = "", bool = true) const = 0;
        void display(std::string = "");
};


#endif // DOT_H
