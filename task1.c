#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int my_number = 0;
int my_result = 0;

void rt_handler(int sig, siginfo_t *info, void *ucontext) {
    my_number = info->si_value.sival_int;
}

void usr_handler(int sig) {
    if (sig == SIGUSR1) my_result = 1;
    else my_result = 2;
}

int main(int argc, char *argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 100;
    if (N < 1) N = 100;

    struct sigaction sa;
    sa.sa_sigaction = rt_handler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN, &sa, NULL);

    sa.sa_handler = usr_handler;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    srand(time(NULL));

    for (int round = 0; round < 20; round++) {
        int parent_guesses = (round >= 10);

        if (pid == 0) {
            if (!parent_guesses) {
                while (my_number == 0) pause();
                int secret = my_number;
                my_number = 0;
                int low = 1, high = N, attempts = 0;

                while (1) {
                    int guess = (low + high) / 2;
                    attempts++;
                    union sigval val; val.sival_int = guess;
                    sigqueue(getppid(), SIGRTMIN, val);

                    while (my_result == 0) pause();
                    if (my_result == 1) {
                        printf("[Ребенок] Угадал %d за %d попыток.\n", secret, attempts);
                        my_result = 0;
                        break;
                    } else {
                        if (guess < secret) low = guess + 1;
                        else high = guess - 1;
                        my_result = 0;
                    }
                }
            } else {
                int secret = rand() % N + 1;
                union sigval val; val.sival_int = secret;
                sigqueue(getppid(), SIGRTMIN, val);

                while (1) {
                    while (my_number == 0) pause();
                    int guess = my_number;
                    my_number = 0;
                    if (guess == secret) {
                        kill(getppid(), SIGUSR1);
                        printf("[Ребенок] Загадал %d, родитель угадал.\n", secret);
                        break;
                    } else {
                        kill(getppid(), SIGUSR2);
                    }
                }
            }
        } else {
            if (!parent_guesses) {
                int secret = rand() % N + 1;
                union sigval val; val.sival_int = secret;
                sigqueue(pid, SIGRTMIN, val);

                while (1) {
                    while (my_number == 0) pause();
                    int guess = my_number;
                    my_number = 0;
                    if (guess == secret) {
                        kill(pid, SIGUSR1);
                        printf("[Родитель] Загадал %d, ребенок угадал.\n", secret);
                        break;
                    } else {
                        kill(pid, SIGUSR2);
                    }
                }
            } else {
                while (my_number == 0) pause();
                int secret = my_number;
                my_number = 0;
                int low = 1, high = N, attempts = 0;

                while (1) {
                    int guess = (low + high) / 2;
                    attempts++;
                    union sigval val; val.sival_int = guess;
                    sigqueue(pid, SIGRTMIN, val);

                    while (my_result == 0) pause();
                    if (my_result == 1) {
                        printf("[Родитель] Угадал %d за %d попыток.\n", secret, attempts);
                        my_result = 0;
                        break;
                    } else {
                        if (guess < secret) low = guess + 1;
                        else high = guess - 1;
                        my_result = 0;
                    }
                }
            }
        }
    }

    if (pid > 0) {
        kill(pid, SIGTERM);
        wait(NULL);
    }
    return 0;
}