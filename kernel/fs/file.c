#include <basic.h>
#include <ng/fs/dentry.h>
#include <ng/fs/file.h>
#include <ng/fs/inode.h>
#include <ng/fs/types.h>
#include <ng/thread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

// Get a file from the running_process's fd table
struct file *get_file(int fd);
// Get a file from someone else's fd table (?)
struct file *get_file_from_process(int fd, struct process *process);

ssize_t default_read(struct file *file, char *buffer, size_t len)
{
    if (file->offset > file->inode->len)
        return 0;
    size_t to_read = umin(len, file->inode->len - file->offset);
    memcpy(buffer, PTR_ADD(file->inode->data, file->offset), to_read);
    file->offset += to_read;
    return to_read;
}

ssize_t default_write(struct file *file, const char *buffer, size_t len)
{
    if (!file->inode->data) {
        file->inode->data = malloc(1024);
        file->inode->capacity = 1024;
    }

    size_t final_len = file->offset + len;
    if (file->offset + len > file->inode->capacity) {
        size_t resized_len = final_len * 3 / 2;
        file->inode->data = realloc(file->inode->data, resized_len);
        file->inode->capacity = resized_len;
    }

    memcpy(PTR_ADD(file->inode->data, file->offset), buffer, len);
    file->offset += len;
    file->inode->len = umax(file->inode->len, final_len);
    return len;
}

off_t default_seek(struct file *file, off_t offset, int whence)
{
    off_t new_offset = file->offset;

    switch (whence) {
    case SEEK_SET:
        new_offset = offset;
        break;
    case SEEK_CUR:
        new_offset += offset;
        break;
    case SEEK_END:
        new_offset = file->inode->len + offset;
        break;
    default:
        return -EINVAL;
    }

    if (new_offset < 0)
        return -EINVAL;

    file->offset = new_offset;
    return new_offset;
}

struct file_operations default_file_ops = { 0 };

bool read_mode(struct file *file) { return file->flags & O_RDONLY; }

bool write_mode(struct file *file) { return file->flags & O_WRONLY; }

bool has_permission(struct inode *inode, int flags)
{
    // bootleg implies, truth table:
    // mode  flags allowed
    // 0     0     1
    // 0     1     0
    // 1     0     1
    // 1     1     1

    return (inode->mode & USR_READ || !(flags & O_RDONLY))
        && (inode->mode & USR_WRITE || !(flags & O_WRONLY));
}

bool write_permission(struct inode *i) { return !!(i->mode & USR_WRITE); }
bool read_permission(struct inode *i) { return !!(i->mode & USR_READ); }
bool execute_permission(struct inode *i) { return !!(i->mode & USR_EXEC); }

struct file *get_file(int fd)
{
    if (fd > running_process->n_files) {
        return NULL;
    }

    return running_process->files[fd];
}

static int expand_fds(int new_max)
{
    struct file **fds = running_process->files;

    int prev_max = running_process->n_files;
    if (new_max == 0)
        new_max = prev_max * 2;

    struct file **new_memory = zrealloc(fds, new_max * sizeof(struct file *));
    if (!new_memory)
        return -ENOMEM;
    running_process->files = new_memory;
    running_process->n_files = new_max;
    return 0;
}

int add_file(struct file *file)
{
    struct file **fds = running_process->files;
    int i;
    for (i = 0; i < running_process->n_files; i++) {
        if (!fds[i]) {
            fds[i] = file;
            return i;
        }
    }

    int err;
    if ((err = expand_fds(0)))
        return err;
    running_process->files[i] = file;
    return i;
}

int add_file_at(struct file *file, int at)
{
    struct file **fds = running_process->files;

    int err;
    if (at >= running_process->n_files && (err = expand_fds(at + 1)))
        return err;

    running_process->files[at] = file;
    return at;
}

struct file *p_remove_file(struct process *proc, int fd)
{
    struct file **fds = proc->files;

    struct file *file = fds[fd];

    fds[fd] = 0;
    return file;
}

struct file *remove_file(int fd) { return p_remove_file(running_process, fd); }

void close_all_files(struct process *proc)
{
    struct file *file;
    for (int i = 0; i < proc->n_files; i++) {
        if ((file = p_remove_file(proc, i)))
            close_file(file);
    }
    free(proc->files); // ?
}

struct file *clone_file(struct file *file)
{
    struct file *new = malloc(sizeof(struct file));
    *new = *file;
    open_file_clone(new);
    return new;
}

struct file **clone_all_files(struct process *proc)
{
    struct file **fds = proc->files;
    size_t n_fds = proc->n_files;
    if (n_fds == 0 || n_fds > 1000000) {
        printf("process %i has %li fds\n", proc->pid, n_fds);
        return NULL;
    }
    struct file **newfds = calloc(n_fds, sizeof(struct file *));
    for (int i = 0; i < n_fds; i++) {
        if (fds[i])
            newfds[i] = clone_file(fds[i]);
    }
    return newfds;
}

ssize_t read_file(struct file *file, char *buffer, size_t len)
{
    if (!read_mode(file))
        return -EPERM;

    if (file->ops->read)
        return file->ops->read(file, buffer, len);
    else
        return default_read(file, buffer, len);
}

ssize_t write_file(struct file *file, const char *buffer, size_t len)
{
    if (!write_mode(file))
        return -EPERM;

    if (file->ops->write)
        return file->ops->write(file, buffer, len);
    else
        return default_write(file, buffer, len);
}

int ioctl_file(struct file *file, int request, void *argp)
{
    if (file->ops->ioctl)
        return file->ops->ioctl(file, request, argp);
    else {
        if (request == TTY_ISTTY)
            return 0;
        return -ENOTTY;
    }
}

off_t seek_file(struct file *file, off_t offset, int whence)
{
    if (file->ops->seek)
        return file->ops->seek(file, offset, whence);
    else
        return default_seek(file, offset, whence);
}

ssize_t getdents_file(struct file *file, struct ng_dirent *dents, size_t len)
{
    if (file->ops->getdents) {
        return file->ops->getdents(file, dents, len);
    } else {
        size_t index = 0;
        list_for_each (
            struct dentry, d, &file->dentry->children, children_node) {
            if (!d->inode) {
                continue;
            }
            strncpy(dents[index].name, d->name, 128);
            dents[index].type = d->inode->type;
            dents[index].mode = d->inode->mode;
            index += 1;

            if (index == len) {
                break;
            }
        }
        return index;
    }
}
