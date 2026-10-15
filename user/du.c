#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// Flags
static int flag_a = 0;  // -a: print files too
static int flag_s = 0;  // -s: summary only

// Print size and path
void print_entry(long size, char *path) {
    printf("%ld\t%s\n", size, path);
}

// Recursive du: returns total bytes under path
long du(char *path) {
    struct stat st;
    int fd;
    long total = 0;

    fd = open(path, 0);
    if (fd < 0) {
        fprintf(2, "du: cannot open %s\n", path);
        return 0;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "du: cannot stat %s\n", path);
        close(fd);
        return 0;
    }

    if (st.type == T_FILE) {
        close(fd);
        return st.size;
    }

    if (st.type == T_DIR) {
        char buf[512];
        struct dirent de;

        int plen = strlen(path);

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;
            // Skip . and ..
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            // Build full child path
            if (plen + 1 + DIRSIZ + 1 > sizeof(buf)) {
                fprintf(2, "du: path too long\n");
                continue;
            }
            memmove(buf, path, plen);
            buf[plen] = '/';
            memmove(buf + plen + 1, de.name, DIRSIZ);
            buf[plen + 1 + DIRSIZ] = 0;

            struct stat cst;
            int cfd = open(buf, 0);
            if (cfd < 0) {
                fprintf(2, "du: cannot open %s\n", buf);
                continue;
            }
            if (fstat(cfd, &cst) < 0) {
                fprintf(2, "du: cannot stat %s\n", buf);
                close(cfd);
                continue;
            }
            close(cfd);

            if (cst.type == T_DIR) {
                long sub = du(buf);
                total += sub;
                // Print subdir post-order (after recursion)
                if (!flag_s)
                    print_entry(sub, buf);
            } else if (cst.type == T_FILE) {
                total += cst.size;
                if (flag_a && !flag_s)
                    print_entry(cst.size, buf);
            }
        }

        close(fd);
        return total;
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    char *path = ".";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0)
            flag_a = 1;
        else if (strcmp(argv[i], "-s") == 0)
            flag_s = 1;
        else
            path = argv[i];
    }

    long total = du(path);

    // Print root path last (all subdirs already printed inside du())
    print_entry(total, path);

    exit(0);
}
