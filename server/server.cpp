#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
// new
#include <sstream>
#include <sys/socket.h>
#include <deque>
#include <limits.h>
#include <set>

#define BUFFER_SIZE 1024

std::string first_path = "";
std::string config_path = "";
int control_port;
std::set<std::string> user_directory;

struct User
{
    std::string userName = "";
    std::string password = "";
    int download_quota = 0;
    int admin = 0;
};

std::string getParentFolder(const std::string &path)
{
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash == 0)
        return "";

    size_t secondLastSlash = path.find_last_of('/', lastSlash - 1);
    if (secondLastSlash == std::string::npos)
        return "";

    return path.substr(secondLastSlash + 1, lastSlash - secondLastSlash - 1);
}

int get_user_quota(const std::string &username)
{
    std::ifstream file(config_path);
    if (!file.is_open())
    {
        std::cout << "eshak_1";
        return -1;
    }

    std::string line;
    bool found_quota_section = false;
    while (std::getline(file, line))
    {
        if (line == "user_quota:")
        {
            found_quota_section = true;
            continue;
        }
        if (found_quota_section)
        {
            std::istringstream iss(line);
            std::string user;
            int quota;
            if (iss >> user >> quota)
            {
                if (user == username)
                {
                    file.close();
                    return quota;
                }
            }
        }
    }
    std::cout << "eshak_2";
    file.close();
    return -1;
}

void update_user_quota(const std::string &username, const std::string &filepath)
{
    struct stat stat_buf;
    if (stat(filepath.c_str(), &stat_buf) != 0)
    {
        std::cerr << "Cannot access file: " << filepath << std::endl;
        return;
    }
    int file_size_kb = stat_buf.st_size / 1024;

    std::ifstream file(config_path);
    if (!file.is_open())
    {
        std::cerr << "Cannot open config file!\n";
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    bool in_quota_section = false;
    bool user_found = false;

    while (std::getline(file, line))
    {
        if (line == "user_quota:")
        {
            in_quota_section = true;
            lines.push_back(line);
            continue;
        }
        if (in_quota_section)
        {
            std::istringstream iss(line);
            std::string user;
            int quota;
            if (iss >> user >> quota)
            {
                if (user == username)
                {
                    if (quota >= file_size_kb)
                    {
                        quota -= file_size_kb;
                        user_found = true;
                    }
                    else
                    {
                        std::cerr << "Not enough quota for user " << username << std::endl;
                        file.close();
                        return;
                    }
                }
                lines.push_back(user + " " + std::to_string(quota));
            }
            else
            {
                lines.push_back(line);
            }
        }
        else
        {
            lines.push_back(line);
        }
    }
    file.close();

    if (!user_found)
    {
        std::cerr << "User not found in quota list.\n";
        return;
    }

    std::ofstream out(config_path, std::ios::trunc);
    if (!out.is_open())
    {
        std::cerr << "Cannot open config file for writing!\n";
        return;
    }

    for (const auto &l : lines)
    {
        out << l << '\n';
    }
    out.flush();
    out.close();
}
bool can_download_file(const std::string &filepath, int user_quota_kb)
{
    struct stat stat_buf;
    if (stat(filepath.c_str(), &stat_buf) != 0)
    {
        return false;
    }
    long long file_size_kb = stat_buf.st_size / 1024;
    return user_quota_kb >= file_size_kb;
}

void load_config()
{
    std::ifstream file(config_path);
    if (!file.is_open())
    {
        std::cerr << "Cannot open config file!\n";
        return;
    }

    std::string line;
    enum Mode
    {
        NONE,
        USER,
        QUOTA
    };
    Mode mode = NONE;

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        if (line.rfind("PORT:", 0) == 0)
        {
            control_port = std::stoi(line.substr(5));
        }
        else if (line == "USER:")
        {
            mode = USER;
        }
        else if (line == "user_quota:")
        {
            mode = QUOTA;
        }
        else
        {
            if (mode == USER)
            {
                user_directory.insert(line);
            }
        }
    }

    file.close();
}

void removeNewline(std::string &str)
{
    if (!str.empty() && str.back() == '\n')
    {
        str.pop_back();
    }
}

bool pathExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string shell(const std::string &command)
{
    if (command.rfind("cd ", 0) == 0)
    {
        std::string path = command.substr(3);
        if (chdir(path.c_str()) != 0)
        {
            return strerror(errno);
        }
        return "Directory changed.";
    }
    else
    {
        std::string result;
        char buffer[128];
        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            return "popen failed!";
        }

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            result += buffer;
        }

        pclose(pipe);
        return result;
    }
}

