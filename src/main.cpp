
#include "ConfigReader/ConfigReader.h"
#include "ConsolePrinter/ConsolePrinter.h"
#include "YamlComment/YamlCommentSaver.h"
#include "YamlComment/YamlCommentPrinter.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <fcntl.h>

using namespace std;
namespace fs = filesystem;

typedef char *CharPtr;

struct ServerProperties{
    int port;
    int verbose;
    CharPtr ip;
    CharPtr user;
};

typedef ServerProperties * PropertiesPtr;

ssh_session GetServerSession(PropertiesPtr container);
ssh_session ConnectToServer(ConfigReader reader, PropertiesPtr container);
sftp_session GetSFTPSession(ssh_session sshSession);
ssh_channel GetShellSession(ssh_session sshSession);
sftp_dir GetSFTPDir(ssh_session sshSession, sftp_session sftp, const fs::path &serverPath);


bool AuthorizeSession(ConfigReader reader,ssh_session sshSession);

int GetUserMode();

PropertiesPtr CopyToContainer(ConfigReader reader);\
YAML::Node GetDeepNode(YAML::Node &node,vector<string>::iterator current,vector<string> &list);

void PrintInfo();
void PrintComputerConnecting(const string& com);
void PrintFailedToConnect();
void PrintResembleFile(sftp_session sftp, sftp_dir dir,const string &fileName);
void FreeContainer(PropertiesPtr container);
void AddFileToServer(ConfigReader reader,ssh_session sshSession);
void WriteFileToTargetDir(const ssh_session &sshSession,const fs::path &directoryPath ,sftp_session sftp, const fs::path &serverPath);
void DeleteFileFromServer(ConfigReader reader,ssh_session sshSession);
void DeleteServerFile(ConfigReader reader,sftp_dir dir,sftp_session sftp,ssh_session sshSession,const fs::path &serverPath);
void ModifyYAMLFile(ConfigReader reader,ssh_session sshSession);
void ModifyYaml(ConfigReader reader,const fs::path &file);
void SplitString(vector<string> &list,const string &context,char split);
bool CopyFileFromServer(ssh_session sshSession, sftp_session sftp, const fs::path &from, const fs::path &to);
bool HasSameFileExtension(const fs::path &file1, const fs::path &file2);
bool IsResembleFile(const fs::path &file1, const fs::path &file2);
bool CopyFileToRemote(const fs::path &from, sftp_file &to);
bool IsFileExists(sftp_session sftp,sftp_dir dir,const string &fileName);

bool CreateInteractiveSession(ssh_channel channel){
    int rc = ssh_channel_request_pty(channel);
    if (rc != SSH_OK) {
        printf("ERR1\n");
        return false;
    }
    rc = ssh_channel_change_pty_size(channel, 80, 24);
    if (rc != SSH_OK){
        printf("ERR2\n");
        return false;
    }
    rc = ssh_channel_request_shell(channel);
    if (rc != SSH_OK) {
        printf("ERR3\n");
        return false;
    }

    return true;
}



void Test(){
    fs::path path("temp/settings.yml");
    YamlCommentSaver saver(path);

    YamlCommentPrinter printer(saver);
    printer.AddCommentToFile();
}

int main() {

    while(1){
        PrintInfo();

        int mode;
        while(true){
            mode = GetUserMode();
            printf("\n");

            if(mode <=4 && mode >= 1) break;
            printf("\u8acb\u8f38\u5165\u6709\u6548\u7684\u6578\u5b57\u865f: ");
        }

        if(mode == 4) {
            break;
        }

        ConfigReader reader;

        do{

            PrintComputerConnecting(reader.getComputerName());

            PropertiesPtr container = CopyToContainer(reader);
            ssh_session session = ConnectToServer(reader, container);
            if(session == nullptr) continue;

            if(!AuthorizeSession(reader,session)){
                ssh_disconnect(session);
                ssh_free(session);
                FreeContainer(container);
                continue;
            }

            Console::setColor(GREEN);
            cout << "Success!\n";
            Console::setColor(WHITE);

            switch (mode) {
                case 1:
                    cout << "Adding file to server - ";
                    AddFileToServer(reader,session);
                    break;
                case 2:
                    cout << "Delete file from server - ";
                    DeleteFileFromServer(reader,session);
                    break;
                default:
                    cout << "Modify file from server - ";
                    ModifyYAMLFile(reader,session);
            }


            ssh_disconnect(session);
            ssh_free(session);
            FreeContainer(container);

            break;
        } while (reader.nextComputer());

        cout << "-------------------------"<<endl;
    }



    system("pause");
}

