
#include <ng/basic.h>
#include <ng/fs.h>
#include <ng/malloc.h>
#include <ng/print.h>
#include <ng/syscall.h>
#include <ng/thread.h>
#include <ng/vmm.h>
#include <ds/dmgr.h>
#include <ds/ringbuf.h>
#include <ds/vector.h>
#include <fs/tarfs.h>
#include <stddef.h>
#include <stdint.h>
#include "char_devices.h"
#include "membuf.h"

struct dmgr fs_node_table = {0};

extern struct tar_header *initfs;

static struct fs_node *fs_node_region = NULL;
static struct list fs_node_free_list = {.head = NULL, .tail = NULL};

struct fs_node *new_file_slot() {
        if (fs_node_free_list.head) {
                return list_pop_front(&fs_node_free_list);
        } else {
                return fs_node_region++;
        }
}

void free_file_slot(struct fs_node *defunct) {
        list_prepend(&fs_node_free_list, defunct);
}

// should these not be staic?
struct fs_node *fs_root_node = &(struct fs_node) {
        .filetype = DIRECTORY,
        .filename = "",
        .permission = USR_READ | USR_WRITE,
        .uid = 0,
        .gid = 0,
};

struct fs_node *dev_zero = &(struct fs_node){0};
struct fs_node *dev_serial = &(struct fs_node){0};

struct open_fd *dev_stdin = &(struct open_fd){0};
struct open_fd *dev_stdout = &(struct open_fd){0};
struct open_fd *dev_stderr = &(struct open_fd){0};

#define FS_NODE_BOILER(fd, perm) \
        struct open_fd *ofd = dmgr_get(&running_process->fds, fd); \
        if (ofd == NULL) { RETURN_ERROR(EBADF); } \
        if (!(ofd->flags & perm)) { RETURN_ERROR(EPERM); } \
        struct fs_node *node = ofd->node;

struct fs_node *find_fs_node_child(struct fs_node *node, const char *filename) {
        struct list_n *chld_list = node->extra.children.head;
        struct fs_node *child;

        printf("trying to find '%s' in '%s'\n", filename, node->filename);

        for (; chld_list; chld_list = chld_list->next) {
                child = chld_list->v;
                if (strcmp(child->filename, filename) == 0) {
                        return child;
                }
        }

        return NULL;
}

struct fs_node *get_file_by_name(struct fs_node *root, char *filename) {
        struct fs_node *node = root;

        char name_buf[256];

        while (filename && node) {
                filename = str_until(filename, name_buf, "/");

                if (strlen(name_buf) == 0) {
                        continue;
                }
                printf("sub: '%s' / '%s'\n", name_buf, filename);
                
                node = find_fs_node_child(node, name_buf);
                //printf("node = '%s'\n", node->filename);
        }
        
        return node;
}

sysret sys_open(char *filename, int flags) {
        struct fs_node *node = get_file_by_name(fs_root_node, filename);

        if (!node)
                return error(ENOENT);

        if (!(flags & O_RDONLY && node->permission & USR_READ))
                return error(EPERM);

        if (!(flags & O_WRONLY && node->permission & USR_WRITE))
                return error(EPERM);

        struct open_fd *new_open_fd = zmalloc(sizeof(struct open_fd));
        new_open_fd->node = node;
        node->refcnt++;
        if (flags & O_RDONLY)
                new_open_fd->flags |= USR_READ;
        if (flags & O_WRONLY)
                new_open_fd->flags |= USR_WRITE;
        new_open_fd->off = 0;

        size_t new_fd = dmgr_insert(&running_process->fds, new_open_fd);

        return value(new_fd);
}

sysret sys_read(int fd, void *data, size_t len) {
        FS_NODE_BOILER(fd, USR_READ);

        ssize_t value;
        while ((value = node->ops.read(ofd, data, len)) == -1) {
                if (node->flags & FILE_NONBLOCKING)
                        RETURN_ERROR(EWOULDBLOCK);

                block_thread(&node->blocked_threads);
        }
        RETURN_VALUE(value);
}

sysret sys_write(int fd, const void *data, size_t len) {
        FS_NODE_BOILER(fd, USR_WRITE);

        len = node->ops.write(ofd, data, len);
        RETURN_VALUE(len);
}

sysret sys_dup2(int oldfd, int newfd) {
        struct open_fd *ofd = dmgr_get(&running_process->fds, oldfd);
        RETURN_ERROR(ETODO);

        RETURN_VALUE(newfd);
}

