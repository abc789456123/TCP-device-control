#include <wiringPi.h>
#include <softPwm.h>
#include <string.h>
#include <stdio.h>

#define LED 0

int runCommand(const char *cmd) {
    pinMode(LED, OUTPUT);
    softPwmCreate(LED, 0, 255);

    int pwm_value;
    if (strcmp(cmd, "LED 0") == 0) pwm_value = 0;
    else if (strcmp(cmd, "LED 1") == 0) pwm_value = 50;
    else if (strcmp(cmd, "LED 2") == 0) pwm_value = 155;
    else if (strcmp(cmd, "LED 3") == 0) pwm_value = 255;
    else return -1;

    softPwmWrite(LED, pwm_value);
    return 0;
}