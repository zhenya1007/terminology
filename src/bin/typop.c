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
   int i, bytes;

   ON_NOT_RUNNING_IN_TERMINOLOGY_EXIT_1();

   if (argc <= 1)
     {
        printf("Usage: %s FILE1 [FILE2 ...]\n"
               "  Pop up a given media file/uri right now\n"
               "\n",
               argv[0]);
        return 0;
     }
   for (i = 1; i < argc; i++)
     {
        char *path, buf[PATH_MAX * 2], tbuf[PATH_MAX * 3];

        path = argv[i];
        if (realpath(path, buf)) path = buf;
        bytes = snprintf(tbuf, sizeof(tbuf), "%c}pn%s", 0x1b, path);
        bytes++;
        if (write(STDIN_FILENO, tbuf, bytes) != bytes) perror("write");
     }
   return 0;
}
