#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_BUFFER 1000000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void print(int pid, char cmd[]) {
  int count = 0;
  printf("[child %u] Received ", pid);
  while (count < MAX_BUFFER) {
    if (cmd[count] != '\n') {
      printf("%c", cmd[count]);
      count++;
    } else {
      printf("\n");
      break;
    }
  }
}

void put(int fd, int pid, char cmd[], int rlen) {
  char fname[MAX_BUFFER];
  int length = 0;
  int olen;
  int ind = 4;
  int rnum = 0;
  int statue = 0;
  int n;
  while (true) {
    if (statue == 0) {
      if (cmd[ind] != ' ') {
        fname[rnum] = cmd[ind];
        ind++;
        rnum++;
      } else {
        fname[rnum] = '\0';
        rnum = 0;
        ind++;
        statue++;
      }
    } else if (statue == 1) {
      if (cmd[ind] != '\n') {
        if (cmd[ind] - '0' > 9 || cmd[ind] - '0' < 0) {
          n = send(fd, "ERROR INVALID REQUEST\n", 22, 0);
          if (n != 22) {
            fprintf(stderr, "ERROR: Failed to send\n");
            return;
          }
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
          return;
        } else {
          length *= 10;
          length += cmd[ind] - '0';
          ind++;
        }
      } else {
        if (length == 0) {
          n = send(fd, "ERROR INVALID REQUEST\n", 22, 0);
          if (n != 22) {
            fprintf(stderr, "ERROR: Failed to send\n");
            return;
          }
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
          return;
        }
        olen = length;
        ind++;
        statue++;
      }
    } else {
      break;
    }
  }
  pthread_mutex_lock(&mutex);
  if (access(fname, F_OK) != -1) {
    n = send(fd, "ERROR FILE EXISTS\n", 18, 0);
    fflush(NULL);
    if (n != 18) {
      fprintf(stderr, "ERROR: Failed to send\n");
      return;
    }
    printf("[child %u] Sent ERROR FILE EXISTS\n", pid);
    fflush(NULL);
    pthread_mutex_unlock(&mutex);
    return;
  }
  pthread_mutex_unlock(&mutex);
  pthread_mutex_lock(&mutex);
  FILE *f = fopen(fname, "w");
  while (true) {
    if (length <= rlen - ind) {
      n = fwrite(cmd + ind, 1, length, f);
      if (n != length) {
        fprintf(stderr, "ERROR: Failed to write\n");
        int er = errno;
        printf("%d\n", er);
        fclose(f);
        break;
      }
      printf("[child %u] Stored file \"%s\" (%d bytes)\n", pid, fname, olen);
      n = send(fd, "ACK\n", 4, 0);
      if (n != 4) {
        fprintf(stderr, "ERROR: Failed to send\n");
        fclose(f);
        break;
      }
      printf("[child %d] Sent ACK\n", pid);
      fclose(f);
      break;
    } else {
      n = fwrite(cmd + ind, 1, rlen - ind, f);
      if (n != rlen - ind) {
        fprintf(stderr, "ERROR: Failed to write\n");
        fclose(f);
        break;
      }
      length -= rlen - ind;
    }
    n = recv(fd, cmd, MAX_BUFFER, 0);
    if (n < 0) {
      fprintf(stderr, "ERROR: Failed to receive from client\n");
      fclose(f);
      break;
    }
    rlen = n;
    ind = 0;
  }
  pthread_mutex_unlock(&mutex);
  fflush(NULL);
}

void get(int fd, int pid, char cmd[]) {
  char fname[MAX_BUFFER];
  int offset = 0;
  int length = 0;
  int ind = 4;
  int rnum = 0;
  int statue = 0;
  int n;
  while (true) {
    if (statue == 0) {
      if (cmd[ind] != ' ') {
        fname[rnum] = cmd[ind];
        ind++;
        rnum++;
      } else {
        fname[rnum] = '\0';
        rnum = 0;
        ind++;
        statue++;
      }
    } else if (statue == 1) {
      if (cmd[ind] != ' ') {
        if (cmd[ind] - '0' > 9 || cmd[ind] - '0' < 0) {
          n = send(fd, "ERROR INVALID REQUEST\n", 22, 0);
          if (n != 22) {
            fprintf(stderr, "ERROR: Failed to send\n");
            return;
          }
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
          return;
        } else {
          offset *= 10;
          offset += cmd[ind] - '0';
          ind++;
        }
      } else {
        ind++;
        statue++;
      }
    } else if (statue == 2) {
      if (cmd[ind] != '\n') {
        if (cmd[ind] - '0' > 9 || cmd[ind] - '0' < 0) {
          n = send(fd, "ERROR INVALID REQUEST\n", 22, 0);
          if (n != 22) {
            fprintf(stderr, "ERROR: Failed to send\n");
            return;
          }
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
          return;
        } else {
          length *= 10;
          length += cmd[ind] - '0';
          ind++;
        }
      } else {
        if (length == 0) {
          n = send(fd, "ERROR INVALID REQUEST\n", 22, 0);
          if (n != 22) {
            fprintf(stderr, "ERROR: Failed to send\n");
            return;
          }
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
          return;
        }
        ind++;
        statue++;
      }
    } else {
      break;
    }
  }
  FILE *f = fopen(fname, "r");
  if (f == NULL) {
    n = send(fd, "ERROR NO SUCH FILE\n", 19, 0);
    if (n != 19) {
      fprintf(stderr, "ERROR: Failed to send\n");
      return;
    }
    printf("[child %u] Sent ERROR NO SUCH FILE\n", pid);
    return;
  }
  pthread_mutex_lock(&mutex);
  n = fseek(f, offset, SEEK_SET);
  if (n != 0) {
    pthread_mutex_unlock(&mutex);
    n = send(fd, "ERROR INVALID BYTE RANGE\n", 25, 0);
    if (n != 25) {
      fprintf(stderr, "ERROR: Failed to send\n");
      return;
    }
    printf("[child %u] Sent ERROR INVALID BYTE RANGE\n", pid);
    return;
  }
  char buff[length];
  n = fread(buff, 1, length, f);
  buff[length] = '\0';
  if (n < length) {
    pthread_mutex_unlock(&mutex);
    n = send(fd, "ERROR INVALID BYTE RANGE\n", 25, 0);
    if (n != 25) {
      fprintf(stderr, "ERROR: Failed to send\n");
      return;
    }
    printf("[child %u] Sent ERROR INVALID BYTE RANGE\n", pid);
    return;
  }
  pthread_mutex_unlock(&mutex);
  char buff2[64];
  strcpy(buff2, "ACK ");
  sprintf(buff2 + 4, "%d", length);
  strcat(buff2, "\n");
  strcat(buff2, buff);
  n = send(fd, buff2, strlen(buff2), 0);
  fflush(NULL);
  if (n != strlen(buff2)) {
    fprintf(stderr, "ERROR: Failed to send\n");
    return;
  }
  printf("[child %u] Sent ACK %d\n", pid, length);
  printf("[child %u] Sent %d bytes of \"%s\" from offset %d\n", pid, length,
         fname, offset);
  fflush(NULL);
}

