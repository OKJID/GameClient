#pragma once
#ifdef __APPLE__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

typedef int SOCKET;
#define INVALID_SOCKET ((SOCKET)-1)
#define SOCKET_ERROR (-1)
#define SD_SEND SHUT_WR
#define SD_BOTH SHUT_RDWR
#define closesocket close

#endif