std::deque<std::string> stringTOsplit(std::string enter)
{
    std::deque<std::string> words;
    std::string word = "";
    std::istringstream split(enter);

    while (split >> word)
    {
        words.push_back(word);
    }

    return words;
}

bool isValidUsername(const std::string &username)
{
    std::ifstream infile("private/users.txt");
    std::string line;

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string user;
        if (iss >> user)
        {
            if (user == username)
                return true;
        }
    }
    return false;
}

bool isAuthenticated(const std::string &username, const std::string &password)
{
    std::ifstream infile("private/users.txt");
    std::string line;

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::string user, pass;
        if (iss >> user >> pass)
        {
            if (user == username && pass == password)
                return true;
        }
    }
    return false;
}
int isAdmin(const std::string &username)
{
    std::ifstream infile("private/users.txt");
    std::string user, pass, admin;
    while (infile >> user >> pass >> admin)
    {
        if (user == username)
        {
            return std::stoi(admin);
        }
    }
    return -1;
}

std::string generateCommand(const std::deque<std::string> &enter, std::string command)
{

    for (std::string word : enter)
    {
        command += " ";
        command += word;
    }
    return command;
}

void checkCommand(std::string enter, int clientControl, int dataSock, User user)
{
    std::deque<std::string> words = stringTOsplit(enter);

    std::string command = words.at(0);
    words.pop_front();
    std::string out = "choom";
    if (command == "PWD")
    {
        out = "257 \"";
        out += shell(generateCommand(words, "pwd"));
        removeNewline(out);
        out += "\" is the current directory\n";
    }
    else if (command == "CWD")
    {
        if (words.empty())
        {
            std::string temp = "cd ";
            temp += first_path;
            shell(temp);
            out += "Directory changed to \"";
            out += first_path;
            out += "\"\n";
        }
        else
        {
            if (pathExists(words.at(0)))
            {
                std::string directory = words.at(0);
                if (directory == ".." || directory == "-")
                {
                    directory = getParentFolder(shell("pwd"));
                }
                else
                {
                    directory = words.at(0);
                }

                if (user.admin == 1 || user_directory.count(directory) > 0)
                {
                    shell(generateCommand(words, "cd"));
                    out = "250 ";
                    out += "Directory changed to \"";
                    out += words.at(0);
                    out += "\"\n";
                }
                else
                {
                    out = "550 Permission denied.\n";
                }
            }
            else
            {
                out = "No such file or directory\n";
            }
        }
    }
    else if (command == "LS")
    {
        out = shell(generateCommand(words, "ls"));
        send(dataSock, out.c_str(), out.size(), 0);
        out = "200 PORT command successful.\n150 Opening ASCII mode data connection for file list.\n";
        send(clientControl, out.c_str(), out.size(), 0);
        return;
    }
    else if (command == "MKD")
    {
        if (!pathExists(words.at(0)))
        {
            shell(generateCommand(words, "mkdir"));
            out = "227 \"";
            out += words.at(0);
            out += "\" directory created\n";
        }
        else
        {
            out = "This Directory Exixts\n";
        }
    }
    else if (command == "DELE")
    {
        std::string flag = words.at(0);
        words.pop_front();
        if (pathExists(words.at(0)))
        {
            if (user.admin == 1 || user_directory.count(words.at(0)) > 0)
            {

                if (flag == "-f")
                {
                    shell(generateCommand(words, "rm -r"));
                    out = "250 \"";
                    out += words.at(0);
                    out += "\" file deleted\n";
                }
                else if (flag == "-d")
                {
                    shell(generateCommand(words, "rm -r"));
                    out = "250 \"";
                    out += words.at(0);
                    out += "\" directory deleted\n";
                }
                else
                {
                    out = "DELE: unrecognized option '-flag' \nTry 'DELE -d' or 'DELE -f\n'";
                }
            }
            else
            {
                out = "550 Permission denied.\n";
            }
        }
        else
        {
            out = "No such file or directory\n";
        }
    }
    else if (command == "RENAME")
    {
        if (pathExists(words.at(0)))
        {
            if (user.admin == 1 || user_directory.count(words.at(0)) > 0)
            {
                shell(generateCommand(words, "mv"));
                out = "250 Rename successful: ";
                out += words.at(0);
                out += " renamed to ";
                out += words.at(1);
                out += "\n";
            }
            else
            {
                out = "550 Permission denied.\n";
            }
        }
        else
        {
            out = "No such file or directory\n";
        }
    }
    else if (command == "quit")
    {
        out = "221 Goodbye!\n";
    }

    else if (command == "HELP")
    {
        if (words.empty())
        {
            out = "Available commands:\n"
                  "  PWD    - Print working directory\n"
                  "  CWD    - Change working directory\n"
                  "  LS     - List directory contents\n"
                  "  MKD    - Make new directory\n"
                  "  DELE   - Delete file or directory\n"
                  "  RENAME - Rename file or directory\n"
                  "  RETR   - Retrieve (download) a file\n"
                  "  QUIT   - Close the connection\n"
                  "Use HELP <command> to get detailed information.\n";
        }
        else if (words.at(0) == "PWD")
        {
            out = "214 Syntax: PWD\nDisplay the current working directory on the server.\n";
        }
        else if (words.at(0) == "CWD")
        {
            out = "214 Syntax: CWD <path>\nChange the server's current directory to the specified path.\n";
        }
        else if (words.at(0) == "LS")
        {
            out = "214 Syntax: LS\nList the contents of the current directory on the server.\n";
        }
        else if (words.at(0) == "MKD")
        {
            out = "214 Syntax: MKD <directory>\nCreate a new directory in the current path on the server.\n";
        }
        else if (words.at(0) == "DELE")
        {
            out = "214 Syntax: DELE -f <file>\nDelete the specified file.\nDELE -d <directory>: Delete the specified directory.\n";
        }
        else if (words.at(0) == "RENAME")
        {
            out = "214 Syntax: RENAME <oldname> <newname>\nRename a file or directory on the server.\n";
        }
        else if (words.at(0) == "RETR")
        {
            out = "214 Syntax: RETR <filename>\nDownload the specified file from the server.\n";
        }
        else if (words.at(0) == "quit" || words.at(0) == "QUIT")
        {
            out = "214 Syntax: QUIT\nTerminate the connection with the server and exit.\n";
        }
        else
        {
            out = "Unknown command. Use HELP to see available commands.\n";
        }
    }
    else
    {
        out = "501 Syntax error in parameters or arguments.\n";
    }

    send(clientControl, out.c_str(), out.size(), 0);
}

