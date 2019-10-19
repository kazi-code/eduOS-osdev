
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int exec(char *const *argv) {
        pid_t child;

        child = fork();
        if (child == -1) {
                perror("fork()");
                return -1;
        }
        if (child == 0) {
                setpgid();
                execve(argv[0], argv + 1, NULL);

                // getting here constitutes failure
                switch (errno) {
                case ENOENT:
                        printf("%s does not exist\n", argv[0]);
                        break;
                case ENOEXEC:
                        printf(
                            "%s is not executable or is not a valid format\n",
                            argv[0]);
                        break;
                default:
                        printf("An unknown error occured running %s\n", argv[0]);
                        perror("execve()");
                }

                exit(127);
        } else {
                int return_code;
                if (waitpid(child, &return_code, 0) == -1) {
                        perror("waitpid()");
                        return -1;
                }
                return return_code;
        }
}

void clear_line(char *buf, long *ix) {
        while (*ix > 0) {
                *ix -= 1;
                buf[*ix] = '\0';
                printf("\x08 \x08");
        }
}

void backspace(char *buf, long *ix) {
        if (*ix == 0)
                return;
        *ix -= 1;
        buf[*ix] = '\0';
        printf("\x08 \x08");
}

void load_line(char *buf, long *ix, char *new_line) {
        clear_line(buf, ix);
        while (*new_line) {
                buf[*ix] = *new_line;
                printf("%c", *new_line);
                new_line += 1;
                *ix += 1;
        }
}

typedef struct hist hist;
struct hist {
        hist *previous;
        hist *next;
        char *history_line;
};

hist hist_base = {0};
hist *hist_top = 0;

void store_history_line(char *line_to_store, long len, hist *node) {
        char *line = malloc(len + 5);
        strcpy(line, line_to_store);
        node->history_line = line;

        hist_top = node;
}

void load_history_line(char *buf, long *ix, hist *current) {
        clear_line(buf, ix);
        if (!current->history_line)
                return;
        load_line(buf, ix, current->history_line);
}

long read_line(char *buf, size_t max_len) {
        long ix = 0;
        int readlen = 0;
        char cb[256] = {0};

        if (!hist_top)
                hist_top = &hist_base;

        hist *this_node = malloc(sizeof(hist));
        hist *current = this_node;
        hist_top->next = current;
        current->previous = hist_top;
        current->history_line = "";

        while (true) {
                memset(cb, 0, 256);
                readlen = read(stdin_fd, cb, 256);
                if (readlen == -1) {
                        perror("read()");
                        return -1;
                }

                if (cb[0] == '\x1b') {
                esc_seq:
                        if (strcmp(cb, "\x1b[A") == 0) { // up arrow
                                if (current->previous)
                                        current = current->previous;
                                load_history_line(buf, &ix, current);
                                continue;
                        } else if (strcmp(cb, "\x1b[B") == 0) { // down arrow
                                if (current->next)
                                        current = current->next;
                                load_history_line(buf, &ix, current);
                                continue;
                        } else {
                                if (strlen(cb) > 3) {
                                        printf("unknown escape-sequence %s\n",
                                               &cb[1]);
                                        continue;
                                }
                                read(stdin_fd, &cb[readlen++], 1);
                                goto esc_seq;
                        }
                }

                for (int i = 0; i < readlen; i++) {
                        char escape_status = 0;
                        char c = cb[i];

                        switch (c) {
                        case 0x7f: // backspace
                                backspace(buf, &ix);
                                continue;
                        case 0x0b: // ^K
                                clear_line(buf, &ix);
                                continue;
                        case 0x0c: // ^L
                                load_line(buf, &ix, "heapdbg both");
                                continue;
                        case 0x0e: // ^N
                                if (current->previous)
                                        current = current->previous;
                                load_history_line(buf, &ix, current);
                                continue;
                        case 0x08: // ^H
                                if (current->next)
                                        current = current->next;
                                load_history_line(buf, &ix, current);
                                continue;
                        case '\n':
                                goto done;
                        }

                        if (ix + 1 == max_len)
                                goto done; // continue;

                        if (!isprint(c)) {
                                printf("(%hhx)", c);
                                continue;
                        }

                        buf[ix++] = c;
                        buf[ix] = '\0';
                        cb[i] = 0;
                }
        }

done:
        if (ix > 0) {
                // printf("storing history\n");
                store_history_line(buf, ix, this_node);
        } else {
                free(this_node);
        }
        return ix;
}

long read_line_simple(char *buf, size_t limit) {
        if (!hist_top) {
                hist_top = &hist_base;
        }

        hist *this_node = malloc(sizeof(hist));
        hist *current = this_node;
        hist_top->next = current;
        current->previous = hist_top;
        current->history_line = "";

        int ix = read(stdin_fd, buf, limit);
        // EVIL HACK FIXME
        if (buf[ix-1] == '\n') {
                buf[ix-1] = '\0';
                ix -= 1;
        }

        if (ix > 0) {
                store_history_line(buf, ix, this_node);
        } else {
                free(this_node);
        }
        return ix;
}

int crash() {
        volatile char *x = 0;
        return *x;
}

int handle_one_line() {
        printf("$ ");

        char cmdline[256] = {0};
        char *args[32] = {0};

#ifdef USER_MODE_TTY
        if (read_line(cmdline, 256) == -1) {
                return 2;
        }
#else
        if (read_line_simple(cmdline, 256) == -1) {
                return 2;
        }
#endif

        char *c = cmdline;
        size_t arg = 0;

        bool was_space = true;
        bool is_space = false;
        int in_quote = '\0';
        while (*c != 0) {
                is_space = isblank(*c);
                if (!is_space && was_space) {
                        args[arg++] = c;
                } else if (is_space) {
                        *c = '\0';
                }

                was_space = is_space;
                c += 1;
        }

        args[arg] = NULL;

        if (cmdline[0] == 0)
                return 0;

        if (strncmp("history", cmdline, 7) == 0) {
                hist *hl = hist_top;
                for (; hl->history_line; hl = hl->previous) {
                        printf("%s\n", hl->history_line);
                }
                return 0;
        }

        if (strncmp("exit", cmdline, 4) == 0) {
                return 1;
        }

        if (strncmp("crash", cmdline, 5) == 0) {
                crash();
        }

        int ret_val = exec(args);

        printf("-> %i\n", ret_val);

        return 0;
}

int main() {
        printf("Nightingale shell\n");

        while (handle_one_line() == 0) {}

        return 0;
}

