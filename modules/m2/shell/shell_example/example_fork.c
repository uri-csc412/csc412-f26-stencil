#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("Hi I am in the parent process.\n");
    int x = 5;
    pid_t child;
    // fork returns 0 to the child process
    // and the child's process id (pid_t) to the parent process
    if ((child = fork()) == 0) {
        // only the child process can reach within this if statement //
        x = x + 1;
        printf("I am in the child process: x is %d.\n", x);
    }
    printf("I am outside the if statement.\n");
    printf("Child is %d and x is %d.\n", child, x);
}
