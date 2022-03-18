#include <basic.h>
#include <assert.h>
#include <ng/string.h>
#include <ng/thread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ng/fs/dentry.h>
#include <ng/fs/file.h>
#include <ng/fs/file_system.h>
#include <ng/fs/inode.h>
#include <ng/fs/types.h>

struct dentry *global_root_dentry;

extern inline struct inode *dentry_inode(struct dentry *dentry);
extern inline struct file_system *dentry_file_system(struct dentry *dentry);

struct dentry *new_dentry()
{
    struct dentry *dentry = zmalloc(sizeof(struct dentry));
    list_init(&dentry->children);
    return dentry;
}

// Cache child Create/Find/Remove

struct dentry *add_child(
    struct dentry *dentry, const char *name, struct inode *inode)
{
    if (inode == NULL) {
        struct dentry *new = new_dentry();
        new->name = strdup(name);
        new->parent = dentry;
        new->inode = NULL;
        if (dentry->mounted_file_system)
            new->file_system = dentry->mounted_file_system;
        else
            new->file_system = dentry->file_system;
        list_append(&dentry->children, &new->children_node);
        return new;
    }
    struct dentry *child = find_child(dentry, name);
    if (child->inode) {
        // it already existed.
        return TO_ERROR(-EEXIST);
    }
    attach_inode(child, inode);
    return child;
}

struct dentry *find_child(struct dentry *dentry, const char *name)
{
    struct dentry *found = NULL;

    if (!dentry_inode(dentry))
        return TO_ERROR(-ENOENT);

    if (dentry_inode(dentry)->type != FT_DIRECTORY)
        return TO_ERROR(-ENOTDIR);

    list_for_each (struct dentry, d, &dentry->children, children_node) {
        if (strcmp(d->name, name) == 0) {
            found = d;
            break;
        }
    }

    if (found) {
        if (found->mounted_file_system)
            found = found->mounted_file_system->root;
        return found;
    }

    return add_child(dentry, name, NULL);
}

struct dentry *unlink_dentry(struct dentry *dentry)
{
    list_remove(&dentry->children_node);
    return dentry;
}

int attach_inode(struct dentry *dentry, struct inode *inode)
{
    if (dentry_inode(dentry))
        return -EEXIST;
    dentry->inode = inode;
    atomic_fetch_add(&inode->dentry_refcnt, 1);
    return 0;
}

int detach_inode(struct dentry *dentry)
{
    assert(dentry->inode);
    atomic_fetch_sub(&dentry->inode->dentry_refcnt, 1);
    // inode->close() or something
    // let it free itself if this is the last reference
    dentry->inode = NULL;
    return 0;
}

// Path resolution

struct dentry *resolve_path_from_loopck(
    struct dentry *cursor, const char *path, bool follow, int n_symlinks)
{
    struct inode *inode;
    char buffer[128] = { 0 };

    if (n_symlinks > 8)
        return TO_ERROR(-ELOOP);

    if (path[0] == '/') {
        cursor = running_process->root;
        path++;
        if (path[0] == 0) {
            return cursor;
        }
    }

    if (!cursor)
        cursor = global_root_dentry;

    assert(cursor);

    do {
        path = strccpy(buffer, path, '/');

        if (strcmp(buffer, "..") == 0) {
            cursor = cursor->parent;
        } else if (strcmp(buffer, ".") == 0) {
            continue;
        } else if (buffer[0] == 0) {
            continue;
        } else {
            cursor = find_child(cursor, buffer);
        }

        while (follow && !IS_ERROR(cursor) && (inode = dentry_inode(cursor))
            && inode->type == FT_SYMLINK) {
            cursor = resolve_path_from_loopck(
                cursor, inode->symlink_destination, true, n_symlinks + 1);
        }

        if (IS_ERROR(cursor))
            return cursor;
    } while (path[0] && cursor->inode);

    if (path[0] || !cursor)
        return TO_ERROR(-ENOENT);

    return cursor;
}

struct dentry *resolve_path_from(
    struct dentry *cursor, const char *path, bool follow)
{
    return resolve_path_from_loopck(cursor, path, follow, 0);
}

struct dentry *resolve_path(const char *path)
{
    return resolve_path_from(running_process->root, path, true);
}

struct dentry *resolve_atfd(int fd)
{
    struct dentry *root = running_process->root;

    if (fd == AT_FDCWD) {
        root = running_thread->cwd2;
    } else if (fd >= 0) {
        struct file *file = get_file(fd);
        if (!file)
            return TO_ERROR(-EBADF);
        root = file->dentry;
    }

    return root;
}

struct dentry *resolve_atpath(int fd, const char *path, bool follow)
{
    struct dentry *at = resolve_atfd(fd);
    if (IS_ERROR(at) || !path)
        return at;
    // if (path[0] == 0 && !(fd & AT_EMPTY_PATH)) // something
    struct dentry *dentry = resolve_path_from(at, path, follow);
    return dentry;
}

// Reverse path resolution

static char *pathname_rec(
    struct dentry *dentry, char *buffer, size_t len, bool root)
{
    if (dentry->parent == dentry) {
        buffer[0] = '/';
        buffer[1] = 0;
        return buffer + 1;
    }

    char *after = pathname_rec(dentry->parent, buffer, len, false);

    size_t rest = len - (after - buffer);
    char *next = memccpy(after, dentry->name, 0, rest);
    if (!root) {
        next[-1] = '/';
        next[0] = 0;
    }
    return next;
}

int pathname(struct file *file, char *buffer, size_t len)
{
    struct dentry *dentry = file->dentry;
    char *after = pathname_rec(dentry, buffer, len, true);
    return after - buffer;
}