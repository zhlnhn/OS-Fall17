#include "hw3.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
FILE *f;
int currIdx;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_i = PTHREAD_MUTEX_INITIALIZER;

void writeFile(void) {
  int i = 0;
  while (i < maxwords) {
    fprintf(f, "%s\n", words[i]);
    words[i][0] = '\0';
    i++;
  }
}

void writeBuffer(char *word, int i) {
  fflush(NULL);
  if (i == maxwords) {
    pthread_mutex_lock(&mutex_buffer);
    printf("MAIN: Buffer is full; writing %d words to output file\n", maxwords);
    fflush(NULL);
    writeFile();
    i = 0;
    currIdx = 0;
    pthread_mutex_unlock(&mutex_buffer);
  }
  pthread_mutex_lock(&mutex_buffer);

  strncpy(words[i], word, 80);

  printf("TID %u: Stored \"%s\" in shared buffer at index [%d]\n",
         (unsigned int)pthread_self(), word, i);
  fflush(NULL);

  pthread_mutex_unlock(&mutex_buffer);
}

void filesInfo(char ***directory, char *location, int *numFiles) {
  DIR *dir = opendir(location);
  printf("MAIN: Opened \"%s\" directory\n", location);
  fflush(NULL);
  struct dirent *file;

  if (dir == NULL) {
    perror("opendir() failed");
  }

  int ret = chdir(location);

  if (ret == -1) {
    perror("chdir() failed");
  }

  int i = 0;
  while ((file = readdir(dir)) != NULL) {
    struct stat buf;

    int rc = lstat(file->d_name, &buf);
    if (rc == -1) {
      perror("lstat() failed");
    }

    if (S_ISREG(buf.st_mode)) {//check if it's a regular file
      (*numFiles)++;
      if (i == 0) {
        *directory = (char **)calloc(1, sizeof(char *));
      } else {
        *directory = realloc(*directory, (i + 1) * sizeof(char *));
      }
      (*directory)[i] = (char *)calloc(80 + 1, sizeof(char));
      strncpy((*directory)[i], file->d_name, 80);
      i++;
    }
  }
  printf("MAIN: Closed \"%s\" directory\n", location);
  fflush(NULL);
  closedir(dir);
}

void process(char *buff) {
  char *tmpWord = (char *)calloc(80, sizeof(char));
  int i = 0;
  int j = 0;
  int countWord = -1;
  for (i = 0; i < strlen(buff); i++) {
    if (isalpha(buff[i]) || isdigit(buff[i])) {
      countWord += 1;
      j = 0;
      tmpWord[0] = buff[i];
      while (1) {
        j += 1;
        if (isalpha(buff[i + j])) {
          tmpWord[j] = buff[i + j];
        } else if (isdigit(buff[i+j])) {
          tmpWord[j] = buff[i + j];
        } else {
          break;
        }
      }
    } else {
      continue;
    }
    fflush(NULL);
    if (j > 1) {
      pthread_mutex_lock(&mutex);
      writeBuffer(tmpWord, currIdx);
      currIdx++;
      pthread_mutex_unlock(&mutex);
    } else {
      countWord -= 1;
    }
    memset(tmpWord, 0, strlen(tmpWord));
    i += j;
  }
  free(tmpWord);
  tmpWord = NULL;
}

void *readFile(void *arg) {
  char *file = (char *)arg;
  char *buffer = NULL;
  FILE *fp = fopen(file, "r");
  printf("TID %u: Opened \"%s\"\n", (unsigned int)pthread_self(), file);
  pthread_mutex_unlock(&mutex_i);
  fflush(NULL);
  if (fp != NULL) {
    if (fseek(fp, 0L, SEEK_END) == 0) {
      long bufsize = ftell(fp);
      if (bufsize == -1) {
        perror("ftell() failed");
      }
      buffer = (char *)calloc(bufsize + 1, sizeof(char));

      if (fseek(fp, 0L, SEEK_SET) != 0) {
        perror("fseek() failed");
      }
      size_t newLen = fread(buffer, sizeof(char), bufsize, fp);
      if (newLen == 0) {
        perror("fread() failed");
      } else {
        buffer[newLen] = '\0';
        process(buffer);
      }
    }
    fclose(fp);
    printf("TID %u: Closed \"%s\"; and exiting\n", (unsigned int)pthread_self(),
           file);
    fflush(NULL);
  }
  free(buffer);
  pthread_exit(NULL);
}

void multiThrd(char **directory, unsigned int numFiles) {
  pthread_t tid[numFiles];
  int i, rc;
  i = 0;
  for (i = 0; i < numFiles; i++) {
    pthread_mutex_lock(&mutex_i);
    rc = pthread_create(&tid[i], NULL, &readFile, (void *)(directory[i]));
    printf("MAIN: Created child thread for \"%s\"\n", directory[i]);
    fflush(NULL);
    if (rc != 0) {
      fprintf(stderr, "Could not create thread\n");
      fflush(NULL);
    }
  }
  for (i = 0; i < numFiles; i++) {
    unsigned int *x;
    rc = pthread_join(tid[i], (void **)&x);

    if (rc != 0) {
      fprintf(stderr, "Could not join thread\n");
      fflush(NULL);
    }
    printf("MAIN: Joined child thread: %u\n", (unsigned int)tid[i]);
    fflush(NULL);
    free(x);
  }
  for (i = 0; i < currIdx; i++) {
    fprintf(f, "%s\n", words[i]);
  }
  if (currIdx == maxwords) {
    printf("MAIN: Buffer is full; writing %d words to output file\n", maxwords);
  }
  currIdx = currIdx % maxwords;
  printf("MAIN: All threads are done; writing %d words to output file\n",
         currIdx);
  fflush(NULL);
}
int main(int argc, char *argv[]) {
  int i;
  if (argc != 4) {
    fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out "
                    "<input-directory> <buffer-size> <output-file>\n");
    return EXIT_FAILURE;
  }
  maxwords = atoi(argv[2]);
  words = (char **)calloc(maxwords, sizeof(char *));
  for (i = 0; i < maxwords; i++) {
    words[i] = (char *)calloc(80, sizeof(char));
  }
  printf("MAIN: Dynamically allocated memory to store %d words\n", maxwords);
  fflush(NULL);
  f = fopen(argv[3], "w");
  if (f == NULL) {
    printf("Error opening file!\n");
    exit(1);
  }
  printf("MAIN: Created \"%s\" output file\n", argv[3]);
  fflush(NULL);

  int numFiles = 0;
  char **directory;
  filesInfo(&directory, argv[1], &numFiles);

  multiThrd(directory, numFiles);

  for (i = 0; i < maxwords; i++) {
    free(words[i]);
  }
  for (i = 0; i < numFiles; i++) {
    free(directory[i]);
  }
  free(words);
  free(directory);
  fclose(f);
  return EXIT_SUCCESS;
}
