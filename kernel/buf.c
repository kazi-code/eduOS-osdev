
#include <basic.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include "buf.h"

struct buf *new_buf(size_t size) {
    struct buf *buf = malloc(sizeof(struct buf));
    buf->data = malloc(size);
    buf->len = 0;
    return buf;
}

size_t buf_put(struct buf *buf, const void *data, size_t len) {
    size_t available = buf->size - buf->len;

    if (available == 0) {
        return 0;
    }

    memcpy(buf->data, data, available);
    return available;
}

size_t buf_get(struct buf *buf, void *data, size_t len) {
    size_t count = min(buf->len, len);

    memcpy(data, buf->data, count);
    memmove(buf->data, buf->data + count, buf->size - count);
    return count;
}

void del_buf(struct buf *buf) {
    free(buf->data);
    free(buf);
}

