#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ttyctl.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse.h"
#include "token.h"

int eval_pipeline(struct pipeline *pipeline)
{
    pid_t last_child = -1;
    assert(!list_empty(&pipeline->commands));
    int next_stdin_fd = 0;
    int next_stdout_fd = 1;
    int is_first = 1;
    list_for_each (struct command, c, &pipeline->commands, node) {
        int pipefds_is_valid = 0;
        int pipefds[2];

        if (strcmp(c->argv[0], "exit") == 0)
            exit(0);

        if (strcmp(c->argv[0], "cd") == 0) {
            const char *dir = c->argv[1];
            if (!dir || !dir[0])
                dir = "/bin";
            int err = chdir(dir);
            if (err < 0)
                perror("cd");
            continue;
        }

        if (c->node.next != &pipeline->commands) {
            // there's another command after this one
            pipe(pipefds);
            pipefds_is_valid = 1;
            if (next_stdout_fd > 1)
                close(next_stdout_fd);
            next_stdout_fd = pipefds[1];
        } else {
            if (next_stdout_fd > 1)
                close(next_stdout_fd);
            next_stdout_fd = 1;
        }

        pid_t pid;
        if ((pid = fork()) == 0) {
            pid = getpid();

            if (pipeline->pgrp) {
                setpgid(0, pipeline->pgrp);
            } else {
                setpgid(0, pid);
            }

            if (next_stdin_fd != STDIN_FILENO)
                dup2(next_stdin_fd, STDIN_FILENO);
            if (next_stdout_fd != STDOUT_FILENO)
                dup2(next_stdout_fd, STDOUT_FILENO);
            if (pipefds_is_valid) {
                close(pipefds[0]);
                close(pipefds[1]);
            }

            if (c->stdin_file) {
                int fd = open(c->stdin_file, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr, "Error opening %s", c->stdin_file);
                    perror("");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (c->stdout_file) {
                int fd
                    = open(c->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    fprintf(stderr, "Error opening %s", c->stdout_file);
                    perror("");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if (c->stderr_file) {
                int fd
                    = open(c->stderr_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                    fprintf(stderr, "Error opening %s", c->stderr_file);
                    perror("");
                    exit(1);
                }
                dup2(fd, STDERR_FILENO);
                close(fd);
            }

            char command[128] = { 0 };
            char *cmd = command;
            if (c->argv[0][0] == '/')
                cmd = c->argv[0];
            else
                snprintf(command, 127, "/bin/%s", c->argv[0]);

            int err = execve(cmd, c->argv, NULL);
            if (err < 0 && errno == ENOENT)
                fprintf(stderr, "%s: command not found\n", c->argv[0]);
            else if (err < 0)
                perror("execve");
            else
                fprintf(stderr,
                    "Tried to run %s, it failed but there was no error",
                    c->argv[0]);
            exit(126);
        } else {
            last_child = pid;
            if (pipeline->pgrp == 0) {
                pipeline->pgrp = pid;
                ttyctl(STDIN_FILENO, TTY_SETBUFFER, 1);
                ttyctl(STDIN_FILENO, TTY_SETECHO, 1);
                ttyctl(STDIN_FILENO, TTY_SETPGRP, pid);
            }
        }

        if (next_stdin_fd > 1)
            close(next_stdin_fd);
        next_stdin_fd = pipefds[0];
    }

    int status = 0;
    int pipeline_status = 0;
    errno = 0;
    while (errno != ECHILD) {
        pid_t pid = waitpid(-pipeline->pgrp, &status, 0);
        if (pid == last_child)
            pipeline_status = status;
    }
    return pipeline_status;
}

int eval(struct node *node)
{
    if (node->type == NODE_PIPELINE)
        return eval_pipeline(node->pipeline);
    if (node->type == NODE_BINOP) {
        int lhs = eval(node->left);
        if (node->op == NODE_AND && lhs != 0)
            return lhs;
        if (node->op == NODE_OR && lhs == 0)
            return lhs;
        return eval(node->right);
    }
    fprintf(stderr, "Error: cannot evaluate node of type %i\n", node->type);
    return -1;
}
