#ifndef AUTOMANIPULATEFILE_CONFIGREADER_H
#define AUTOMANIPULATEFILE_CONFIGREADER_H
#include "yaml-cpp/yaml.h"
#include <string>

class ConfigReader{
    public:
        ConfigReader();
        void initiateAddFileMode();
        void initiateDeleteFileMode();
        void initiateModifyYAMLMode();
        bool nextComputer();
        int getPort();
        YAML::Node getIPs();
        YAML::Node getBaseDir();
        std::string getComputerName();
        std::string getUser();
        std::string getKeyPath();
        std::string getSubdirectory();
        std::string getAddFileTargetFiles();
        std::string getFileName();
        std::string getModifyNodePath();
        std::string getModifyNodeValue();

    private:
        YAML::Node setting, computers, targetMode;
        YAML::const_iterator currentComputer;
        bool isFileExists(const char* name);
};
#endif //AUTOMANIPULATEFILE_CONFIGREADER_H
