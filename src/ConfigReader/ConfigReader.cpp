//
// Created by Jim on 2023/4/15.
//
#include "ConfigReader.h"
#include "yaml-cpp/yaml.h"
#include "../ConsolePrinter/ConsolePrinter.h"
#include <string>
#include <filesystem>
#include <cstdlib>

ConfigReader::ConfigReader() {
    if(!isFileExists("setting.yml")){
        Console::printError("File setting.yml not found!");
        exit(-1);
    }

    this -> setting = YAML::LoadFile("setting.yml");

    this -> computers = YAML::LoadFile(setting["ComputerConfig"].as<std::string>());
    this -> currentComputer = computers.begin();
    this -> targetMode = NULL;
}


bool ConfigReader::nextComputer() {
    this -> currentComputer ++;

    return this -> currentComputer != this -> computers.end();
}

YAML::Node ConfigReader::getIPs() {
    return this -> currentComputer -> second["ips"];
}

YAML::Node ConfigReader::getBaseDir() {
    return this -> currentComputer -> second["BaseDir"];
}

int ConfigReader::getPort() {
    return this -> currentComputer -> second["port"].as<int>();
}

std::string ConfigReader::getUser() {
    return this -> currentComputer -> second["user"].as<std::string>();
}

std::string ConfigReader::getKeyPath() {
    return this -> currentComputer -> second["key"].as<std::string>();
}

std::string ConfigReader::getComputerName() {
    return this -> currentComputer -> first.as<std::string>();
}




bool ConfigReader::isFileExists(const char* name){
    return std::filesystem::exists(name);
}

void ConfigReader::initiateAddFileMode() {
    std::string fileName = this->setting["AddFileConfig"].as<std::string>();
    if(!isFileExists(fileName.c_str())){
        Console::printError("File " + fileName +" not exists!");
        exit(-1);
    }

    this -> targetMode = YAML::LoadFile(fileName);
}

void ConfigReader::initiateDeleteFileMode() {
    std::string fileName = this->setting["DeleteFileConfig"].as<std::string>();
    if(!isFileExists(fileName.c_str())){
        Console::printError("File " + fileName +" not exists!");
        exit(-1);
    }

    this -> targetMode = YAML::LoadFile(fileName);
}

void ConfigReader::initiateModifyYAMLMode() {
    std::string fileName = this->setting["ModifyYAMLConfig"].as<std::string>();
    if(!isFileExists(fileName.c_str())){
        Console::printError("File " + fileName +" not exists!");
        exit(-1);
    }

    this -> targetMode = YAML::LoadFile(fileName);
}

std::string ConfigReader::getSubdirectory() {
    return this->targetMode["subDir"].as<std::string>();
}

std::string ConfigReader::getAddFileTargetFiles() {
    return this -> targetMode["targetFiles"].as<std::string>();
}

std::string ConfigReader::getFileName() {
    return this -> targetMode["fileName"].as<std::string>();;
}

std::string ConfigReader::getModifyNodePath() {
    return this -> targetMode["nodePath"].as<std::string>();
}

std::string ConfigReader::getModifyNodeValue() {
    return this -> targetMode["nodeValue"].as<std::string>();;
}










