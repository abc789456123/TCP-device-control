#include <wiringPi.h>
#include <softTone.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SPKR 5
int gpiopins[4] = {4, 1, 16, 15};
int number[10][4] = {
    {0,0,0,0}, {0,0,0,1}, {0,0,1,0}, {0,0,1,1},
    {0,1,0,0}, {0,1,0,1}, {0,1,1,0}, {0,1,1,1},
    {1,0,0,0}, {1,0,0,1}
};

int runCommand(const char *cmd) {
    if (strncmp(cmd, "FND ", 4) != 0) return -1;
    int num = atoi(cmd + 4);
    if (num < 0 || num > 9) return -1;

    softToneCreate(SPKR);
    for (int i = 0; i < 4; i++) pinMode(gpiopins[i], OUTPUT);

    for (int i = num; i >= 0; i--) {
        for (int j = 0; j < 4; j++)
            digitalWrite(gpiopins[j], number[i][j]);

        if (i == 0) {
            delay(200);
            softToneWrite(SPKR, 250);
            delay(1000);
            softToneWrite(SPKR, 0);
        } else {
            delay(1000);
        }
    }

    for (int i = 0; i < 4; i++)
        digitalWrite(gpiopins[i], HIGH);

    return 0;
}
