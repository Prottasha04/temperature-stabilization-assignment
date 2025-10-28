// tcp_client.c — external process (client)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include "utils.h"

#define HOST "127.0.0.1"
#define PORT 2000

static void die(const char *m){ perror(m); exit(1); }

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <id 1..4> <initial_external_temp>\n", argv[0]);
        return 1;
    }
    int id = atoi(argv[1]);
    if (id < 1 || id > 4) { fprintf(stderr, "id must be 1..4\n"); return 1; }
    float ext = strtof(argv[2], NULL);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) die("socket");

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &srv.sin_addr) != 1) die("inet_pton");

    if (connect(s, (struct sockaddr*)&srv, sizeof(srv)) < 0) die("connect");

    printf("[C%d] start ext=%.6f\n", id, ext);

    for (;;) {
        // 1) Send our current external temperature (ALWAYS use send_msg)
        struct msg out = prepare_message(id, ext, MSG_TEMP);
        if (!send_msg(s, &out)) die("send_msg");

        // 2) Receive server reply (ALWAYS use recv_msg)
        struct msg in;
        if (!recv_msg(s, &in)) {
            // If we get here, server closed or short read -> treat as fatal
            fprintf(stderr, "[C%d] recv_msg failed (server closed?)\n", id);
            die("recv_msg");
        }

        if (in.Type == MSG_DONE) {
            printf("[C%d] DONE final=%.6f\n", id, in.T);
            break; // exit loop
        }
        if (in.Type != MSG_TEMP) {
            fprintf(stderr, "[C%d] unexpected message type %d\n", id, in.Type);
            exit(1);
        }

        // 3) Update external based on central
        float central = in.T;
        ext = (3.0f * ext + 2.0f * central) / 5.0f;
        printf("[C%d] update ext=%.6f (central=%.6f)\n", id, ext, central);
        // loop continues: we’ll send again on next iteration
    }

    close(s);
    return 0;
}

