#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int check_division(char *expr) {
  int i = 0;
  for (i = 0; i < strlen(expr); i++) {
    if (expr[i] == '0' && expr[i + 1] == ')' && expr[i - 1] == ' ') {
      return 1;
    }
  }
  return 0;
}
// count number of pairs of parethethese, which stands for how many
int counting(char *expr) {
  int i, num_of_ops = 0;
  for (i = 0; i < strlen(expr); i++) {
    if (isdigit(expr[i])) {
      num_of_ops++;
    } else if (expr[i] == '(' && i > 0) {
      int count_paren = 0;
      while (count_paren != 0 && i < strlen(expr)) {
        if (expr[i] == '(')
          count_paren++;
        else if (expr[i] == ')')
          count_paren--;
        i++;
      }
      if (!count_paren)
        num_of_ops++;
    }
  }
  return num_of_ops;
}
int findrp(char *expression, int index) {
  int i;
  int open_count = 0; /* start at -1 to ignore first open parenthses*/
  int close_count = 0;
  for (i = index; i < strlen(expression); i++) {
    if (open_count == close_count && i > index) {
      return i;
    }
    if (expression[i] == '(') {
      open_count++;
    }
    if (expression[i] == ')') {
      close_count++;
    }
  }
  return i;
}
int not_operator(char a) {
  return !(a == '+' || a == '-' || a == '/' || a == '*' || isdigit(a) ||
           a == '(' || a == ')' || a == ' ');
}

