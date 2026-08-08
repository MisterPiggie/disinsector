#include <unistd.h>

typedef enum 
{
    IO_OK,
    IO_EOF,
    IO_ERROR,
} io_status;

io_status io_send_data(int fd, const void *buf, size_t size);
io_status io_read_data(int fd, void *buf, size_t size);
