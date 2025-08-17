#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

#define SERVER_IP "127.0.0.1"
#define CONTROL_PORT 2121
#define BUFFER_SIZE 1024

enum TransferMode
{
    ASCII,
    BINARY
};

int connectToPort(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    inet_pton(AF_INET, SERVER_IP, &servAddr.sin_addr);
    connect(sock, (sockaddr *)&servAddr, sizeof(servAddr));
    return sock;
}

int createClientListener(int &port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;

    bind(sockfd, (sockaddr *)&addr, sizeof(addr));
    socklen_t len = sizeof(addr);
    getsockname(sockfd, (sockaddr *)&addr, &len);
    port = ntohs(addr.sin_port);

    listen(sockfd, 1);
    return sockfd;
}

std::string getLastPathComponent(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    return (pos != std::string::npos) ? path.substr(pos + 1) : path;
}

void receiveFile(int sock, const std::string &filename, TransferMode mode)
{
    std::ofstream file("received_" + getLastPathComponent(filename), (mode == BINARY) ? std::ios::binary : std::ios::out);
    char buffer[BUFFER_SIZE];
    std::string data;

    while (true)
    {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0)
            break;
        data.append(buffer, bytes);

        size_t pos = data.find("<FILE_END>");
        if (pos != std::string::npos)
        {
            file.write(data.c_str(), pos);
            break;
        }
    }

    file.close();
    std::cout << "Received and saved as received_" << filename << "\n";
}

int main()
{
    int controlSock = connectToPort(CONTROL_PORT);
    std::cout << "Connected to server control port.\n";

    std::string input;
    int dataSock = -1;
    bool inSession = false;
    TransferMode mode = ASCII;

    while (true)
    {
        std::cout << "ftp> ";
        std::getline(std::cin, input);

        if (input == "quit")
        {
            char buffer[1024] = {0};
            send(controlSock, input.c_str(), input.size(), 0);
            recv(controlSock, buffer, sizeof(buffer), 0);
            std::cout << std::string(buffer);

            break;
        }

        else if (input == "PASV")
        {
            if (dataSock != -1)
                close(dataSock);
            send(controlSock, input.c_str(), input.size(), 0);

            char buffer[BUFFER_SIZE] = {0};
            recv(controlSock, buffer, BUFFER_SIZE, 0);
            std::string response(buffer);
            int port = std::stoi(response.substr(response.find(":") + 1));
            dataSock = connectToPort(port);
            std::cout << "Connected to data port " << port << "\n";
            inSession = true;
        }
        else if (input == "LS")
        {
            if (dataSock != -1)
            {
                /* code */

                char buffer[1024] = {0};
                send(controlSock, input.c_str(), input.size(), 0);
                recv(dataSock, buffer, sizeof(buffer), 0);
                std::cout << std::string(buffer);
                memset(buffer, 0, sizeof(buffer));
                recv(controlSock, buffer, sizeof(buffer), 0);
                std::cout << std::string(buffer);
            }
            else
            {
                std::cout << "First Select PASV Or ACTIVE Mode\n";
            }
        }

        else if (input == "ACTIVE")
        {
            if (dataSock != -1)
                close(dataSock);
            int dataPort;
            int listener = createClientListener(dataPort);
            std::string command = "ACTIVE:" + std::to_string(dataPort);
            send(controlSock, command.c_str(), command.size(), 0);
            dataSock = accept(listener, nullptr, nullptr);
            close(listener);
            std::cout << "Server connected to active port " << dataPort << "\n";
            inSession = true;
        }
        else if (input == "ASCII" || input == "BINARY")
        {
            send(controlSock, input.c_str(), input.size(), 0);
            mode = (input == "BINARY") ? BINARY : ASCII;
        }
        else if (input.substr(0, 4) == "RETR")
        {
            if (inSession && dataSock != -1)
            {
                char buffer[1024] = {0};
                send(controlSock, input.c_str(), input.size(), 0);
                recv(controlSock, buffer, sizeof(buffer), 0);
                std::string msg (buffer);
                if (msg.substr(0,3) == "425" || msg.substr(0,3) == "550" || msg.substr(0,3) == "560")
                {
                    std::cout << msg;
                }
                else {
                    receiveFile(dataSock, input.substr(5), mode);
                    std::cout << msg;
                }
                
                
                
            }
            else
            {
                std::cout << "First Select PASV Or ACTIVE Mode\n";
            }
        }
        else if (input.substr(0, 4) == "User")
        {
            send(controlSock, input.c_str(), input.size(), 0);
            char buffer[1024] = {0};
            recv(controlSock, buffer, sizeof(buffer), 0);
            std::string msg(buffer);
            if (msg.substr(0, 3) == "200")
            {
                std::cout << msg;
            }
            else
            {
                std::cout << msg;
                do
                {
                    std::cout << "ftp> ";
                    std::getline(std::cin , input);
                    send(controlSock , input.c_str() , input.size() , 0);
                    memset(buffer, 0, sizeof(buffer));
                    recv(controlSock, buffer, sizeof(buffer), 0);
                    msg = std::string(buffer);
                    std::cout << msg;
                } while (msg.substr(0, 3) == "331");
            }
        }
        else
        {
            char buffer[1024] = {0};
            send(controlSock, input.c_str(), input.size(), 0);
            recv(controlSock, buffer, sizeof(buffer), 0);
            std::cout << std::string(buffer);
        }
    }

    if (dataSock != -1)
        close(dataSock);
    close(controlSock);
    return 0;
}
