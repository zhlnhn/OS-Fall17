/* fd-open.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if 0

  file descriptor (fd)

  -- each process has a file descriptor table associated with it
      that keeps track of its open files (a.k.a., byte streams)

  fd         C++     Java           C
  0 stdin    cin     System.in      scanf(), read(), getchar(), ...
  1 stdout   cout    System.out     printf(), write(), putchar(), ...
  2 stderr   cerr    System.err     fprintf( stderr, "ERROR: ....\n" );
                                    perror( "open() failed" );

  stdout and stderr (by default) are both displayed on the terminal

  stdout is buffered output
   (therefore, we use '\n' at the end of printf() statements and
     also fflush() to explicitly flush the output buffer)

  stderr (fd 2) is not buffered

#endif

int main()
{
  char name[80];
  strcpy( name, "testfile.txt" );

  int fd = open( name, O_RDONLY );

  if ( fd == -1 )
  {
    perror( "open() failed" );
    return EXIT_FAILURE;
  }

  /*    fd table:
        0 stdin
        1 stdout
        2 stderr
        3 testfile.txt [O_RDONLY]
  */

  printf( "fd is %d\n", fd );

  char buffer[80];

/* Set j to different values here to see what
    happens if you read from a file multiple times
     (also adjust how much data is in testfile.txt) */
int j = 1;
while ( j > 0 )
{
  int rc = read( fd, buffer, 79 );  /* read in at most 79 bytes, leaving
                                       room for the null '\0' character
                                       that identifies the end of the string */

  /* TO DO: use the rc value to detect when we're done reading the
             file (i.e., rc will be 0) */

  printf( "rc is %d\n", rc );

  buffer[rc] = '\0';   /* given that I'm treating this data as a string,
                          explicitly null-terminate the string */

    /* buffer: "this is a test\n\0"  */

  printf( "read: [%s]\n", buffer );

  j--;
}

  close( fd );   /* remove entry 3 from the fd table */

  return EXIT_SUCCESS;
}
