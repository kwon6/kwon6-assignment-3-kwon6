#include "systemcalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int return_value = system(cmd);

    if (0 == return_value) {
        return true;
    } else {
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    va_end(args);

	pid_t pid = fork(); // 创建子进程

    if (pid == -1) {
        // fork失败
        perror("fork failed");
        return 0; // 返回false
    } else if (pid == 0) {
        // 子进程
        execv(command[0], command); // 执行命令
        // 如果execv返回，说明执行失败
        perror("execv failed");
        exit(EXIT_FAILURE); // 子进程退出
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0); // 等待子进程结束

        if (WIFEXITED(status)) {
            // 子进程正常结束
            return WEXITSTATUS(status) == 0 ? 1 : 0; // 如果退出状态为0，则返回true，否则返回false
        } else {
            // 子进程异常结束
            return 0; // 返回false
        }
	}
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);
    // 执行命令
    pid_t pid = fork();
    if (pid == -1) {
        // fork失败
        perror("fork failed");
        return false;
    } else if (pid == 0) {
        // 子进程, 需要在fork()调用前执行打开文件操作吗？为什么？
        // 打开文件output.txt用于写入，如果文件不存在则创建它
        int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open failed");
            exit(EXIT_FAILURE);
        }

        // 重定向标准输出到文件
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 failed");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd); // 关闭文件描述符，因为已经使用dup2复制了

        // 执行命令
        execv(command[0], command);
        // 如果execvp返回，说明执行失败
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0); // 等待子进程结束

        if (WIFEXITED(status)) {
            // 子进程正常结束
            return WEXITSTATUS(status) == 0; // 如果退出状态为0，则返回true
        } else {
            // 子进程异常结束
            return false;
        }
    }
    return true;
}
