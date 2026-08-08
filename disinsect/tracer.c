#include "tracer.h"
#include "disinsect.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>

/* 
 * this is the biggest source of problem need to undesrtand how to handle errors at different stages 
 * what to do what to sent what to do if sent failed 
 * right now design goes like this
 * if tracer successfuly stopped all of the threads or none were found send THREADS_OK (or however i named this code)
 * if tracer failed to stop some thread and succesfully resumed them send THREAD_FAILED_RESUMED
 * if tracer failed to stop some thread and faile to resume send THREAD_FAILED_TO_RESUME
 * if communication between tracer and main failed stop tracer right here and there with error code 1 (other exits return 0)
 * this is the whole logic for this part right now dont think about logic of main just work with tracer logic 
 * you dont care what main does with your error codes
 */

void tracer_loop(void)
{
    uint8_t buffer;

    while(1) {
        ssize_t bytes_read = read(debugger.main_to_tracer[0], &buffer, sizeof(buffer));
        if (bytes_read == 0)
            tracer_exit();
        else if (bytes_read == -1) {
            if (errno == EINTR)
                continue;
            else {
                send_to_main(TRACER_FAILED);
                if (errno == EBADF || errno == EINVAL) {  //test version might just return all of the time 
                    tracer_exit();
                    return;
                }
                continue; //might cahnge to return need to think about main loop
            }
        }

        tracer_code_handler(buffer);
    }
}

void tracer_code_handler(uint8_t code)
{
    //TODO this is where i need to count treads; Create this funcs
    bool has_threads = tracer_has_threads();
    switch(code)
    {
        case CODE_BREAK:
            if (!has_threads)
                break;
            tracer_break_threads();
            break;
        case CODE_CONTINUE:
            if (!has_threads)
                break;
            tracer_continue_threads();
            break;
        case CODE_CHECKPOINT:
            if (!has_threads)
            {
                perror("Checkpoint is not supported with multithreaded apps\n");
                break;
            }

            // tracer_checkpoint_make();
            break;
    }
}

int tracer_stop_threads(void)
{
    char path[64];

    snprintf(path, sizeof(path), "/proc/%d/task", debugger.main_pid);

    DIR *dir = opendir(path);

    if (!dir)
    {
        perror("No threads are found\n");
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (debugger.tids_count >= THREAD_MAX)
        {
            //TODO change max threads to dynamic
            perror("Max threads amount exceeded\n");
            return -1;
        }

        if (entry->d_name[0] == '.')
            continue;

        pid_t tid = atoi(entry->d_name);
        if (tid == debugger.main_pid)
            continue;

        if (stop_thread(tid) != 0)
            return -1;

        debugger.tids_count++;
    }

    closedir(dir);

    return 0;
}

int stop_thread(pid_t tid)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) == -1) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "PTRACE_ATTACH failed on thread with tid: %d", tid);

        perror(err_msg);
        return -1;
    }

    int status;
    pid_t result = waitpid(tid, &status, 0);

    if (result == -1) {
        if (errno != EINTR) {
            perror("waitpid failed");
            return -1;
        }

        bool is_success = false;
        
        for (int tries = 0; tries < 3; tries++) {
            result = waitpid(tid, &status, 0);
            if (result != -1) {
                is_success = true;
                break;
            }

            if (errno != EINTR) {
                perror("waitpid failed");
                return -1;
            }
        }

        if (!is_success) {
            perror("waitpid failed");
            return -1;
        }
    }

    if (!WIFSTOPPED(status)) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "Thread %d exited before it could be stopped; exit code %d\n", tid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Thread %d was killed by signal %d before it could be stopped\n", tid, WTERMSIG(status));
        } else {
            fprintf(stderr, "Unexpected waitpid status 0x%x on thread %d\n", status, tid);
        }
        return -1;
    }

    return 0;
}

/* this is wrong remade using communication.h
void send_to_main(uint8_t code)
{
    ssize_t bytes_written;
    do {
        bytes_written = write(debugger.tracer_to_main[1], &code, sizeof(code));
    } while (bytes_written < 0 && errno == EINTR);

    if (bytes_written < 0) {
        perror("Error sending to main; Abort the process\n");
        exit(1);
    }
}
*/

bool tracer_has_threads(void)
{
    char path[64];
    int count = 0;

    snprintf(path, sizeof(path), "/proc/%d/task", debugger.main_pid);

    DIR *dir = opendir(path);

    if (!dir)
        return false;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < 2) {
        if (entry->d_name[0] == '.')
            continue;
        count++;
    }

    closedir(dir);

    return count >= 2;
}

void tracer_break_threads(void)
{
    uint8_t buffer;
    if(tracer_stop_threads() != 0)
        perror("Failed to stop threads\n");
}

int tracer_continue_threads(void)
{
    for (int i = 0; i < debugger.tids_count; i++) {
        pid_t tid = debugger.tids[i];
        if (continue_thread(tid) != 0)
            return -1;

    }

    return 0;
}

int continue_thread(pid_t tid)
{
    if (ptrace(PTRACE_DETACH, tid, NULL, 0) == -1) {
        if (errno == ESRCH) 
            return 0;

        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "PTRACE_DETACH failed on thread with id: %d", tid);

        perror(err_msg);
        return -1;
    }

    return 0;
}

void tracer_exit(void)
{
    bool exited_without_errors = true;
    for (int i = 0; i < debugger.tids_count; i++)
        if (continue_thread(debugger.tids[i]) != 0)
            exited_without_errors = false;

    if (exited_without_errors)
        exit(0);

    exit(1);
}


