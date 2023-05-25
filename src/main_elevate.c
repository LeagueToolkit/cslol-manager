#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <libgen.h>

#define COMMAND_CHAR_COUNT_MAX  1000 + PATH_MAX

int main()
{
    char pathBuff[PATH_MAX];
    uint32_t buffSize = PATH_MAX;

    //
    //  Find the absolute path
    //  of this executable.
    //
    if (_NSGetExecutablePath(pathBuff, &buffSize) != KERN_SUCCESS)
        return -1;

    //
    //  Remove the file component of the abs. path.
    //
    char *parentDir = dirname(pathBuff);

    char str[COMMAND_CHAR_COUNT_MAX];

    sprintf(
        str, 
        "do shell script \"sudo %s/cslol-manager >/dev/null 2>&1 &\""
        " with prompt \"CSLOL-Manager needs administrator privileges to restart.\""
        " with administrator privileges"
        " without altering line endings",
        parentDir
    );

    int code = execlp (
        "osascript",
        "osascript",
        "-e",
        str,
        NULL);

    return code;
}