enum TransferMode
{
    ASCII,
    BINARY
};

int createListeningSocket(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(sockfd, (sockaddr *)&addr, sizeof(addr));
    listen(sockfd, 1);
    return sockfd;
}

int createPassiveSocket(int &chosenPort)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    bind(sockfd, (sockaddr *)&addr, sizeof(addr));
    socklen_t len = sizeof(addr);
    getsockname(sockfd, (sockaddr *)&addr, &len);
    chosenPort = ntohs(addr.sin_port);

    listen(sockfd, 1);
    return sockfd;
}

int connectToClient(const std::string &clientIP, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, clientIP.c_str(), &addr.sin_addr);

    connect(sock, (sockaddr *)&addr, sizeof(addr));
    return sock;
}

void sendFileASCII(int dataSock, const std::string &filename)
{
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line))
    {
        line += "\r\n";
        send(dataSock, line.c_str(), line.size(), 0);
    }
    std::string endSignal = "<FILE_END>";
    send(dataSock, endSignal.c_str(), endSignal.size(), 0);
}

void sendFileBinary(int dataSock, const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        std::string err = "ERROR: File not found.\n";
        send(dataSock, err.c_str(), err.size(), 0);
        return;
    }
    char buffer[BUFFER_SIZE];
    while (file.read(buffer, BUFFER_SIZE))
        send(dataSock, buffer, BUFFER_SIZE, 0);
    if (file.gcount() > 0)
        send(dataSock, buffer, file.gcount(), 0);
    std::string endSignal = "<FILE_END>";
    send(dataSock, endSignal.c_str(), endSignal.size(), 0);
}

