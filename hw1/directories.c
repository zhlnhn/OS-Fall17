/* directories.c */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
  DIR * dir = opendir( "." );   /* open the current working directory */

  if ( dir == NULL )
  {
    perror( "opendir() failed" );
    return EXIT_FAILURE;
  }

  struct dirent * file;

  while ( ( file = readdir( dir ) ) != NULL )
  {
    printf( "found %s", file->d_name );   /* e.g., "xyz.txt" */

    struct stat buf;
    int rc = lstat( file->d_name, &buf );  /* e.g., "xyz.txt" */
     /* ==> "assignments/xyz.txt" */
     /* chdir() */


    if ( rc == -1 )
    {
      perror( "lstat() failed" );
      return EXIT_FAILURE;
    }

    printf( " (%d bytes)", (int)buf.st_size );

    if ( S_ISREG( buf.st_mode ) )
    {
      printf( " -- regular file\n" );
    }
    else if ( S_ISDIR( buf.st_mode ) )
    {
      printf( " -- directory\n" );
    }
    else
    {
      printf( " -- ????\n" );
    }

    /* TO DO: determine whether each file is executable ...

                man lstat      */

  }

  closedir( dir );

  return EXIT_SUCCESS;
}
