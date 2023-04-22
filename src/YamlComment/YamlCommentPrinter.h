//
// Created by Jim on 2023/4/21.
//

#ifndef AUTOMANIPULATEFILE_YAMLCOMMENTPRINTER_H
#define AUTOMANIPULATEFILE_YAMLCOMMENTPRINTER_H

#include "YamlCommentSaver.h"
#include <filesystem>

namespace fs = std::filesystem;

class YamlCommentPrinter {
    public:
        explicit YamlCommentPrinter(YamlCommentSaver &saver);
        ~YamlCommentPrinter();

        void AddCommentToFile();
        fs::path getFilePath();


    private:
        char currentChar = '\0';
        NodeStatus status{"", true, false, false,0,0,-1};
        std::fstream steamOut, steamIn;
        CommentSaverPtr commentsHead = nullptr;
        fs::path fileName;

        static bool IsWhiteSpace(char chr);
        static bool IsSpecial(char chr);


        bool GetNextChar();
        bool TempPrintComment();
        bool CanPrintCurrentComment();
        void SkipSpaceAndTab();
        void SaveBeforeSpecialString();
        void SkipSpecial();
        void SkipNewLine();
        void SkipAfterSpecial();
        void PrintComment();


};


#endif //AUTOMANIPULATEFILE_YAMLCOMMENTPRINTER_H
