#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int N = (argc > 1) ? atoi(argv[1]) : 100;
    if (N < 1) N = 100;

    int p2c[2];
    int c2p[2];

    if (pipe(p2c) == -1 || pipe(c2p) == -1) {
        perror("pipe"); exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    srand(time(NULL));

    for (int round = 0; round < 20; round++) {
        int parent_guesses = (round >= 10);

        if (pid == 0) {
            close(p2c[1]);
            close(c2p[0]);
            int msg;

            if (!parent_guesses) {
                read(p2c[0], &msg, sizeof(int));
                int secret = msg;
                int low = 1, high = N, attempts = 0;

                while (1) {
                    int guess = (low + high) / 2;
                    attempts++;
                    write(c2p[1], &guess, sizeof(int));

                    read(p2c[0], &msg, sizeof(int));
                    if (msg == 1) {
                        printf("[Ребенок] Угадал %d за %d попыток.\n", secret, attempts);
                        break;
                    } else if (msg == 2) {
                        low = guess + 1;
                    } else {
                        high = guess - 1;
                    }
                }
            } else {
                int secret = rand() % N + 1;
                write(c2p[1], &secret, sizeof(int));

                while (1) {
                    read(p2c[0], &msg, sizeof(int));
                    int guess = msg;
                    int res;
                    if (guess == secret) {
                        res = 1;
                        write(c2p[1], &res, sizeof(int));
                        printf("[Ребенок] Загадал %d, родитель угадал.\n", secret);
                        break;
                    } else if (guess < secret) {
                        res = 2;
                        write(c2p[1], &res, sizeof(int));
                    } else {
                        res = 3;
                        write(c2p[1], &res, sizeof(int));
                    }
                }
            }
            close(p2c[0]);
            close(c2p[1]);
            exit(0);

        } else {
            close(p2c[0]);
            close(c2p[1]);
            int msg;

            if (!parent_guesses) {
                int secret = rand() % N + 1;
                write(p2c[1], &secret, sizeof(int));

                while (1) {
                    read(c2p[0], &msg, sizeof(int));
                    int guess = msg;
                    int res;
                    if (guess == secret) {
                        res = 1;
                        write(p2c[1], &res, sizeof(int));
                        printf("[Родитель] Загадал %d, ребенок угадал.\n", secret);
                        break;
                    } else if (guess < secret) {
                        res = 2;
                        write(p2c[1], &res, sizeof(int));
                    } else {
                        res = 3;
                        write(p2c[1], &res, sizeof(int));
                    }
                }
            } else {
                read(c2p[0], &msg, sizeof(int));
                int secret = msg;
                int low = 1, high = N, attempts = 0;

                while (1) {
                    int guess = (low + high) / 2;
                    attempts++;
                    write(p2c[1], &guess, sizeof(int));

                    read(c2p[0], &msg, sizeof(int));
                    if (msg == 1) {
                        printf("[Родитель] Угадал %d за %d попыток.\n", secret, attempts);
                        break;
                    } else if (msg == 2) {
                        low = guess + 1;
                    } else {
                        high = guess - 1;
                    }
                }
            }
            close(p2c[1]);
            close(c2p[0]);
        }
    }

    if (pid > 0) wait(NULL);
    return 0;
}