//
// Created by Jim on 2023/4/21.
//

#include "YamlCommentPrinter.h"
#include <fstream>

using namespace std;
YamlCommentPrinter::YamlCommentPrinter(YamlCommentSaver &saver) {
    fs::path cleanName = saver.getFileName().stem();
    cleanName += "_replace";
    cleanName += saver.getFileName().extension();
    this -> fileName = saver.getFileName().parent_path().u8string()+"/"+cleanName.u8string();
    this -> commentsHead = saver.getComments();
    this -> steamIn.open(saver.getFileName(),ios::in);
    this -> steamOut.open(fileName,ios::out|ios::trunc);
}

YamlCommentPrinter::~YamlCommentPrinter() {
    steamIn.close();
    steamOut.close();

    fs::remove(fileName);
}


void YamlCommentPrinter::AddCommentToFile() {
    GetNextChar();

    while(!steamIn.eof()){
        while(TempPrintComment()) ;

        SkipSpaceAndTab();
        if(!status.afterSpecial){
            if(IsSpecial(currentChar)) SkipSpecial();
            else SaveBeforeSpecialString();
        }
        else{
            if(currentChar == '\n') SkipNewLine();
            else SkipAfterSpecial();
        }
    }

    while (commentsHead != nullptr){
        if(commentsHead->line>status.line){
            steamOut.write("\n",1);
            status.line++;
        }
        else{
            PrintComment();
        }
    }

    steamOut.flush();
}

bool YamlCommentPrinter::GetNextChar() {



    if(this->currentChar == '\n'){
        status.line++;
        status.column = 1;
        status.afterSpecial = false;
        this -> status.hasTextInLine = false;
    }
    else{
        status.column++;
    }

    steamIn.get(this->currentChar);

    if(this -> steamIn.eof()) {
        currentChar = '\0';
        return false;
    }

    return true;
}


void YamlCommentPrinter::SkipSpaceAndTab() {
    while(currentChar == ' ' || currentChar == '\t'){
        this -> steamOut.write(&currentChar,1);
        GetNextChar();
    }
}

void YamlCommentPrinter::SkipSpecial() {

    this -> steamOut.write(&currentChar,1);
    GetNextChar();
    status.afterSpecial = true;
}

void YamlCommentPrinter::SkipAfterSpecial() {

    while(currentChar != '\n'){
        this -> steamOut.write(&currentChar,1);
        if(!GetNextChar()) break;
    }
}

void YamlCommentPrinter::SaveBeforeSpecialString() {
    std::string nodeBuffer;
    int indent = this -> status.column - 1;

    while(currentChar != ':'){
        if(!IsWhiteSpace(currentChar)){
            nodeBuffer += currentChar;
        }

        this -> steamOut.write(&currentChar,1);
        GetNextChar();
    }

    if(status.baseIndent != -1 && indent > status.baseIndent ) return;

    status.line = 0;
    status.baseIndent = indent;
    status.nodeName = nodeBuffer;

}

void YamlCommentPrinter::PrintComment() {
    steamOut.write(commentsHead->comment.c_str(),commentsHead->comment.size());
    if(!commentsHead -> isBehindText){
        status.line++;
    }

    commentsHead = commentsHead->next;

    if(!steamIn.eof() || commentsHead != nullptr){
        steamOut.write("\n",1);
    }
}

void YamlCommentPrinter::SkipNewLine() {
    this -> status.hasTextInLine = true;

    if(TempPrintComment()) {
        GetNextChar();
        return;
    }

    steamOut.write("\n",1);
    GetNextChar();
}

bool YamlCommentPrinter::IsSpecial(char chr) {
    return chr == ':' || chr == '-' ;
}

bool YamlCommentPrinter::IsWhiteSpace(char chr) {
    return chr == ' ' || chr == '\t' || chr == '\n';
}

bool YamlCommentPrinter::CanPrintCurrentComment() {
    if(commentsHead == nullptr)return false;
    if(commentsHead->isBehindText != status.hasTextInLine) return false;
    if(commentsHead->type == INSTANT_APPEND)return true;
    if(commentsHead->relativeNode == status.nodeName && commentsHead->line == status.line) return true;
    return false;
}

bool YamlCommentPrinter::TempPrintComment() {
    if(CanPrintCurrentComment()){
        PrintComment();
        return true;
    }

    return false;
}

fs::path YamlCommentPrinter::getFilePath() {
    return this ->fileName;
}












