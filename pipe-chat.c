#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define FAIL -1

int main(void) {
  // 2-way pipe, master proc writes on [0],
  // child procs write on [1]
  int pipefds[2];
  // init pipe
  if (pipe(pipefds) == FAIL) {
    perror("creating pipe");
    return 1;
  }

  // interact with 2 child processes
  for (int i = 1; i <= 2; i++) {
    int pid;
    switch (pid = fork()) {
      case 0:               // child process
        close(pipefds[0]);  // close the read side
        char msg[30];
        sprintf(msg, "Child process %d is sending a message!\n", i);
        if (write(pipefds[1], msg, strlen(msg)) == FAIL) {  // send the msg
          perror("sending msg");
          exit(1);
        }
        exit(0);

      case -1:  // fork err
        perror("forking");
        return 1;

      default:  // master process
        waitpid(pid, NULL, 0);
    }
  }

  // read from pipe
  close(pipefds[1]);  // close the write side
  char buf[100];
  int len = read(pipefds[0], buf, 100);  // receive msgs
  buf[len] = 0;                          // end the string
  printf("received from child processes:\n%s", buf);

  return 0;
}