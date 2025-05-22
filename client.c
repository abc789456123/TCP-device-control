#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define TCP_PORT 5100

void show_menu() {
    puts("=====메뉴=====");
    puts("1. LED 제어");
    puts("2. 조도 센서 제어");
    puts("3. 부저 제어");
    puts("4. 7세그먼트 제어");
    puts("5. 프로그램 종료");
    printf("원하는 동작을 선택하세요(1~5) : ");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage : %s <IP_ADDRESS>\n", argv[0]);
        return -1;
    }

    int choice;
    char input[256], mesg[256];
    struct sockaddr_in servaddr;

    while (1) {
        show_menu();
        if (scanf("%d", &choice) != 1) break;
        getchar();

        if (choice == 5) break;

        memset(input, 0, sizeof(input));
        memset(mesg, 0, sizeof(mesg));

        switch (choice) {
            case 1:
                printf("밝기 선택 (0: OFF, 1: 낮음, 2: 중간, 3: 최대): ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                snprintf(mesg, sizeof(mesg), "LED %.240s", input);
                break;
            case 2:
                strncpy(mesg, "CDS START", sizeof(mesg));
                break;
            case 3:
                strncpy(mesg, "BUZZER PLAY", sizeof(mesg));
                break;
            case 4:
                printf("0~9 숫자 입력: ");
                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = 0;
                snprintf(mesg, sizeof(mesg), "FND %.240s", input);
                break;
            default:
                puts("잘못된 선택입니다.");
                continue;
        }

        printf("[DEBUG] 명령 준비됨: %s\n", mesg);

        int ssock = socket(AF_INET, SOCK_STREAM, 0);
        if (ssock < 0) {
            perror("socket()");
            continue;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
        servaddr.sin_port = htons(TCP_PORT);

        printf("[DEBUG] 서버에 연결 시도 중...\n");
        if (connect(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("connect()");
            close(ssock);
            continue;
        }
        printf("[DEBUG] 서버 연결 성공\n");

        send(ssock, mesg, strlen(mesg), 0);
        printf("[DEBUG] 명령 전송 완료\n");

        if (choice == 2 || choice == 3) {
            puts(choice == 2 ? "조도센서 실행 중... 종료하려면 q 입력" : "부저 제어 중... 명령 입력 가능");
            fd_set readfds;
            int maxfd = ssock > STDIN_FILENO ? ssock : STDIN_FILENO;
            while (1) {
                FD_ZERO(&readfds);
                FD_SET(ssock, &readfds);
                FD_SET(STDIN_FILENO, &readfds);
                select(maxfd + 1, &readfds, NULL, NULL, NULL);

                if (FD_ISSET(ssock, &readfds)) {
                    int len = read(ssock, mesg, sizeof(mesg) - 1);
                    if (len <= 0) break;
                    mesg[len] = 0;
                    printf("[DEBUG] 서버 응답:\n%s종료하려면 q 입력\n", mesg);
                    if (strstr(mesg, "종료")) break;
                }

                if (FD_ISSET(STDIN_FILENO, &readfds)) {
                    char key = getchar();
                    send(ssock, &key, 1, 0);
                    if (key == '2') {
                        printf("[DEBUG] 종료 명령 전송함\n");
                    }
                }
            }
        } else {
            printf("[DEBUG] 응답 수신 대기 중...\n");
            int len = recv(ssock, mesg, sizeof(mesg) - 1, 0);
            if (len > 0) {
                mesg[len] = 0;
                printf("서버 응답: %s\n", mesg);
            } else {
                printf("[DEBUG] 응답 수신 실패 또는 없음 (len=%d)\n", len);
            }
        }

        close(ssock);
        printf("[DEBUG] 연결 종료됨\n\n");
    }

    puts("프로그램을 종료합니다.");
    return 0;
}