ssh_session ConnectToServer(ConfigReader reader, PropertiesPtr container){
    YAML::Node ipNode = reader.getIPs();

    bool canConnect = false;
    ssh_session newSession;


    for(int i = 0 ; i < ipNode.size() && !canConnect; i++){
        strcpy(container->ip, ipNode[i].as<string>().c_str());

        newSession = GetServerSession(container);
        int rc = ssh_connect(newSession);

        if(rc != SSH_OK){
            ssh_free(newSession);
            continue;
        }

        canConnect = true;
    }

    if(!canConnect){
        PrintFailedToConnect();
        fprintf(stderr, "Error while  connection: %s\n",
                ssh_get_error(newSession));
        return nullptr;
    }



    return newSession;
}

ssh_session GetServerSession(PropertiesPtr container){
    ssh_session newSession = ssh_new();

    if(newSession == nullptr){
        PrintFailedToConnect();
        fprintf(stderr, "Error create new session: %s\n",
                ssh_get_error(newSession));
        exit(-1);
    }

    ssh_options_set(newSession, SSH_OPTIONS_HOST, container->ip);
    ssh_options_set(newSession, SSH_OPTIONS_LOG_VERBOSITY, &container->verbose);
    ssh_options_set(newSession, SSH_OPTIONS_PORT, &container->port);
    ssh_options_set(newSession, SSH_OPTIONS_USER, container->user);

    return newSession;
}

sftp_session GetSFTPSession(ssh_session sshSession){
    sftp_session sftp = sftp_new(sshSession);
    if (sftp == nullptr) {
        PrintFailedToConnect();
        fprintf(stderr, "Error allocating SFTP session: %s\n",
                ssh_get_error(sshSession));
        return nullptr;
    }

    int rc = sftp_init(sftp);
    if (rc != SSH_OK) {
        PrintFailedToConnect();
        fprintf(stderr, "Error initializing SFTP session: code %d.\n",
                sftp_get_error(sftp));
        sftp_free(sftp);
        return nullptr;
    }

    return sftp;
}

ssh_channel GetShellSession(ssh_session sshSession){
    ssh_channel channel = ssh_channel_new(sshSession);;
    int rc;

    if (channel == nullptr){
        PrintFailedToConnect();
        cout << "Error while create a new channel\n";
        return nullptr;
    }


    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        PrintFailedToConnect();
        cout << "Error while open a channel session\n";
        ssh_channel_free(channel);
        return nullptr;
    }


    return channel;
}

sftp_dir GetSFTPDir(ssh_session sshSession, sftp_session sftp, const fs::path &serverPath){
    sftp_dir dir;

    dir = sftp_opendir(sftp, serverPath.u8string().c_str());
    if (!dir) {
        PrintFailedToConnect();
        fprintf(stderr, "Directory not opened: %s\n",
                ssh_get_error(sshSession));
        return nullptr;
    }

    return dir;
}

YAML::Node GetDeepNode(YAML::Node &node,vector<string>::iterator current,vector<string> &list){
    if(current == list.end()){
        return node;
    }

    YAML::Node nextNode = node[*current];
    current++;

    return GetDeepNode(nextNode,current,list);
}

bool AuthorizeSession(ConfigReader reader,ssh_session sshSession){
    ssh_key privateKey ;
    int rc = ssh_pki_import_privkey_file(reader.getKeyPath().c_str(),nullptr,nullptr,nullptr,&privateKey);
    if(rc != SSH_OK){
        PrintFailedToConnect();
        fprintf(stderr, "Error while getting private key: %s\n",
                ssh_get_error(sshSession));
        return false;
    }

    rc = ssh_userauth_publickey(sshSession,nullptr,privateKey);
    ssh_key_free(privateKey);

    if(rc == SSH_AUTH_ERROR){
        PrintFailedToConnect();
        fprintf(stderr, "Error while authentication private key: %s\n",
                ssh_get_error(sshSession));
        return false;
    }


    return true;
}

int GetUserMode(){
    string in;
    cin >> in;


    bool isNum = true;

    for( string::const_iterator itr = in.cbegin(); itr  != in.end() && isNum ; itr++){
        if(!(*itr >= '0' && *itr <= '9')) isNum = false;
    }

    if(!isNum) return -1;

    return stoi(in);
}