sysret sys_seek(int fd, off_t offset, int whence) {
        if (whence > SEEK_END || whence < SEEK_SET) {
                RETURN_ERROR(EINVAL);
        }

        struct open_fd *ofd = dmgr_get(&running_process->fds, fd);
        if (ofd == NULL) {
                RETURN_ERROR(EBADF);
        }

        struct fs_node *node = ofd->node;
        if (!node->ops.seek) {
                RETURN_ERROR(EINVAL);
        }

        off_t old_off = ofd->off;

        node->ops.seek(ofd, offset, whence);

        if (ofd->off < 0) {
                ofd->off = old_off;
                RETURN_ERROR(EINVAL);
        }

        RETURN_VALUE(ofd->off);
}

sysret sys_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
        if (nfds < 0) {
                RETURN_ERROR(EINVAL);
        } else if (nfds == 0) {
                RETURN_VALUE(0);
        }

        for (int t = 0; t < timeout; t++) {
                for (int slow_down = 0; slow_down < 5000; slow_down++) {
                        for (int i = 0; i < nfds; i++) {
                                if (fds[i].fd < 0) {
                                        continue;
                                }

                                struct open_fd *ofd = dmgr_get(&running_process->fds, fds[i].fd);
                                if (ofd == NULL) {
                                        RETURN_ERROR(EBADF);
                                }
                                struct fs_node *node = ofd->node;

                                if (!node) {
                                        RETURN_ERROR(EBADF);
                                }

                                if (node->filetype != PTY) {
                                        RETURN_ERROR(ETODO);
                                }

                                if (node->extra.ring.len != 0) {
                                        fds[i].revents = POLLIN;
                                        RETURN_VALUE(1);
                                }
                        }
                }
        }

        RETURN_VALUE(0);
}

struct fs_node *make_dir(const char *name, struct fs_node *dir) {
        struct fs_node *new_dir = zmalloc(sizeof(struct fs_node));
        new_dir->filetype = DIRECTORY;
        strcpy(new_dir->filename, name);
        new_dir->permission = USR_READ | USR_WRITE;
        new_dir->parent = dir;

        return new_dir;
}

void put_file_in_dir(struct fs_node *file, struct fs_node *dir) {
        file->parent = dir;
        list_append(&dir->extra.children, file);
}

extern struct tar_header *initfs;

struct fs_node *make_tar_file(const char *name, size_t len, void *file) {
        struct fs_node *node = new_file_slot();
        strcpy(node->filename, name);
        node->filetype = MEMORY_BUFFER;
        node->permission = USR_READ;
        node->len = len;
        node->ops.read = membuf_read;
        node->ops.seek = membuf_seek;
        node->extra.memory = file;
        return node;
}

void vfs_init() {
        fs_root_node->parent = fs_root_node;

        struct fs_node *dev = make_dir("dev", fs_root_node);
        struct fs_node *bin = make_dir("bin", fs_root_node);
        put_file_in_dir(dev, fs_root_node);
        put_file_in_dir(bin, fs_root_node);

        fs_node_region = vmm_reserve(20 * 1024);

        // make all the tarfs files into fs_nodes and put into directories

        dev_zero->ops.read = dev_zero_read;
        strcpy(dev_zero->filename, "zero");

        put_file_in_dir(dev_zero, dev);

        dev_serial->ops.write = serial_write;
        dev_serial->ops.read = file_buf_read;
        dev_serial->filetype = PTY;
        emplace_ring(&dev_serial->extra.ring, 128);
        strcpy(dev_serial->filename, "serial");

        put_file_in_dir(dev_serial, dev);

        struct tar_header *tar = initfs;
        while (tar->filename[0]) {
                size_t len = tar_convert_number(tar->size);

                printf("making '/bin/%s'\n", tar->filename);

                char *filename = tar->filename;
                void *file_content = ((char *)tar) + 512;
                struct fs_node *new_node = make_tar_file(filename, len, file_content);
                put_file_in_dir(new_node, bin);


                uintptr_t next_tar = (uintptr_t)tar;
                next_tar += len + 0x200;
                next_tar = round_up(next_tar, 512);

                tar = (void *)next_tar;
        }
}

void vfs_print_tree(struct fs_node *root, int indent) {
        for (int i=0; i<indent; i++) {
                printf("  ");
        }
        printf("+ '%s'\n", root->filename);
        if (root->filetype == DIRECTORY) {
                struct list_n *ch = root->extra.children.head;
                if (!ch)
                        printf("CH IS NULL\n");
                for (; ch; ch = ch->next) {
                        vfs_print_tree(ch->v, indent + 1);
                }
        }
}