int main()
{
    first_path = shell("pwd");
    removeNewline(first_path);
    config_path = first_path + "/private/config.txt";
    load_config();
    bool logIn = false;
    User user;
    std::deque<std::string> words;
    int controlSock = createListeningSocket(control_port);
    std::cout << "Server listening on control port " << control_port << "...\n";

    int clientControl = accept(controlSock, nullptr, nullptr);
    std::cout << "Client connected.\n";

    char buffer[BUFFER_SIZE];
    std::string clientIP = "127.0.0.1";
    int dataSock = -1;
    bool inSession = false;
    TransferMode mode = ASCII;

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(clientControl, buffer, BUFFER_SIZE, 0);
        if (len <= 0)
            break;
        words.clear();
        words = stringTOsplit(std::string(buffer));
        std::string command = words.at(0);
        words.pop_front();
        if (!logIn && command != "User")
        {
            std::string msg = "503 Bad sequence of commands.\n";
            send(clientControl, msg.c_str(), msg.size(), 0);
            continue;
        }

        else if (command == "PASV")
        {
            if (dataSock != -1)
                close(dataSock);
            int dataPort;
            int dataListener = createPassiveSocket(dataPort);
            std::string portMsg = "PORT:" + std::to_string(dataPort);
            send(clientControl, portMsg.c_str(), portMsg.size(), 0);

            dataSock = accept(dataListener, nullptr, nullptr);
            std::cout << "Client connected to data port " << dataPort << "\n";
            close(dataListener);
            inSession = true;
        }
        else if (command.rfind("ACTIVE:", 0) == 0)
        {
            if (dataSock != -1)
                close(dataSock);
            int port = std::stoi(command.substr(7));
            dataSock = connectToClient(clientIP, port);
            std::cout << "Connected to client data port " << port << "\n";
            inSession = true;
        }
        else if (command == "ASCII")
        {
            mode = ASCII;
        }
        else if (command == "BINARY")
        {
            mode = BINARY;
        }

        else if (command == "RETR")
        {
            if (inSession && dataSock != -1)
            {
                if (pathExists(words.at(0)))
                {
                    std::cout << "Quota: " << user.download_quota << std::endl
                              << "File: " << words.at(0);
                    if (!can_download_file(words.at(0), user.download_quota))
                    {
                        std::cout << "-----------------";
                        std::cout << "Quota: " << user.download_quota << std::endl
                                  << "File: " << words.at(0);
                        std::string msg = "425 Cannot open data connection\n";
                        send(clientControl, msg.c_str(), msg.size(), 0);
                    }
                    else
                    {
                        if (user.admin == 1 || user_directory.count(words.at(0)) > 0)
                        {
                            std::string filename = words.at(0);
                            std::string msg = "226 Transfer complete.\n";
                            send(clientControl, msg.c_str(), msg.size(), 0);
                            if (mode == ASCII)
                                sendFileASCII(dataSock, filename);
                            else
                                sendFileBinary(dataSock, filename);
                            update_user_quota(user.userName, words.at(0));
                            user.download_quota = get_user_quota(user.userName);
                        }
                        else
                        {
                            std::string msg = "550 Permission denied.\n";
                            send(clientControl, msg.c_str(), msg.size(), 0);
                        }
                    }
                }
                else
                {
                    std::string msg = "560 No such file or directory\n";
                    send(clientControl, msg.c_str(), msg.size(), 0);
                }
            }
        }
        else if (command == "User")
        {
            std::string msg = "";
            if (logIn)
            {
                msg = "200 ";
                msg += user.userName;
                msg += "\n";
                send(clientControl, msg.c_str(), msg.size(), 0);
            }
            else
            {
                if (isValidUsername(words.at(0)))
                {
                    user.userName = words.at(0);
                    msg = "331 Password required for username.\n";
                    do
                    {
                        send(clientControl, msg.c_str(), msg.size(), 0);
                        memset(buffer, 0, sizeof(buffer));
                        recv(clientControl, buffer, BUFFER_SIZE, 0);
                        words.clear();
                        words = stringTOsplit(std::string(buffer));
                    } while (words.at(0) != "Pass");
                    if (isAuthenticated(user.userName, words.at(1)))
                    {
                        user.password = words.at(0);
                        msg = "230 User ";
                        msg += user.userName;
                        msg += " logged in.\n";
                        send(clientControl, msg.c_str(), msg.size(), 0);
                        logIn = true;
                        user.admin = isAdmin(user.userName);
                        user.download_quota = get_user_quota(user.userName);
                    }
                    else
                    {
                        msg = "530 Login incorrect.\n";
                        send(clientControl, msg.c_str(), msg.size(), 0);
                    }
                }
                else
                {
                    msg = "Not Exists\n";
                    send(clientControl, msg.c_str(), msg.size(), 0);
                }
            }
        }
        else
        {
            checkCommand(std::string(buffer), clientControl, dataSock, user);
        }
    }

    if (dataSock != -1)
        close(dataSock);
    close(clientControl);
    close(controlSock);
    return 0;
}
