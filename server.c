#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <wiringPi.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>

#define TCP_PORT 5100
#define BUF_SIZE 128

// 공용 GPIO 자원 보호용 뮤텍스
pthread_mutex_t gpio17_mutex = PTHREAD_MUTEX_INITIALIZER; // LED (GPIO17)
pthread_mutex_t gpio24_mutex = PTHREAD_MUTEX_INITIALIZER; // BUZZER (GPIO24)
pthread_mutex_t gpio27_mutex = PTHREAD_MUTEX_INITIALIZER; // CDS (GPIO27)
pthread_mutex_t fnd_mutex     = PTHREAD_MUTEX_INITIALIZER; // FND 핀 그룹 보호

// 데몬 프로세스로 실행하기 위한 함수
void daemonize() {
    pid_t pid;
    struct rlimit rl;
    int fd0, fd1, fd2;

    umask(0);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) exit(1);
    if ((pid = fork()) < 0) exit(1);
    else if (pid != 0) exit(0);
    if (setsid() < 0) exit(1);
    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) < 0) exit(1);
    else if (pid != 0) exit(0);

    if (rl.rlim_max == RLIM_INFINITY) rl.rlim_max = 1024;
    for (int i = 0; i < rl.rlim_max; i++) close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog("tcp-server", LOG_CONS, LOG_DAEMON);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }

    syslog(LOG_INFO, "Daemon started");
}

void* load_library(const char* libname, const char* symbol, void** func_ptr) {
    void* handle = dlopen(libname, RTLD_LAZY);
    if (!handle) {
        syslog(LOG_ERR, "dlopen failed for %s: %s", libname, dlerror());
        return NULL;
    }
    *(void **)func_ptr = dlsym(handle, symbol);
    if (!*func_ptr) {
        syslog(LOG_ERR, "dlsym failed for symbol %s in %s: %s", symbol, libname, dlerror());
        dlclose(handle);
        return NULL;
    }
    return handle;
}

void* handle_client(void* arg) {
    int csock = *(int*)arg;
    free(arg);

    char mesg[BUF_SIZE];
    memset(mesg, 0, BUF_SIZE);

    if (read(csock, mesg, BUF_SIZE) <= 0) {
        close(csock);
        return NULL;
    }

    syslog(LOG_INFO, "Received command: %s", mesg);

    char libpath[256];
    void* handle = NULL;

    if (strncmp(mesg, "CDS", 3) == 0) {
        snprintf(libpath, sizeof(libpath), LIBDIR "/libcds.so");
        int (*func)(const char*, int) = NULL;
        handle = load_library(libpath, "runCommandWithSocket", (void**)&func);
        if (!handle || !func) {
            close(csock);
            return NULL;
        }
        pthread_mutex_lock(&gpio17_mutex);
        pthread_mutex_lock(&gpio27_mutex);
        func(mesg, csock);
        pthread_mutex_unlock(&gpio27_mutex);
        pthread_mutex_unlock(&gpio17_mutex);
    } else if (strncmp(mesg, "BUZZER", 6) == 0) {
        snprintf(libpath, sizeof(libpath), LIBDIR "/libbuzzer.so");
        int (*func)(const char*, int) = NULL;
        handle = load_library(libpath, "runCommandWithSocket", (void**)&func);
        if (!handle || !func) {
            close(csock);
            return NULL;
        }
        pthread_mutex_lock(&gpio24_mutex);
        func(mesg, csock);
        pthread_mutex_unlock(&gpio24_mutex);
    } else {
        int (*func)(const char*) = NULL;

        if (strncmp(mesg, "LED", 3) == 0) {
            snprintf(libpath, sizeof(libpath), LIBDIR "/libled.so");
            pthread_mutex_lock(&gpio17_mutex);
        } else if (strncmp(mesg, "FND", 3) == 0) {
            snprintf(libpath, sizeof(libpath), LIBDIR "/libfnd.so");
            pthread_mutex_lock(&gpio24_mutex);
            pthread_mutex_lock(&fnd_mutex);
        } else {
            write(csock, "Unknown Command\n", 17);
            close(csock);
            return NULL;
        }

        handle = load_library(libpath, "runCommand", (void**)&func);
        if (!handle || !func) {
            close(csock);
            return NULL;
        }

        int result = func(mesg);
        write(csock, result == 0 ? "OK\n" : "ERR\n", result == 0 ? 3 : 4);

        if (strncmp(mesg, "LED", 3) == 0) pthread_mutex_unlock(&gpio17_mutex);
        else if (strncmp(mesg, "FND", 3) == 0) {
            pthread_mutex_unlock(&fnd_mutex);
            pthread_mutex_unlock(&gpio24_mutex);
        }
    }

    if (handle) dlclose(handle);
    close(csock);
    return NULL;
}

int main() {
    daemonize();
    wiringPiSetup();

    int ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock < 0) {
        syslog(LOG_ERR, "socket() failed");
        exit(1);
    }

    struct sockaddr_in servaddr, cliaddr;
    socklen_t clen = sizeof(cliaddr);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        syslog(LOG_ERR, "bind() failed");
        exit(1);
    }

    if (listen(ssock, 8) < 0) {
        syslog(LOG_ERR, "listen() failed");
        exit(1);
    }

    while (1) {
        int* csock = malloc(sizeof(int));
        *csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (*csock < 0) {
            syslog(LOG_ERR, "accept() failed");
            free(csock);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, csock) != 0) {
            syslog(LOG_ERR, "pthread_create() failed");
            close(*csock);
            free(csock);
        } else {
            pthread_detach(tid);
        }
    }

    close(ssock);
    closelog();
    return 0;
}
