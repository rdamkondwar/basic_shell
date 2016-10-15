#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#define SIZE_MAX 514

struct JobNode {
  int jid;
  int argc;
  char **args;
  struct JobNode *next;
  int isBackgroundJob;
  int pid;
};

typedef struct JobNode JobNode;

struct JobList {
  struct JobNode *head;
};
typedef struct JobList JobList;

extern struct JobList jobList;
extern int jobCounter;

JobNode * findJobWithJid(int jid);
void freeJobNodeArgs(struct JobNode *jobNode);
void deleteJobNodeFromList(struct JobNode *jobNode);
void appendJobInTheList(struct JobNode *node);
void printJobList();
void forceDeleteJobs();
