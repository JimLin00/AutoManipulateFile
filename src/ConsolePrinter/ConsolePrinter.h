//
// Created by Jim on 2023/4/15.
//

#ifndef AUTOMANIPULATEFILE_CONSOLEPRINTER_H
#define AUTOMANIPULATEFILE_CONSOLEPRINTER_H
#include <string>

enum ConsoleColor{
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

namespace Console{
    void cusGoto(short x,short y);
    void setColor(int color);
    void printError(std::string info);
    void printError(const char * info);
}

#endif //AUTOMANIPULATEFILE_CONSOLEPRINTER_H
