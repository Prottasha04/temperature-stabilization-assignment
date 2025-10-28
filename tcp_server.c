// tcp_server.c — central process (server)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>   // struct sockaddr_in
#include <arpa/inet.h>    // inet_pton / inet_ntop
#include "utils.h"

#define NUM_EXTERNALS 4
#define HOST        "127.0.0.1"
#define PORT        2000
#define EPS         1e-3f

static void die(const char *msg) { perror(msg); exit(1); }

// Accept exactly 4 clients; return pointer to static array of fds and set *listen_fd_out
static int* establishConnectionsFromExternalProcesses(int *listen_fd_out) {
    static int client_fd[NUM_EXTERNALS];
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) die("socket");

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, HOST, &addr.sin_addr);

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(s, NUM_EXTERNALS) < 0) die("listen");

    printf("[S] Listening on %s:%d\n", HOST, PORT);
    printf("-------------------- Initial connections --------------------\n");

    for (int i = 0; i < NUM_EXTERNALS; i++) {
        struct sockaddr_in cli;
        socklen_t clen = sizeof(cli);
        int cfd = accept(s, (struct sockaddr*)&cli, &clen);
        if (cfd < 0) die("accept");

        char ip[64]; ip[0] = '\0';
        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        printf("[S] External connected from %s:%d (fd=%d)\n", ip, ntohs(cli.sin_port), cfd);
        client_fd[i] = cfd;
    }

    printf("-------------------------------------------------------------\n");
    printf("[S] All four external processes are now connected.\n");
    printf("-------------------------------------------------------------\n");

    *listen_fd_out = s;
    return client_fd;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <initial_central_temp>\n", argv[0]);
        return 1;
    }
    float central = strtof(argv[1], NULL);
    printf("[S] Initial central temperature = %.6f\n", central);

    int listen_fd = -1;
    int *client_fd = establishConnectionsFromExternalProcesses(&listen_fd);

    float ext[NUM_EXTERNALS]  = {0};
    float prev[NUM_EXTERNALS] = {INFINITY, INFINITY, INFINITY, INFINITY};
    int iterations = 0;

    for (;;) {
        // 1) Receive one temperature from each client
        for (int i = 0; i < NUM_EXTERNALS; i++) {
            struct msg m;
            if (!recv_msg(client_fd[i], &m)) {
                fprintf(stderr, "[S] recv_msg failed from fd %d\n", client_fd[i]);
                die("recv_msg");
            }
            if (m.Type != MSG_TEMP || m.Index < 1 || m.Index > 4) {
                fprintf(stderr, "[S] Bad message: Type=%d Index=%d fd=%d\n", m.Type, m.Index, client_fd[i]);
                exit(1);
            }
            ext[m.Index - 1] = m.T;
        }

        // 2) Update central temperature
        float sum = 0.0f;
        for (int i = 0; i < NUM_EXTERNALS; i++) sum += ext[i];
        central = (2.0f * central + sum) / 6.0f;
        iterations++;

        printf("[S] it=%d central=%.6f | ext=[%.6f %.6f %.6f %.6f]\n",
               iterations, central, ext[0], ext[1], ext[2], ext[3]);

        // 3) Convergence: compare current external temps to previous external temps
        bool stable = true;
        for (int i = 0; i < NUM_EXTERNALS; i++) {
            if (fabsf(ext[i] - prev[i]) >= EPS) { stable = false; break; }
        }

        // 4) Broadcast either next central (continue) or done (stop)
        for (int i = 0; i < NUM_EXTERNALS; i++) {
            struct msg out = prepare_message(0, central, stable ? MSG_DONE : MSG_TEMP);
            if (!send_msg(client_fd[i], &out)) {
                fprintf(stderr, "[S] send_msg failed to fd %d\n", client_fd[i]);
                die("send_msg");
            }
        }

        if (stable) {
            printf("[S] Stabilized at %.6f after %d iterations.\n", central, iterations);
            break;
        }

        // 5) ext -> prev for next iteration’s convergence check
        for (int i = 0; i < NUM_EXTERNALS; i++) prev[i] = ext[i];
    }

    // Close sockets
    for (int i = 0; i < NUM_EXTERNALS; i++) close(client_fd[i]);
    if (listen_fd >= 0) close(listen_fd);
    return 0;
}

