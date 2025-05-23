#include <wiringPi.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define CDS 2
#define LED 0

int runCommandWithSocket(const char *cmd, int client_fd) {
    if (strcmp(cmd, "CDS START") != 0) return -1;

    pinMode(CDS, INPUT);
    pullUpDnControl(CDS, PUD_DOWN);
    pinMode(LED, OUTPUT);

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    int prev = -1;
    char sendbuf[64];
    char recvbuf[2] = {0};

    while (1) {
        int current = digitalRead(CDS);

        if (current != prev) {
            digitalWrite(LED, current == LOW ? HIGH : LOW);
            snprintf(sendbuf, sizeof(sendbuf), "CDS sensor %s -> LED %s\n종료하려면 q 입력\n",
                current == LOW ? "LOW" : "HIGH",
                current == LOW ? "ON" : "OFF");
            write(client_fd, sendbuf, strlen(sendbuf));
            prev = current;
        }

        if (read(client_fd, recvbuf, 1) > 0 && recvbuf[0] == 'q') {
            write(client_fd, "CDS 종료\n", 11);
            break;
        }

        delay(100);
    }

    digitalWrite(LED, LOW);
    return 0;
}