void PrintInfo(){
    Console::setColor(LIGHTGRAY);
    printf("%-6s%s\n","\u6a21\u5f0f","\u6558\u8ff0");
    Console::setColor(YELLOW);
    printf("%-6s","1.");
    Console::setColor(WHITE);
    printf("%s\n","\u6dfb\u52a0\u6a94\u6848\u81f3\u4f3a\u670d\u5668");
    Console::setColor(YELLOW);
    printf("%-6s","2.");
    Console::setColor(WHITE);
    printf("%s\n","\u522a\u9664\u6a94\u6848");

    Console::setColor(YELLOW);
    printf("%-6s","3.");
    Console::setColor(WHITE);
    printf("%s\n","\u4fee\u6539YAML\u6a94\u6848");
    Console::setColor(YELLOW);
    printf("%-6s","4.");
    Console::setColor(WHITE);
    printf("%s\n","\u7d50\u675f");
    printf("\u8acb\u8f38\u5165\u6a21\u5f0f\u7de8\u865f: ");
}

void PrintComputerConnecting(const string& com){
    Console::setColor(WHITE);
    cout << "Connecting to ";
    Console::setColor(YELLOW);
    cout << com;
    Console::setColor(WHITE);
    cout << " - ";
}

void PrintFailedToConnect(){
    Console::setColor(RED);
    cout << "Failed! \n";
    Console::setColor(WHITE);
}

void PrintResembleFile(sftp_session sftp, sftp_dir dir,const string &fileName){
    sftp_attributes attributes;

    bool firstPrint = true;
    while( (attributes = sftp_readdir(sftp,dir) ) != nullptr){
        fs::path file1(attributes -> name), file2(fileName);

        if(!IsResembleFile(file1,file2)){
            sftp_attributes_free(attributes);
            continue;
        }

        if(firstPrint){
            cout << "Find resemble file names: ";
            firstPrint = false;
            Console::setColor(YELLOW);
        }
        else{
            cout<< ", ";
        }

        cout << file1.u8string();

        sftp_attributes_free(attributes);
    }

    if(!firstPrint) cout << endl;

    Console::setColor(WHITE);
}

void FreeContainer(PropertiesPtr container){
    delete container -> ip;
    delete container -> user;
    delete container;
}

void AddFileToServer(ConfigReader reader,ssh_session sshSession){
    sftp_session sftp = GetSFTPSession(sshSession);
    if(sftp == nullptr ) return;

    reader.initiateAddFileMode();
    YAML::Node baseDirs = reader.getBaseDir();
    fs::path targetFilesDir(reader.getAddFileTargetFiles());
    fs::path serverSubDir(reader.getSubdirectory());

    if(!fs::exists(targetFilesDir) && !fs::is_directory(targetFilesDir)){
        PrintFailedToConnect();
        printf("No directory call %ls\n", targetFilesDir.c_str());
        sftp_free(sftp);
        return;
    }

    cout << endl;

    for(int i = 0 ; i < baseDirs.size() ; i++){
        fs::path serverPath(baseDirs[i].as<string>() + "/" + serverSubDir.u8string());

        cout<< "Adding file to " << serverPath << " - ";
        WriteFileToTargetDir(sshSession,targetFilesDir, sftp, serverPath);

        Console::setColor(GREEN);
        cout << "finish\n";
        Console::setColor(WHITE);
    }

    sftp_free(sftp);
}

void WriteFileToTargetDir(const ssh_session &sshSession,const fs::path &directoryPath ,sftp_session sftp, const fs::path &serverPath){
    int access_type = O_WRONLY | O_CREAT | O_TRUNC;
    int permission = 0755;

    for(auto const &itr : fs::directory_iterator(directoryPath) ){
        fs::path serverPathWithFile(serverPath.u8string() + "/" + itr.path().filename().u8string());

        if(itr.is_directory()){
            if(sftp_mkdir(sftp,serverPathWithFile.u8string().c_str(),permission) != SSH_OK){
                if(sftp_get_error(sftp) != SSH_FX_FILE_ALREADY_EXISTS){
                    fprintf(stderr, "Can't create directory: %s\n", ssh_get_error(sshSession));
                    continue;
                }
            }

            WriteFileToTargetDir(sshSession,itr.path(),sftp,serverPathWithFile);
            continue;
        }

        sftp_file serverFile = sftp_open(sftp,serverPathWithFile.u8string().c_str(),access_type,permission);

        if (serverFile == nullptr) {
            fprintf(stderr, "Can't open file for writing: %s\n",
                    ssh_get_error(sshSession));
            continue;
        }

        bool isSuccess = CopyFileToRemote(itr.path(),serverFile);

        if(!isSuccess){
            Console::setColor(WHITE);
            cout << "Can't writ file ";
            Console::setColor(YELLOW);
            cout << itr.path() << endl;
            Console::setColor(WHITE);
        }

        sftp_close(serverFile);
    }
}

