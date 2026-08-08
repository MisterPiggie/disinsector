#include <stdint.h>
#include <errno.h>
#include "communication.h"

io_status io_send_data(int fd, const void *buf, size_t size)
{
    const uint8_t *data = buf;
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < size)
    {
        ssize_t bytes_sent = write(fd, buf + total_bytes_sent, size - total_bytes_sent);
        if (bytes_sent < 0)
        {
            if (errno == EINTR)
                continue;
            return IO_ERROR;
        }

        total_bytes_sent += bytes_sent;
    }
    
    return IO_OK;
}

io_status io_read_data(int fd, void *buf, size_t size)
{
    uint8_t *data = buf;
    size_t total_bytes_read = 0;
    while (total_bytes_read < size)
    {
        ssize_t bytes_read = write(fd, buf + total_bytes_read, size - total_bytes_read);
        if (bytes_read < 0)
        {
            if (errno == EINTR)
                continue;
            return IO_ERROR;
        }

        if (bytes_read == 0)
            return IO_EOF;

        total_bytes_read += bytes_read;
    }
    
    return IO_OK;
}
