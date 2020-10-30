#include <basic.h>
#include <ng/fs.h>
// #include <ng/fs/directory.h>
#include <ng/thread.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <list.h>

struct file_ops directory_ops = {
        .readdir = directory_readdir,
};

ssize_t directory_readdir(struct open_file *ofd, struct ng_dirent *buf, size_t count) {
        struct file *file = ofd->node;
        if (file->filetype != FT_DIRECTORY) {
                return -ENOTDIR;
        }
        struct directory_file *directory = (struct directory_file *)file;

        int index = 0;

        list_for_each(struct directory_node, node, &directory->entries, siblings) {
                if (index > count)  break;
                buf[index].type = node->file->filetype;
                buf[index].permissions = node->file->permissions;
                strncpy(&buf[index].filename[0], node->name, 64);
                index++;
        }

        return index;
}

struct file *__make_directory(struct directory_file *parent, struct directory_file *new, const char *name) {
        assert(new->file.filetype == FT_DIRECTORY);
        assert(parent->file.filetype == FT_DIRECTORY);

        struct directory_node *new_node = zmalloc(sizeof(struct directory_node));
        struct directory_node *new_dot = zmalloc(sizeof(struct directory_node));
        struct directory_node *new_dotdot = zmalloc(sizeof(struct directory_node));

        new->file.filetype = FT_DIRECTORY;
        new->file.permissions = USR_READ | USR_EXEC;
        new->file.refcnt = 1;
        new->file.ops = &directory_ops;
        list_init(&new->entries);
        list_init(&new->file.blocked_threads);

        // TODO: INCREF

        if (!(new == fs_root_node)) {
                new_node->name = name;
                new_node->file = &new->file;
                list_append(&parent->entries, &new_node->siblings);
        }

        new_dot->name = ".";
        new_dot->file = &new->file;
        list_append(&new->entries, &new_dot->siblings);

        new_dotdot->name = "..";
        new_dotdot->file = &parent->file;
        list_append(&new->entries, &new_dotdot->siblings);

        return &new->file;
}

struct file *make_directory(struct file *directory, const char *name) {
        assert(directory->filetype == FT_DIRECTORY);
        struct directory_file *dir = (struct directory_file *)directory;
        struct directory_file *new = zmalloc(sizeof(struct directory_file));
        new->file.filetype = FT_DIRECTORY;

        return __make_directory(dir, new, name);
}

struct file *make_directory_inplace(struct file *directory, struct file *new, const char *name) {
        assert(directory->filetype == FT_DIRECTORY);
        struct directory_file *dir = (struct directory_file *)directory;
        struct directory_file *new_dir = (struct directory_file *)new;
        new_dir->file.filetype = FT_DIRECTORY;

        return __make_directory(dir, new_dir, name);
}

void destroy_directory(struct file *directory) {
        assert(directory->filetype == FT_DIRECTORY);
        struct directory_file *dir = (struct directory_file *)directory;

        dir->file.refcnt--;

        list_for_each(struct directory_node, node, &dir->entries, siblings) {
                free(node);
        }
        list_init(&dir->entries);

        if (dir->file.refcnt <= 0) {
                // if dir was allocated!
        }
}

struct file *fs_root_init(void) {
        __make_directory(fs_root_node, fs_root_node, "");
        return &fs_root_node->file;
}

void add_dir_file(struct file *directory, struct file *file, const char *name) {
        assert(directory->filetype == FT_DIRECTORY);

        struct directory_file *dir = (struct directory_file *)directory;
        struct directory_node *new_node = zmalloc(sizeof(struct directory_node));

        new_node->name = name;
        new_node->file = file;
        list_append(&dir->entries, &new_node->siblings);
}

struct directory_node *__dir_child(struct file *directory, const char *name) {
        assert(directory->filetype == FT_DIRECTORY);
        struct directory_file *dir = (struct directory_file *)directory;

        if (list_empty(&dir->entries)) {
                return NULL;
        }

        list_for_each(struct directory_node, node, &dir->entries, siblings) {
                if (strcmp(name, node->name) == 0) {
                        return node;
                }
        }

        return NULL;
}

struct file *dir_child(struct file *directory, const char *name) {
        struct directory_node *node = __dir_child(directory, name);
        if (node) {
                return node->file;
        } else {
                return NULL;
        }
}
