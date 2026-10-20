#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

constexpr const char* SERVER_IP = "172.16.40.1";
constexpr unsigned short SERVER_PORT = 13;

int main()
{
    int socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFd < 0) {
        std::perror("socket");
        return 1;
    }

    timeval timeout{};
    timeout.tv_sec = 5;

    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0) {
        std::perror("setsockopt");
        close(socketFd);
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) != 1) {
        std::cerr << "Ошибка преобразования IP-адреса.\n";
        close(socketFd);
        return 1;
    }

    const char request[] = "time";

    if (sendto(socketFd, request, sizeof(request) - 1, 0,
               reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        std::perror("sendto");
        close(socketFd);
        return 1;
    }

    char buffer[1024];
    sockaddr_in sender{};
    socklen_t senderSize = sizeof(sender);

    ssize_t received = recvfrom(socketFd, buffer, sizeof(buffer), 0,
                                reinterpret_cast<sockaddr*>(&sender),
                                &senderSize);

    if (received < 0) {
        std::perror("recvfrom");
        close(socketFd);
        return 1;
    }

    std::cout << "Ответ сервера daytime: ";
    std::cout.write(buffer, received);

    if (received == 0 || buffer[received - 1] != '\n') {
        std::cout << '\n';
    }

    close(socketFd);
    return 0;
}
