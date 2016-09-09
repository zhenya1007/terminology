#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <Eina.h>
#include "tycommon.h"


int
main(int argc, char **argv)
{
   int i, perm = 0;

   ON_NOT_RUNNING_IN_TERMINOLOGY_EXIT_1();

   if (argc > 1 &&
       (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
     {
        printf("Usage: %s [-p] [FILE1 FILE2 ...]\n"
               "  Change the terminal background to the given file/uri\n"
               "  -p  Make change permanent (stored in config)\n"
               "\n",
               argv[0]);
        return 0;
     }
   if (argc <= 1)
     {
        char tbuf[32];
        snprintf(tbuf, sizeof(tbuf), "%c}bt", 0x1b);
        if (write(STDIN_FILENO, tbuf, strlen(tbuf) + 1) != (signed)(strlen(tbuf) + 1)) perror("write");
        return 0;
     }
   for (i = 1; i < argc; i++)
     {
        char *path, buf[PATH_MAX * 2], tbuf[PATH_MAX * 3];

        if (!strcmp(argv[i], "-p"))
          {
             perm = 1;
             i++;
             if (i >= argc) break;
          }
        path = argv[i];
        if (realpath(path, buf)) path = buf;
        if (perm)
          snprintf(tbuf, sizeof(tbuf), "%c}bp%s", 0x1b, path);
        else
          snprintf(tbuf, sizeof(tbuf), "%c}bt%s", 0x1b, path);
        if (write(STDIN_FILENO, tbuf, strlen(tbuf) + 1) != (signed)(strlen(tbuf) + 1)) perror("write");
     }
   return 0;
}