void list(int fd, int pid) {
  int fnum = 0;
  int n, i;
  char buff[MAX_BUFFER];
  struct dirent **flist;
  chdir("..");
  pthread_mutex_lock(&mutex);
  fnum = scandir("storage", &flist, 0, alphasort);
  chdir("storage");
  pthread_mutex_unlock(&mutex);
  if (fnum < 0) {
    fprintf(stderr, "ERROR: Cant open directory\n");
    return;
  }
  sprintf(buff, "%d", fnum - 2);
  printf("[child %u] Sent %d", pid, fnum - 2);
  for (i = 0; i < fnum; i++) {
    if (flist[i]->d_type == DT_REG) {
      strcat(buff, " ");
      strcat(buff, flist[i]->d_name);
      printf(" %s", flist[i]->d_name);
    }
  }
  strcat(buff, "\n");
  n = send(fd, buff, strlen(buff), 0);
  if (n != strlen(buff)) {
    fprintf(stderr, "ERROR: Failed to send\n");
    return;
  }
  printf("\n");
  fflush(NULL);
}

void *process(void *argv) {
  int *fd = (int *)argv;
  int pid = getpid();
  int n, ns;
  char buff[MAX_BUFFER];
  while (true) {
    n = recv(*fd, buff, MAX_BUFFER, 0);
    if (n < 0) {
      fprintf(stderr, "ERROR: Failed to receive from client\n");
      return 0;
    } else if (n == 0) {
      printf("[child %u] Client disconnected\n", pid);
      fflush(NULL);
      return 0;
    } else {
      print(pid, buff);
      if (strncmp(buff, "PUT ", 4) == 0) {
        put(*fd, pid, buff, n);
      } else if (strncmp(buff, "GET ", 4) == 0) {
        get(*fd, pid, buff);
      } else if (strncmp(buff, "LIST\n", 5) == 0) {
        list(*fd, pid);
      } else {
        ns = send(*fd, "ERROR INVALID REQUEST\n", 22, 0);
        if (ns != 22) {
          fprintf(stderr, "ERROR: Failed to send\n");
        } else {
          printf("[child %u] Sent ERROR INVALID REQUEST\n", pid);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "ERROR: Invalid arguments\n"
                    "usage: ./ [port]\n");
    return EXIT_FAILURE;
  }
  unsigned short port = atoi(argv[1]);

  int res = chdir("storage");
  if (res < 0) {
    fprintf(stderr, "ERRNO DOES NOT EXIST:");
    return EXIT_FAILURE;
  }

  int sd = socket(PF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    fprintf(stderr, "ERROR: socket() failed\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server;
  struct sockaddr_in client;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  int len = sizeof(server);
  if (bind(sd, (struct sockaddr *)&server, len) < 0) {
    fprintf(stderr, "ERROR: bind() failed\n");
    return EXIT_FAILURE;
  }

  listen(sd, 5);
  int fromlen = sizeof(client);
  printf("Started server\n");
  printf("Listening for TCP connections on port: %u\n", port);
  int pcount = 8;
  int pnum = 0;
  pthread_t *tid = (pthread_t *)malloc(8 * sizeof(pthread_t));

  while (true) {
    int *newsock = (int *)malloc(sizeof(int));
    *newsock = accept(sd, (struct sockaddr *)&client, (socklen_t *)&fromlen);
    printf("Rcvd incoming TCP connection from: %s\n",
           inet_ntoa((struct in_addr)client.sin_addr));
    fflush(NULL);
    if (pnum == pcount) {
      pcount *= 2;
      tid = (pthread_t *)realloc(tid, pcount * sizeof(pthread_t));
    }
    int rc = pthread_create(&tid[pnum], NULL, &process, (void *)newsock);
    pnum++;
    if (rc != 0) {
      fprintf(stderr, "ERROR: Failed to create thread\n");
      return EXIT_FAILURE;
    }
  }
}