// lp stands for left parentheses, rp stands for right parentheses
int parser(char *line, int lp, int isexit, int level) {
  int i = 0;
  char op = '\0';
  int status = 0;
  int rp = findrp(line, lp);
  printf("PID %d: My expression is \"%s\"\n", getpid(), line);
  int result = 0;
  for (i = lp; i < rp; i++) {
    if (not_operator(line[i])) {
      int j = 0;
      char inval_op[128];
      // Extract the invalid operator
      while (not_operator(line[i])) {
        inval_op[j] = line[i];
        i++;
        j++;
      }
      printf("PID %d: ERROR: unknown \"%s\" operator; exiting\n", getpid(),
             inval_op);
      fflush(stdout);
      exit(1);
    } else if ((line[i] == '+' || line[i] == '-' || line[i] == '*' ||
                line[i] == '/') &&
               line[i + 1] == ' ') {
      op = line[i];
      printf("PID %d: Starting \"%c\" operation\n", getpid(), op);
      fflush(stdout);
      if (counting(line) < 2) {
        printf("PID %d: ERROR: not enough operands; exiting\n", getpid());
        fflush(stdout);
        isexit = 1;
      }
    } else if (isdigit(line[i])) {
      int j = 0;
      char temp[128];
      if (line[i - 1] == '-') {
        temp[j] = '-';
        j++;
      }
      while (isdigit(line[i]) && i < strlen(line)) {
        temp[j] = line[i];
        i++;
        j++;
      }
      temp[j] = '\0';
      int p[2];
      int rc;
      rc = pipe(p);
      if (rc == -1) {
        perror("wrong pipe!\n");
        fflush(stdout);
        exit(1);
      }
      pid_t pid = fork();
      if (pid < 0) {
        perror("wrong fork\n");
        fflush(stdout);
        exit(1);
      }
      if (pid == 0) {
        close(p[0]);
        p[0] = -1;
        printf("PID %d: My expression is \"%s\"\n", getpid(), temp);
        fflush(stdout);
        printf("PID %d: Sending \"%s\" on pipe to parent\n", getpid(), temp);
        fflush(stdout);
        write(p[1], temp, strlen(temp));
        close(p[1]);
        exit(0);
      } else if (pid > 0) {
        pid_t child_pid = wait(&status);
        if (WIFSIGNALED(status)) /* child process was terminated due to */
        {                        /* a signal (e.g., segfault, etc.)     */
          printf("PID %d: child %d terminated abnormally\n", getpid(),
                 child_pid);
        } else if (WIFEXITED(status)) /* child proc returned or called exit() */
        {
          if (status != 0) {
            isexit = 1;
            printf("PID %d: child terminated with nonzero exit status 1 [child "
                   "pid %d]\n",
                   getpid(), child_pid);
          }
        }
        if (op == '/') {
          if (check_division(line)) {
            printf("PID %d: ERROR: division by zero is not allowed; exiting\n",
                   getpid());
            fflush(stdout);
            exit(1);
          }
        }

        close(p[1]);
        p[1] = -1;
        char temp[128];
        int bytes = read(p[0], temp, 128);
        temp[bytes] = '\0';
        int num = atoi(temp);
        if (result == 0) {
          result += num;
        } else if (op == '+') {
          result += num;
        } else if (op == '-') {
          result -= atoi(temp);
        } else if (op == '*') {
          result *= num;
        } else if (op == '/') {
          if (num == 0) {
            printf("PID %d: ERROR: division by zero is not allowed; exiting\n",
                   getpid());
            fflush(stdout);
            exit(1);
          }
          // printf("%c %d", op, num);
          fflush(stdout);
          result /= num;
        }
      }
    } else if (line[i] == '(' && i != 0) {
      int mark = i;
      char temp[128];
      int ps = 0;
      while (line[i] != ')') {
        temp[i - mark] = line[i];
        if (line[i] == '(') {
          ps++;
        }
        i++;
      }
      int j = 0;
      while (j < ps) {
        temp[i - mark + j] = ')';
        j++;
      }
      temp[i - mark + j] = '\0';
      int p[2];
      int rc;
      rc = pipe(p);
      if (rc == -1) {
        perror("wrong pipe\n");
        fflush(stdout);
        return 1;
      }
      pid_t pid = fork();
      if (pid < 0) {
        perror("wrong fork\n");
        fflush(stdout);
        return 1;
      }
      if (pid == 0) {
        close(p[0]);
        p[0] = -1;
        int result_parathe = parser(temp, 0, isexit, level + 1);
        printf("PID %d: Processed \"%s\"; sending \"%d\" on pipe to parent\n",
               getpid(), temp, result_parathe);
        fflush(stdout);
        char temp[128];
        sprintf(temp, "%d", result_parathe);
        write(p[1], temp, strlen(temp));
        exit(0);
      }
      if (pid > 0) {
        // wait(0);
        pid_t child_pid = wait(&status);
        if (WIFSIGNALED(status)) /* child process was terminated due to */
        {                        /* a signal (e.g., segfault, etc.)     */
          printf("PID %d: child %d terminated abnormally\n", getpid(),
                 child_pid);
        } else if (WIFEXITED(status)) /* child proc returned or called exit() */
        {
          if (status != 0) {
            isexit = 1;
            printf("PID %d: child terminated with nonzero exit status 1 [child "
                   "pid %d]\n",
                   getpid(), child_pid);
            isexit = 1;
          }
        }
        close(p[1]);
        p[1] = -1;
        char temp[128];
        int bytes = read(p[0], temp, 128);
        temp[bytes] = '\0';
        int num = atoi(temp);
        if (result == 0) {
          result += num;
        } else if (op == '+') {
          result += num;
        } else if (op == '-') {
          result -= num;
        } else if (op == '*') {
          result *= num;
        } else if (op == '/') {
          if (num == 0) {
            printf("PID %d: ERROR: division by zero is not allowed; exiting\n",
                   getpid());
            fflush(stdout);
            exit(1);
          }
          // printf("%c %d", op, num);
          result /= num;
        }
      }
    }
  }
  if (isexit == 1) {
    exit(1);
  }
  if (isexit == 0 && level == 0) {
    printf("PID %d: Processed \"%s\"; final answer is \"%d\"", getpid(), line,
           result);
    fflush(stdout);
  }
  return result;
}

int main(int argc, char **argv) {
  if (argc == 2) {
    char line[128];
    FILE *fptr = fopen(argv[1], "r");
    fgets(line, 128, fptr);

    line[strcspn(line, "\n")] = 0;
    int if_exit = 0;
    parser(line, 0, if_exit, 0);

    return 0;
  } else if (argc < 2) {
    fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out <input-file>");
    fflush(NULL);
    return 1;
  }
  else {
    return 1;
  }
}
