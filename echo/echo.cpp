#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

constexpr const char* SERVER_IP = "172.16.40.13";
constexpr unsigned short SERVER_PORT = 7;

bool sendAll(int socketFd, const std::string& message)
{
    size_t sentTotal = 0;

    while (sentTotal < message.size()) {
        ssize_t sent = send(socketFd, message.data() + sentTotal,
                            message.size() - sentTotal, 0);

        if (sent <= 0) {
            std::perror("send");
            return false;
        }

        sentTotal += static_cast<size_t>(sent);
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0]
                  << " <текстовое сообщение>\n";
        return 1;
    }

    std::string message;

    for (int i = 1; i < argc; ++i) {
        if (i > 1) {
            message += ' ';
        }

        message += argv[i];
    }

    int socketFd = socket(AF_INET, SOCK_STREAM, 0);

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

    if (connect(socketFd, reinterpret_cast<sockaddr*>(&server),
                sizeof(server)) < 0) {
        std::perror("connect");
        close(socketFd);
        return 1;
    }

    if (!sendAll(socketFd, message)) {
        close(socketFd);
        return 1;
    }

    std::string response;
    char buffer[1024];

    while (response.size() < message.size()) {
        ssize_t received = recv(socketFd, buffer, sizeof(buffer), 0);

        if (received < 0) {
            std::perror("recv");
            close(socketFd);
            return 1;
        }

        if (received == 0) {
            std::cerr << "Сервер закрыл соединение до получения ответа.\n";
            close(socketFd);
            return 1;
        }

        response.append(buffer, static_cast<size_t>(received));
    }

    std::cout << "Отправлено: " << message << '\n';
    std::cout << "Получено: " << response << '\n';

    if (response == message) {
        std::cout << "Проверка пройдена: сервер вернул исходное сообщение.\n";
    } else {
        std::cerr << "Ошибка: полученный ответ не совпадает с сообщением.\n";
        close(socketFd);
        return 2;
    }

    close(socketFd);
    return 0;
}