void DeleteFileFromServer(ConfigReader reader,ssh_session sshSession){
    reader.initiateDeleteFileMode();
    YAML::Node baseDirs = reader.getBaseDir();
    fs::path serverSubDir(reader.getSubdirectory());



    sftp_session sftp = GetSFTPSession(sshSession);
    if(sftp == nullptr) return;

    string fileName = reader.getFileName();
    cout << endl;

    for(int i = 0 ; i < baseDirs.size() ; i++){
        fs::path serverPath(baseDirs[i].as<string>() + "/" + serverSubDir.u8string());

        cout<< "Delete file from " << serverPath << endl;

        sftp_dir dir = GetSFTPDir(sshSession,sftp,serverPath);
        if(dir == nullptr) continue;
        DeleteServerFile(reader,dir,sftp,sshSession,serverPath);
        sftp_closedir(dir);

        dir = GetSFTPDir(sshSession,sftp,serverPath);
        if(dir == nullptr) continue;
        PrintResembleFile(sftp,dir,fileName);
        sftp_closedir(dir);

    }


    sftp_free(sftp);
}

void DeleteServerFile(ConfigReader reader,sftp_dir dir,sftp_session sftp,ssh_session sshSession,const fs::path &serverPath){
    ssh_channel channel = GetShellSession(sshSession);
    if(channel == nullptr) {
        return;
    }

    string fileName = reader.getFileName();

    if(!IsFileExists(sftp,dir,fileName)){
        PrintFailedToConnect();
        cout << "Failed to delete file in " << serverPath << endl;
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return;
    }

    string command("rm ");
    command += serverPath.u8string() + "/" + fileName;

    ssh_channel_request_exec(channel, command.c_str());

    Console::setColor(GREEN);
    cout <<"Success" << endl;
    Console::setColor(WHITE);

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
}

void ModifyYAMLFile(ConfigReader reader,ssh_session sshSession){
    reader.initiateModifyYAMLMode();

    fs::path tempFile("temp");
    if(!fs::exists(tempFile)){
        fs::create_directories(tempFile);
    }


    YAML::Node baseDirs = reader.getBaseDir();
    fs::path serverSubDir(reader.getSubdirectory());
    string fileName = reader.getFileName();

    tempFile += "/" + fileName;


    sftp_session sftp = GetSFTPSession(sshSession);
    if(sftp == nullptr) return;

    cout << endl;

    int access_type = O_WRONLY | O_CREAT | O_TRUNC;

    for(int i = 0 ; i < baseDirs.size() ; i++){
        fs::path serverPath(baseDirs[i].as<string>() + "/" + serverSubDir.u8string());
        cout<< "Modifying file " << serverPath << endl;

        sftp_dir dir = GetSFTPDir(sshSession,sftp,serverPath);
        if(!IsFileExists(sftp,dir,fileName)){
            PrintFailedToConnect();
            cout << "File not found!\n";

            sftp_closedir(dir);
            continue;
        }

        sftp_closedir(dir);


        serverPath += "/" + fileName;

        if(!CopyFileFromServer(sshSession,sftp,serverPath,tempFile)){
            PrintFailedToConnect();
            continue;
        }


        YamlCommentSaver saver(tempFile);
        ModifyYaml(reader,tempFile);


        YamlCommentPrinter printer(saver);
        printer.AddCommentToFile();



        sftp_file serverFile = sftp_open(sftp,serverPath.u8string().c_str(),access_type,0700);

        if (serverFile == nullptr) {
            fprintf(stderr, "Can't open file for writing: %s\n",
                    ssh_get_error(sftp));
            continue;
        }

        bool isSuccess = CopyFileToRemote(printer.getFilePath(),serverFile);

        if(!isSuccess){
            Console::setColor(WHITE);
            cout << "Can't writ file ";
            Console::setColor(YELLOW);
            cout << printer.getFilePath() << endl;
            Console::setColor(WHITE);
        }

        sftp_close(serverFile);


        Console::setColor(GREEN);
        cout <<"Finish" << endl;
        Console::setColor(WHITE);
    }

    sftp_free(sftp);
}

void ModifyYaml(ConfigReader reader,const fs::path &file){
    YAML::Node node = YAML::LoadFile(file.u8string());
    vector<string> splitStr;
    SplitString(splitStr,reader.getModifyNodePath(),'.');

    YAML::Node nodeToTarget = GetDeepNode(node,splitStr.begin(),splitStr);

    nodeToTarget = reader.getModifyNodeValue();



    fstream out(file,ios::out | ios::trunc);
    out << node;


}

