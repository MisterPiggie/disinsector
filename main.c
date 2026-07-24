#include "print.h"
#include <elfutils/libdw.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

int main(void)
{
    print_message("Hello, World\n");

    char exe_path[256];

    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror("readlink");
        return 1;
    }
    exe_path[len] = '\0';

    int fd = open(exe_path, O_RDONLY);
    Dwarf *dbg = dwarf_begin(fd, DWARF_C_READ);

    Dwarf_Off offset = 0, next_offset;
    size_t header_size;
    while (dwarf_nextcu(dbg, offset, &next_offset, &header_size, 0, 0, 0) == 0) {
        Dwarf_Die cu_die;
        dwarf_offdie(dbg, offset + header_size, &cu_die);
        printf("CU: %s\n", dwarf_diename(&cu_die));
        offset = next_offset;
    }

    dwarf_end(dbg);
    close(fd);
    return 0;
}
