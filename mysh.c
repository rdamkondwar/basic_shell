#include "mysh.h"

int jobCounter = 0;
JobList jobList;

int parseCommand(JobNode *, char *);
void runCommand(JobNode * jobNode);
int isBackgroundJob(JobNode *jobNode);
void freeCompletedBackgroundJobs();
char * readline(FILE *, int *);
void consumeRemainingLine(FILE *fp);

void cleanup(FILE *fp) {
  forceDeleteJobs();
  if (NULL != fp) {
      fclose(fp);
  }
}

void batchMode(char * file) {
  FILE *fp = (FILE *) fopen(file, "r");

  if (NULL == fp) {
    fprintf(stderr, "Error: Cannot open file %s\n", file);
    exit(1);
  }
  while (1) {
    int ret = 0;
    char *cmd = readline(fp, &ret);

    if ( 2 == ret ) {
      fprintf(stdout, "%s\n", cmd);
      fflush(stdout);
      fprintf(stderr, "Error: Command too long.\n");
      fflush(stderr);
      free(cmd);
      continue;
    }

    if (ret > 0) {
      fflush(stdout);
      free(cmd);
      cleanup(fp);
      return;
    }

    fprintf(stdout, "%s\n", cmd);
    fflush(stdout);

    struct JobNode *jobNode = (struct JobNode *) malloc(sizeof(struct JobNode));
    ret = 0;
    ret = parseCommand(jobNode, cmd);
    if (ret == 1) {
      free(cmd);
      free(jobNode);
      cleanup(fp);
      return;
    }
    jobNode->isBackgroundJob = isBackgroundJob(jobNode);

    if (jobNode->argc > 0) {
      runCommand(jobNode);
    }

    if (!jobNode->isBackgroundJob) {
      freeJobNodeArgs(jobNode);
      free(jobNode);
    }

    // freeCompletedBackgroundJobs();
    free(cmd);
  }
}

void interactiveMode() {
  while (1) {
    fprintf(stdout, "mysh> ");
    fflush(stdout);

    struct JobNode *jobNode = (struct JobNode *) malloc(sizeof(struct JobNode));
    int ret;
    char *cmd = readline(stdin, &ret);

    if (2 == ret) {
      fprintf(stdout, "%s\n", cmd);
      fflush(stdout);

      fprintf(stderr, "Error: Command too long.\n");
      fflush(stderr);
      free(cmd);
      continue;
    }

    if (ret > 0) {
      // freeJobNodeArgs(jobNode);
      free(jobNode);
      free(cmd);
      cleanup(NULL);
      return;
    }
    ret = 0;
    ret = parseCommand(jobNode, cmd);
    if (ret == 1) {
      freeJobNodeArgs(jobNode);
      free(jobNode);
      free(cmd);
      cleanup(NULL);
      return;
    }
    jobNode->isBackgroundJob = isBackgroundJob(jobNode);

    if ( jobNode->argc > 0 ) {
      runCommand(jobNode);
    }

    if (!jobNode->isBackgroundJob) {
      freeJobNodeArgs(jobNode);
      free(jobNode);
    }

    // check if any background jobs have ended..
    // freeCompletedBackgroundJobs();
    free(cmd);
  }
}

int
main(int argc, char* argv[]) {
  jobCounter = 0;

  if (argc > 1) {
    if (argc > 2) {
      fprintf(stderr, "Usage: mysh [batchFile]\n");
      fflush(stderr);
      exit(1);
    }
    // Batch mode;
    batchMode(argv[1]);
  } else {
    interactiveMode();
  }
  return 0;
}

void freeCompletedBackgroundJobs() {
  struct JobNode *jobNode = jobList.head;

  while (jobNode != NULL) {
    int status = 0;
    int ret = waitpid(jobNode->pid, &status, WNOHANG);

    if (ret < 0 && errno != ECHILD) {
      perror("Error: ");
      return;
    } else if (ret < 0  && errno == ECHILD) {
      // Delete jobNode from list...
      deleteJobNodeFromList(jobNode);
    } else if (ret == jobNode->pid && WIFEXITED(status)) {
      // Delete jobNode from list...
      deleteJobNodeFromList(jobNode);
    }
    jobNode = jobNode -> next;
  }
}

int isBackgroundJob(struct JobNode *jobNode) {
  if (jobNode->argc > 0) {
    // if (strcmp("&", jobNode->args[(jobNode->argc)-1]) == 0) {
    int len = strlen(jobNode->args[(jobNode->argc)-1]);
    if (jobNode->args[(jobNode->argc)-1][len-1] == '&') {
      // printf("debug: %s\n", jobNode->args[(jobNode->argc)-1]);
      // free(jobNode->args[jobNode->argc-1]);

      if (len == 1) {
        jobNode->args[jobNode->argc-1] = NULL;
        jobNode->argc--;
      } else {
        jobNode->args[jobNode->argc-1][len-1] = '\0';
      }

      return 1;
    }
  }
  return 0;
}

