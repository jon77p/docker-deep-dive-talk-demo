#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <sys/syscall.h>

#define CHILD_STACK_SIZE 8196
#define BUFFER_SIZE 200
#define PROCS_MOUNT "/proc"

#define NEW_ROOT "/tmp/rootfs"
#define OLD_ROOT "oldrootfs"

#define PARENT 0
#define CHILD 1

#define SLEEP_TIMEOUT "60"
#define HOSTNAME "demo-namespace"

static int pivot_root(const char *new_root, const char *old_root) {
  return syscall(SYS_pivot_root, new_root, old_root);
}

char *currentCtx(int ctx) {
  char *result = NULL;

  result = calloc(sizeof(char), BUFFER_SIZE);

  if (ctx == PARENT) {
    sprintf(result, "[Parent %d]", getpid());
  } else if (ctx == CHILD) {
    sprintf(result, "[Child %d]", getpid());
  }

  return result;
}

void lsProcs(int ctx, char *basepath) {
  DIR *procs = NULL;
  struct dirent *f = NULL;
  struct stat buf;

  procs = opendir(basepath);
  if (procs == NULL) {
    perror("opendir");
    exit(-1);
  }

  while ((f = readdir(procs)) != NULL) {
    if (atoi(f->d_name)) {
      char fullpath[PATH_MAX];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", basepath, f->d_name);

      printf("%-25s   %s\n", currentCtx(ctx), fullpath);

      if (-1 == stat(fullpath, &buf)) {
        perror("stat");
        exit(-1);
      }

      lsProcs(ctx, fullpath);
    }
  }

  closedir(procs);
}

int execCMD(void *args) {
  char **argv = (char **)args;

  printf("%-25s Calling execvp with '%s'\n", currentCtx(CHILD), argv[0]);

  execvp(argv[0], argv);
  perror("execvp");

  return -1;
}

void runCMD(char *argv[]) {
  pid_t child_pid = -1;
  int status = -1;
  int flags;
  char *childStack = NULL;

  childStack = calloc(CHILD_STACK_SIZE, sizeof(char));
  if (childStack == NULL) {
    perror("calloc");
    exit(-1);
  }

  flags = CLONE_VM | SIGCHLD | CLONE_FS | CLONE_FILES;

  child_pid = clone(&execCMD, (void *)childStack + CHILD_STACK_SIZE, flags, (void *)argv);
  if (-1 == wait(&status)) {
    perror("wait");
    exit(-1);
  }

  printf("%-25s Child %d exited with status %d\n", currentCtx(CHILD), child_pid, status);

  free(childStack);
}

int childFn() {
  printf("%-25s Unsharing mount, filesystem, and hostname namespaces\n", currentCtx(CHILD));
  if (-1 == unshare(CLONE_NEWNS | CLONE_FS | CLONE_NEWUTS | CLONE_NEWIPC)) {
    perror("unshare");
    exit(-1);
  }

  printf("%-25s Changing root mount to '%s'\n", currentCtx(CHILD), NEW_ROOT);
  if (-1 == mount(NEW_ROOT, NEW_ROOT, NULL, MS_BIND, NULL)) {
    perror("mount");
    exit(-1);
  }

  char old_root_path[PATH_MAX];
  snprintf(old_root_path, sizeof(old_root_path), "%s/%s", NEW_ROOT, OLD_ROOT);
  if (-1 == mkdir(old_root_path, 0777)) {
    perror("mkdir");
    exit(-1);
  }

  if (-1 == pivot_root(NEW_ROOT, old_root_path)) {
    perror("pivot_root");
    exit(-1);
  }

  chdir("/");


  printf("%-25s Mounting procfs at %s\n", currentCtx(CHILD), PROCS_MOUNT);
  mkdir(PROCS_MOUNT, 0555);
  if (-1 == mount("proc", PROCS_MOUNT, "proc", 0, NULL)) {
    perror("mount");
    exit(-1);
  }

  printf("%-25s Setting new hostname to '%s'\n", currentCtx(CHILD), HOSTNAME);
  if (sethostname(HOSTNAME, strlen(HOSTNAME)) != 0) {
    perror("sethostname");
    exit(-1);
  }

  
  printf("%-25s Unmounting old root now at '%s'\n", currentCtx(CHILD), OLD_ROOT);
  if (-1 == umount2(OLD_ROOT, MNT_DETACH)) {
    perror("umount2");
    exit(-1);
  }
  if (-1 == rmdir(OLD_ROOT)) {
    perror("rmdir");
    exit(-1);
  }

  char *cmd1[2] = {"whoami", NULL};
  printf("%-25s Creating a new child to exec '%s'\n", currentCtx(CHILD), cmd1[0]);
  runCMD(cmd1);


  char *cmd2[3] = {"sleep", SLEEP_TIMEOUT, NULL};
  printf("%-25s Creating a new child to exec '%s'\n", currentCtx(CHILD), cmd2[0]);
  runCMD(cmd2);

  printf("%-25s Current Processes in '%s':\n", currentCtx(CHILD), PROCS_MOUNT);
  lsProcs(CHILD, PROCS_MOUNT);


  // Cleanup on exit
  printf("%-25s Unmounting procfs from '%s'\n", currentCtx(CHILD), PROCS_MOUNT);
  if (-1 == umount2(PROCS_MOUNT, MNT_DETACH)) {
    perror("umount2");
    exit(-1);
  }

  if (-1 == rmdir(PROCS_MOUNT)) { // not really necessary as we are in a new mount namespace, so nothing happens to the parent */
    perror("rmdir");
    exit(-1);
  }

  return 0;
}

int main() {
  char stack[CHILD_STACK_SIZE];
  int flags;
  pid_t child = 0;

  printf("%-25s Currently-visible Processes:\n", currentCtx(PARENT));

  char *argv[2] = {"ps", NULL};
  runCMD(argv);

  printf("%-25s Unsharing pid namespace for child procs\n", currentCtx(PARENT));
  if (-1 == unshare(CLONE_NEWPID | CLONE_NEWUSER)) {
    perror("unshare");
    exit(-1);
  }

  flags = CLONE_PARENT | SIGCHLD;

  printf("%-25s Cloning child proc\n", currentCtx(PARENT));
  child = clone(&childFn, (void *)stack + CHILD_STACK_SIZE, flags, NULL);
  if (-1 == child) {
    perror("clone");
    exit(-1);
  }

  printf("%-25s Child PID: \033[0;36m%d\033[0m\n", currentCtx(PARENT), child);
  printf("%-25s Attachable with 'sudo nsenter -t %d -m -u -i -n -p <prog>'\n", currentCtx(PARENT), child);

  return 0;
}
