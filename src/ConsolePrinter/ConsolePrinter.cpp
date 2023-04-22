#include <windows.h>
#include <string>
#include <iostream>
#include "ConsolePrinter.h"

using namespace std;

void Console::cusGoto(short x,short y){
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordination;
    coordination.X = x;
    coordination.Y = y;

    SetConsoleCursorPosition(handle,coordination);
}

void Console::setColor(int color){
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    WORD word = color;

    SetConsoleTextAttribute(handle,word);
}

void Console::printError(std::string info) {
    Console::setColor(RED);
    cout << info << endl;
}

void Console::printError(const char * info) {
    Console::setColor(RED);
    cout << info << endl;
    Console::setColor(WHITE);
}