int handleInBuiltCommands(JobNode *jobNode) {
  // Inbuilt command j
  if (jobNode->argc == 1 && strcmp(jobNode->args[0], "j") == 0) {
    // InBuilt command..
    freeCompletedBackgroundJobs();
    printJobList();
    return 1;
  }

  if (jobNode->argc == 2 && strcmp(jobNode->args[0], "myw") == 0) {
    if (jobNode->args[1] == NULL) {
      fprintf(stderr, "Usage: myw <job_id>\n");
      fflush(stderr);
      return 1;
    }

    int jid = atoi(jobNode->args[1]);
    JobNode *n = findJobWithJid(jid);
    if (NULL == n) {
      fprintf(stderr, "Invalid jid %d\n", jid);
      fflush(stderr);
      return 1;
    }
    // printf("Debug: Found child with pid: %d\n", n->pid);
    // start time
    struct timeval tv1, tv2;

    gettimeofday(&tv1, NULL);
    // long int before = (tv1.tv_sec*1000000L + tv1.tv_usec);
    // printf("debug sec: %lu before: %lu\n", tv1.tv_sec
    // , (tv1.tv_sec*1000000L + tv1.tv_usec));
    waitpid(n->pid, NULL, 0);

    gettimeofday(&tv2, NULL);
    // long int after = (tv2.tv_sec*1000000L + tv2.tv_usec);
    // printf("debug sec: %lu after: %lu\n",
    // tv2.tv_sec, (tv2.tv_sec*1000000L + tv2.tv_usec));
    // printf("debug diff: %lu\n", after-before);
    // end time
    fprintf(stdout, "%lu : Job %d terminated\n",
       ((tv2.tv_sec-tv1.tv_sec)*1000000L + (tv2.tv_usec-tv1.tv_usec)/1000),
       n->jid);
    fflush(stdout);
    deleteJobNodeFromList(n);
    return 1;
  }

  if (jobNode->argc == 1 && strcmp(jobNode->args[0], "exit") == 0) {
    exit(0);
  }
  // No inbuilt command found...
  return 0;
}

void runCommand(struct JobNode *jobNode) {
  char **argv = jobNode->args;

  if (handleInBuiltCommands(jobNode)) {
    return;
  }
  jobCounter++;
  jobNode->jid = jobCounter;

  if (jobNode->isBackgroundJob) {
    appendJobInTheList(jobNode);
  }
  int ret = fork();

  if (ret < 0) {
    fprintf(stderr, "Error: Fork failed!\n");
    fflush(stderr);
  } else if (ret == 0) {
    // child
    execvp(argv[0], argv);
    fprintf(stderr, "%s: Command not found\n", argv[0]);
    fflush(stderr);
    exit(1);
  } else {
    // parent
    jobNode->pid = ret;
    if (!jobNode->isBackgroundJob) {
      int status;
      waitpid(jobNode->pid, &status, 0);
    }
  }
}

int parseCommand(struct JobNode *jobNode, char *cmd) {
  if (NULL == cmd) {
    return 1;
  }
  // Allocate 2d array for holding command splits
  jobNode->args = (char **)malloc(SIZE_MAX*sizeof(char *));
  // Get first part
  char *word = strtok(cmd, " \t");
  // word = strtok(word, "\n");
  // args[0] = word;
  int i = 0;
  while (NULL != word && strlen(word) > 0) {
    // printf("word = \"%s\"\n", word);
    jobNode->args[i] = strdup(word);
    i++;
    word = strtok(NULL, " \t");
  }
  // printf("debug2:, args[1]=%s, i=%d\n", (*args)[1], i);
  jobNode->args[i] = NULL;
  jobNode->argc = i;

  // free(cmd);
  return 0;
}

char *readline(FILE * fp, int *ret) {
  char *cmd = (char *) malloc(SIZE_MAX*sizeof(char));
  char *ret_str = fgets(cmd, SIZE_MAX, fp);

  if (NULL == ret_str) {
    *ret = 1;
    return cmd;
  }

  int len = strlen(cmd);

  if (cmd[len-1] == '\n') {
    cmd[len-1] = '\0';
    len--;
    *ret = 0;
    return cmd;
  }

  // Keep on reading till EOF file is reached.
  consumeRemainingLine(fp);
  *ret = 2;
  return cmd;
}

void consumeRemainingLine(FILE *fp) {
  char arr[SIZE_MAX];
  char *ret;
  arr[0] = '\0';
  do {
    ret = fgets(arr, SIZE_MAX, fp);
  }  while (NULL != ret && strlen(arr) == (SIZE_MAX-1));
}
