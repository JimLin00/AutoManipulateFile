//
// Created by Jim on 2023/4/18.
//

#ifndef AUTOMANIPULATEFILE_YAMLCOMMENTSAVER_H
#define AUTOMANIPULATEFILE_YAMLCOMMENTSAVER_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

enum CommentType{
    RELATIVE,
    FILE_HEAD,
    INSTANT_APPEND
};

struct CommentSaver{
    bool isBehindText;
    int line;
    CommentType type;
    std::string relativeNode;
    std::string comment;
    CommentSaver *next;
};

struct NodeStatus{
    std::string nodeName;
    bool isComment;
    bool hasTextInLine;
    bool afterSpecial;
    int line;
    int column;
    int baseIndent;
};

typedef CommentSaver * CommentSaverPtr;

class YamlCommentSaver {


    public:
        explicit YamlCommentSaver(const fs::path &file);
        ~YamlCommentSaver();

        fs::path getFileName();
        CommentSaverPtr getComments();

        void clear(CommentSaverPtr &head);
        void print();
        void out();
    private:
        char currentChar = '\0';
        fs::path fileName;
        std::string lineBuffer;
        NodeStatus status{"", true, false, false,0,0,-1};
        CommentSaverPtr commentsHead = nullptr;
        CommentSaverPtr commentsTail = nullptr;
        std::fstream steam;

        static bool IsWhiteSpace(char chr);
        static bool IsStringQuote(char chr);
        static bool IsSpecial(char chr);
        bool GetNextChar();

        void SkipSingleQuoteString();
        void SkipMultiQuoteString();
        void SkipAfterSpecialString();
        void SkipSpaceAndTab();
        void SkipSpecial();
        void SkipNewLine();
        void AppendNewNode(CommentSaverPtr &newNode);
        void SaveBeforeSpecialString();
        void SaveComment();
        void Read();

        CommentSaverPtr CreateCommentNewNode();
};


#endif //AUTOMANIPULATEFILE_YAMLCOMMENTSAVER_H
