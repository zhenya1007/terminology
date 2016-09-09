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
   int i;

   ON_NOT_RUNNING_IN_TERMINOLOGY_EXIT_1();

   if (argc <= 1)
     {
        printf("Usage: %s FILE1 [FILE2 ...]\n"
               "  Queue a given media file/uri to the popped up\n"
               "\n",
               argv[0]);
        return 0;
     }
   for (i = 1; i < argc; i++)
     {
        char *path, buf[PATH_MAX * 2], tbuf[PATH_MAX * 3];

        path = argv[i];
        if (realpath(path, buf)) path = buf;
        snprintf(tbuf, sizeof(tbuf), "%c}pq%s", 0x1b, path);
        if (write(STDIN_FILENO, tbuf, strlen(tbuf) + 1) != (signed)(strlen(tbuf) + 1)) perror("write");
     }
   return 0;
}
