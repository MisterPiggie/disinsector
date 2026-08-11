#include "tracer.h"
#include "communication.h"
#include "disinsect.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

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
        io_status status = io_read_data(debugger.main_to_tracer[0], &buffer, sizeof(buffer));
        if (status != IO_OK)
            _exit(TRACER_EXIT_RESUME_CLEAN);

        tracer_code_handler(buffer);
    }
}

void tracer_code_handler(uint8_t code)
{
    tracer_exit_codes exit_code;

    switch(code)
    {
        case CODE_BREAK:
        {
            if (tracer_stop_threads() != 0)
            {
                if (tracer_continue_threads() != 0)
                    exit_code = TRACER_EXIT_RESUME_FAIL;
                else 
                    exit_code = TRACER_EXIT_RESUME_CLEAN;

                io_status status = io_send_data(debugger.tracer_to_main[1], &exit_code, sizeof(exit_code));
                if (status != IO_OK)
                    _exit(exit_code);
            } else
            {
                exit_code = TRACER_OK;
                io_status status = io_send_data(debugger.tracer_to_main[1], &exit_code, sizeof(exit_code));
                if (status != IO_OK)
                {
                    exit_code = (tracer_continue_threads() == 0) ? TRACER_EXIT_RESUME_CLEAN : TRACER_EXIT_RESUME_FAIL;
                    _exit(exit_code);
                }
            }
            break;
        }
        case CODE_CONTINUE:
        {
            exit_code = (tracer_continue_threads() == 0) ? TRACER_OK : TRACER_EXIT_RESUME_FAIL;

            io_status status = io_send_data(debugger.tracer_to_main[1], &exit_code, sizeof(exit_code));
            if (status != IO_OK)
                _exit(exit_code == TRACER_OK ? TRACER_EXIT_RESUME_CLEAN : TRACER_EXIT_RESUME_FAIL);

            break;
        }
        case CODE_CHECKPOINT:
            if (!tracer_has_threads())
            {
                printf("Checkpoint is not supported with multithreaded apps\n\
                        Checkpoint was not created");
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
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (debugger.tids_count >= THREAD_MAX)
        {
            //TODO change max threads to dynamic
            closedir(dir);
            fprintf(stderr, "Max threads amount exceeded\n");
            return -1;
        }

        pid_t tid = atoi(entry->d_name);
        if (tid == debugger.caller_tid)
            continue;

        if (stop_thread(tid) != 0)
        {
            closedir(dir);
            return -1;
        }

        debugger.tids[debugger.tids_count++] = tid;
    }

    closedir(dir);

    return 0;
}

int stop_thread(pid_t tid)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) == -1)
    {
        fprintf(stderr, "PTRACE_ATTACH failed on thread with tid: %d: %s\n", tid, strerror(errno));
        return -1;
    }

    int status;
    pid_t result;

    do 
    {
        result = waitpid(tid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        fprintf(stderr, "waitpid failed on thread with tid: %d: %s\n", tid, strerror(errno));
        return -1;
    }

    if (!WIFSTOPPED(status))
    {
        if (WIFEXITED(status))
            fprintf(stderr, "Thread %d exited before it could be stopped; exit code %d\n", tid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            fprintf(stderr, "Thread %d was killed by signal %d before it could be stopped\n", tid, WTERMSIG(status));
        else
            fprintf(stderr, "Unexpected waitpid status 0x%x on thread %d\n", status, tid);
       
        return -1;
    }

    return 0;
}


int tracer_continue_threads(void)
{
    bool any_failed = false;
    for (int i = 0; i < debugger.tids_count; i++) {
        pid_t tid = debugger.tids[i];
        if (continue_thread(tid) != 0)
            any_failed = true;

    }
    debugger.tids_count = 0;
    return any_failed ? -1 : 0;
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

void tracer_exit(void)
{
    tracer_exit_codes exit_code = TRACER_EXIT_RESUME_CLEAN;
    if (debugger.tids_count != 0)
        if (tracer_continue_threads() != 0)
            exit_code = TRACER_EXIT_RESUME_FAIL;

    io_send_data(debugger.tracer_to_main[1], &exit_code, sizeof(exit_code));
    _exit(exit_code);
}
