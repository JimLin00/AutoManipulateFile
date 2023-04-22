//
// Created by Jim on 2023/4/18.
//

#include "YamlCommentSaver.h"
#include <fstream>

using namespace std;

YamlCommentSaver::YamlCommentSaver(const fs::path &file) {
    this -> fileName = file;
    steam.open(file.u8string(),ios::in);
    Read();
}

YamlCommentSaver::~YamlCommentSaver() {
    clear(commentsHead);
    steam.close();
    fs::remove(fileName);
}

void YamlCommentSaver::Read() {
    GetNextChar();
    while(currentChar != '\0'){
        SkipSpaceAndTab();

        if(!status.afterSpecial){
            if(currentChar == '#' && status.isComment) SaveComment();
            else if(currentChar == '\n' ) SkipNewLine();
            else if(IsSpecial(currentChar)) SkipSpecial();
            else SaveBeforeSpecialString();
        }
        else{
            if(currentChar == '#' && status.isComment) SaveComment();
            else if(currentChar == '\n' ) SkipNewLine();
            else if(currentChar == '\'') SkipSingleQuoteString();
            else if(currentChar == '"') SkipMultiQuoteString();
            else SkipAfterSpecialString();
        }
    }
}

void YamlCommentSaver::SkipSingleQuoteString() {
    status.hasTextInLine = true;
    lineBuffer.clear();

    GetNextChar();
    while(this->currentChar != '\''){
        GetNextChar();
    }

    status.hasTextInLine = true;
    GetNextChar();
}

void YamlCommentSaver::SkipMultiQuoteString() {
    status.hasTextInLine = true;
    lineBuffer.clear();

    GetNextChar();
    while(this->currentChar != '"'){
        GetNextChar();
    }

    status.hasTextInLine = true;
    GetNextChar();
}

void YamlCommentSaver::SkipAfterSpecialString() {
    status.hasTextInLine = true;
    lineBuffer.clear();

    while(!IsWhiteSpace(this->currentChar)){
        if(!GetNextChar()) break;
    }
}

void YamlCommentSaver::SkipSpaceAndTab() {
    while(currentChar == ' ' || currentChar == '\t'){
        lineBuffer += currentChar;
        GetNextChar();
    }
}

void YamlCommentSaver::SaveBeforeSpecialString() {
    status.hasTextInLine = true;
    lineBuffer.clear();

    std::string nodeBuffer;
    int indent = this -> status.column - 1;

    while(currentChar != ':'){
        if(!IsWhiteSpace(currentChar)){
            nodeBuffer += currentChar;
        }
        GetNextChar();
    }

    if(status.baseIndent != -1 && indent > status.baseIndent ) return;

    status.line = 0;
    status.baseIndent = indent;
    status.nodeName = nodeBuffer;

}

void YamlCommentSaver::SkipSpecial() {
    status.hasTextInLine = true;
    lineBuffer.clear();

    GetNextChar();
    status.afterSpecial = true;
}

void YamlCommentSaver::AppendNewNode(CommentSaverPtr &newNode) {
    if(commentsHead == nullptr){
        commentsHead = newNode;
        commentsTail = newNode;
    }
    else{
        commentsTail -> next = newNode;
        commentsTail = newNode;
    }
}

void YamlCommentSaver::SaveComment() {
    while(currentChar != '\n' ){
        lineBuffer += currentChar;
        if(!GetNextChar()) break;
    }

    CommentSaverPtr newNode = CreateCommentNewNode();

    AppendNewNode(newNode);

    status.hasTextInLine = true;
}

void YamlCommentSaver::SkipNewLine() {
    if(!status.hasTextInLine){
        CommentSaverPtr newNode = CreateCommentNewNode();
        AppendNewNode(newNode);
    }

    GetNextChar();
}

void YamlCommentSaver::clear(CommentSaverPtr &head) {
    if(head == nullptr) return;
    clear(head -> next);
    delete head;
}


bool YamlCommentSaver::GetNextChar() {

    this -> status.isComment = IsWhiteSpace(this->currentChar) || IsStringQuote(this->currentChar) || currentChar == 0;

    if(this->currentChar == '\n'){
        status.line++;
        status.column = 1;
        status.afterSpecial = false;
        status.hasTextInLine = false;
        lineBuffer.clear();

    }
    else{
        status.column++;
    }

    steam.get(this->currentChar)   ;

    if(this -> steam.eof()) {
        currentChar = '\0';
        return false;
    }

    return true;
}

bool YamlCommentSaver::IsWhiteSpace(char chr) {
    return chr == ' ' || chr == '\t' || chr == '\n';
}

bool YamlCommentSaver::IsStringQuote(char chr) {
    return chr == '\'' || chr == '"';
}

bool YamlCommentSaver::IsSpecial(char chr) {
    return chr == ':' || chr == '-' ;
}

CommentSaverPtr YamlCommentSaver::CreateCommentNewNode() {
    auto res = new CommentSaver;

    res -> line = status.line;
    res -> comment = lineBuffer;
    res -> relativeNode = status.nodeName;
    res -> isBehindText = status.hasTextInLine;
    res -> next = nullptr;


    if(status.baseIndent == -1){
        res -> type = INSTANT_APPEND;
    }
    else if(commentsTail != nullptr && commentsTail -> comment == status.nodeName && commentsTail -> line == status.line -1){
        res -> type = INSTANT_APPEND;
    }
    else{
        res -> type = CommentType::RELATIVE;
    }

    return res;
}

CommentSaverPtr YamlCommentSaver::getComments() {
    return commentsHead;
}

void YamlCommentSaver::print() {


    CommentSaverPtr  temp = commentsHead;

    while (temp != nullptr){
        printf("Node: %s, Comment: %s, Line: %d, Type: %d, Behind: %s\n", temp ->relativeNode.c_str(),temp->comment.c_str()
                ,temp->line,temp->type,temp->isBehindText ? "True":"False" );

        temp = temp ->next;
    }


}

void YamlCommentSaver::out() {
    fstream outFile("temp/test.yml",ios::out|ios::trunc);
    CommentSaverPtr temp = this ->commentsHead;
    while(temp != nullptr){

        outFile.write(temp->comment.c_str(),temp->comment.size());


        outFile.write("\n",1);
        temp = temp -> next;
    }
}

fs::path YamlCommentSaver::getFileName() {
    return this -> fileName;
}























