#define _GNU_SOURCE
#include "disinsect.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

/* 
 * Dont think about tracer and its state or how it works inside here you just thik about main logic
 * after sending data to tracer you wait for response and act accordingly dont overthink it 
 * you can do it
 */

//Callback for getting bias of main app 
int dl_phdr_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (info->dlpi_name[0] == '\0') {
        uintptr_t *bias = (uintptr_t *)data;
        *bias = info->dlpi_addr;
        return 1;
    }

    return 0;
}

int DIS_insector_init(void)
{
    //getting bias
    dl_iterate_phdr(dl_phdr_callback, &debugger.main_load_bias);
    debugger.main_pid = gettid();

    //initing pipes 
    if (pipe(debugger.main_to_tracer) == -1)
    {
        perror("Failed to init pipe main_to_tracer\n");
        return -1;
    }
    if (pipe(debugger.tracer_to_main) == -1)
    {
        perror("Failed to init pipe main_to_tracer\n");
        return -1;
    }

    //forking an app
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Failed to create tracer; fork() call returned -1");
        return -1;
    }

    //splitting logic depending on child and parent roles
    if (pid == 0) {
        close(debugger.main_to_tracer[1]);
        close(debugger.tracer_to_main[0]);

        debugger.role = ROLE_TRACER;
        debugger.tracer_pid = getpid();
        tracer_loop();
    }

    close(debugger.main_to_tracer[0]);
    close(debugger.tracer_to_main[1]);

    debugger.role = ROLE_MAIN;
    debugger.tracer_pid = pid;
    debugger.tids_count = 0;

    return 0;
}



//REPL loop used as command interface
void main_repl(void)
{
    char line[256];
    while (1) {
        printf("dis> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = '\0';

        char *cmd = strtok(line, " ");
        if (!cmd)
            continue;

        if (strcmp(cmd, "continue"))
            //might need clean up
            break;
        else if (strcmp(cmd, "exit") == 0)
            //also migth need cleanup
            exit(0);
        else if (strcmp(cmd, "checkpoint"))
            //need arguments; look up how gdb does that
            continue;
        else if (strcmp(cmd, "print"))
            //need arguments; look up how gdb does that
            continue;
        else if (strcmp(cmd, "set"))
            //need arguments; also need to verify types
            continue;
        else if (strcmp(cmd, "help"))
            continue;
        else 
            //print about help command
            printf("Invalid command\n");


    }
}
 
/* 
 * THIS CODE IS AWFUL KILL URSELF PLEASE THANK YOU
 * TODO: REDO THIS USING communication.h for sending and reading 
 *       remake to handle errors gotten from tracer if any were recieved
 *
 *
 *
 * void send_to_tracer(uint8_t code)
 * {
 *     ssize_t bytes_written;
 *     do {
 *         bytes_written = write(debugger.main_to_tracer[1], &code, sizeof(code));
 *     } while (bytes_written < 0 && errno == EINTR);
 * 
 *     if (bytes_written <= 0) {
 *         perror("Error sending to tracer; Abort the process\n");
 *         exit(1);
 *   }
 * }

 * void read_from_tracer(void)
 * {
 *     ssize_t bytes_read;
 *     uint8_t code;
 * 
 *   do {
 *         bytes_read = read(debugger.main_to_tracer[1], &code, sizeof(code));
 *     } while (bytes_read < 0 && errno == EINTR);
 * 
 *     if (bytes_read < 0) {
 *         perror("Error reading from tracer; Abort the process\n");
 *         exit(1);
 *     }
 * }
 */

