#include "mysh.h"

void forceDeleteJobs() {
  if (NULL == jobList.head) {
    return;
  }
  while (jobList.head != NULL) {
    deleteJobNodeFromList(jobList.head);
  }
}

JobNode * findJobWithJid(int jid) {
  JobNode *n = jobList.head;
  while (NULL != n) {
    if (n->jid == jid) {
      return n;
    }
    n = n->next;
  }
  // Not found.
  return NULL;
}

void freeJobNodeArgs(JobNode *jobNode) {
  int i = 0;
  for (; i < jobNode->argc; i++) {
    free(jobNode->args[i]);
  }
  free(jobNode->args);
}

void deleteJobNodeFromList(JobNode *jobNode) {
  JobNode *node = jobList.head;
  JobNode *prev;

  if (NULL == jobList.head) {
    return;
  }

  // Check if its root node
  if (jobNode->jid == node->jid) {
    jobList.head = node->next;
    freeJobNodeArgs(node);
    free(node);
    return;
  }

  while (node != NULL) {
    if (node->jid == jobNode->jid) {
      // printf("debug: deleting pid %d jid %d\n", jobNode->pid, jobNode->jid);
      prev->next = node->next;
      freeJobNodeArgs(node);
      free(node);
      return;
    }
    prev = node;
    node = node->next;
  }
}

void appendJobInTheList(JobNode *node) {
  node->next = NULL;
  if (jobList.head == NULL) {
    jobList.head = node;
    return;
  }

  JobNode *n = jobList.head;

  while (n->next != NULL) {
    n = n->next;
  }
  n->next = node;
  return;
}

void printJob(JobNode *node) {
  fprintf(stdout, "%d :", node->jid);

  int i = 0;
  for (i = 0; i < node->argc; i++) {
    fprintf(stdout, " %s", node->args[i]);
  }
  fprintf(stdout, "\n");
  fflush(stdout);
}

void printJobList() {
  JobNode *node = jobList.head;
  while (node != NULL) {
    printJob(node);
    node = node->next;
  }
}