void SplitString(vector<string> &list,const string &context,char split){
    string temp(context);
    while (!temp.empty()){
        string splitPart;

        for(string::const_iterator itr = temp.begin() ; itr != temp.end() && *itr != split ; itr ++){
            splitPart += *itr;
        }

        list.push_back(splitPart);
        temp = temp.substr(splitPart.size());

        if(!temp.empty()){
            temp = temp.substr(1);
        }

    }
}

bool CopyFileFromServer(ssh_session sshSession, sftp_session sftp, const fs::path &from, const fs::path &to){
    int numOfByte, numOfWrite;
    char buffer[1024];

    int access_type = O_RDONLY;
    sftp_file serverFile = sftp_open(sftp,from.u8string().c_str(),access_type,0);

    if (serverFile == NULL) {
        fprintf(stderr, "Can't open file for reading: %s\n",
                ssh_get_error(sshSession));
        return false;
    }


    fstream localFile(to,ios::out | ios::trunc | ios::binary);


    while(true){
        numOfByte = sftp_read(serverFile,buffer,sizeof(buffer));
        if(numOfByte == 0) break;
        else if(numOfByte < 0){
            fprintf(stderr, "Error while reading server file: %s\n",
                    ssh_get_error(sshSession));
            sftp_close(serverFile);
            localFile.close();
            return false;
        }


        localFile.write(buffer,numOfByte);
    }

    sftp_close(serverFile);
    localFile.close();

    return true;

}

bool IsResembleFile(const fs::path &file1, const fs::path &file2){
    if(!HasSameFileExtension(file1,file2))return false;
    string stripVersion1 = file1.stem().u8string();

    int len = 0;
    for(string::const_iterator itr = stripVersion1.begin(); itr != stripVersion1.end() ; itr++, len++){
        if(*itr >= '0' && *itr < '9'){
            break;
        }
    }

    stripVersion1 =  stripVersion1.substr(0,len);

    string stripVersion2 = file2.stem().u8string();

    len = 0;
    for(string::const_iterator itr = stripVersion2.begin(); itr != stripVersion2.end() ; itr++, len++){
        if(*itr >= '0' && *itr < '9'){
            break;
        }
    }

    stripVersion2 =  stripVersion2.substr(0,len);

    return stripVersion1 == stripVersion2;
}

bool HasSameFileExtension(const fs::path &file1, const fs::path &file2){
    if(file1.has_extension() != file2.has_extension()) return false;


    if(file1.has_extension()){
        if(file1.extension().compare(file2.extension()) != 0){
            return false;
        }
    }

    return true;
}


bool IsFileExists(sftp_session sftp, sftp_dir dir,const string &fileName){
    sftp_attributes attributes;

    const char * cFileName = fileName.c_str();
    bool isSame = false;
    while( (attributes = sftp_readdir(sftp,dir) ) != nullptr && !isSame){
        if(strcmp(cFileName, attributes ->name) == 0) isSame = true;

        sftp_attributes_free(attributes);
    }


    return isSame;
}

bool CopyFileToRemote(const fs::path &from, sftp_file &to){
    fstream fromFile(from.u8string(),fstream ::in | fstream ::binary);

    streampos currentPos = fromFile.tellg();
    fromFile.seekg(0,fstream ::end);
    streampos endPos = fromFile.tellg();
    fromFile.seekg(0,fstream::beg);

    long long maxSize = endPos - currentPos > 20480 ? 20480 : endPos - currentPos;
    char *buf = new char[maxSize];

    while(true){
        long long size = endPos - currentPos > maxSize ? maxSize :  endPos - currentPos;
        fromFile.read(buf,size);

        int nwritten = sftp_write(to, buf, size);
        if (nwritten != size) {
            fromFile.close();
            delete[] buf;
            return false;
        }

        currentPos = fromFile.tellg();
        if(endPos - currentPos == 0) break;
    }



    fromFile.close();
    delete[] buf;

    return true;
}



PropertiesPtr CopyToContainer(ConfigReader reader){
    PropertiesPtr res;
    res = new ServerProperties;
    string user = reader.getUser();

    res -> port = reader.getPort();
    res -> verbose = SSH_LOG_INFO;
    res -> user = new char [user.size()+1];
    res -> ip = new char [24];
    strcpy(res -> user,user.c_str());

    return res;
}

