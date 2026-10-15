#include "kernel/types.h"
#include "user/user.h"

int readline(int fd, char *buf, int max) {
    int i = 0;
    char c;
    int n;

    while (i < max - 1) {
        n = read(fd, &c, 1);
        if (n < 0)
            return -1;
        if (n == 0)
            break;          // EOF
        if (c == '\n')
            break;
        buf[i++] = c;
    }

    buf[i] = 0;

    if (i == 0 && n == 0)
        return 0;

    return i;
}

int main(int argc, char *argv[]) {
    int flag_q = 0;
    char *file1 = 0, *file2 = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0)
            flag_q = 1;
        else if (file1 == 0)
            file1 = argv[i];
        else
            file2 = argv[i];
    }

    if (file1 == 0 || file2 == 0) {
        fprintf(2, "Usage: diff file1 file2 [-q]\n");
        exit(1);
    }

    int fd1 = open(file1, 0);
    if (fd1 < 0) {
        fprintf(2, "diff: cannot open %s\n", file1);
        exit(1);
    }

    int fd2 = open(file2, 0);
    if (fd2 < 0) {
        fprintf(2, "diff: cannot open %s\n", file2);
        close(fd1);
        exit(1);
    }

    char buf1[1024], buf2[1024];
    int line = 0;
    int different = 0;

    while (1) {
        int r1 = readline(fd1, buf1, sizeof(buf1));
        int r2 = readline(fd2, buf2, sizeof(buf2));

        if (r1 == 0 && r2 == 0)
            break;

        line++;

        char *s1 = (r1 == 0) ? 0 : buf1;
        char *s2 = (r2 == 0) ? 0 : buf2;

        int same = (s1 && s2 && strcmp(s1, s2) == 0);

        if (!same) {
            different = 1;
            if (flag_q) {
                break;
            }
            if (s1)
                printf("%s:%d: < %s\n", file1, line, s1);
            else
                printf("%s:%d: < EOF\n", file1, line);

            if (s2)
                printf("%s:%d: > %s\n", file2, line, s2);
            else
                printf("%s:%d: > EOF\n", file2, line);
        }

        if (r1 == 0 && r2 != 0) {
            while ((r2 = readline(fd2, buf2, sizeof(buf2))) != 0) {
                line++;
                different = 1;
                if (!flag_q)
                    printf("%s:%d: < EOF\n%s:%d: > %s\n", file1, line, file2, line, buf2);
            }
            break;
        }
        if (r2 == 0 && r1 != 0) {
            while ((r1 = readline(fd1, buf1, sizeof(buf1))) != 0) {
                line++;
                different = 1;
                if (!flag_q)
                    printf("%s:%d: < %s\n%s:%d: > EOF\n", file1, line, buf1, file2, line);
            }
            break;
        }
    }

    if (flag_q && different)
        printf("diff: files differ\n");

    close(fd1);
    close(fd2);
    exit(0);
}

