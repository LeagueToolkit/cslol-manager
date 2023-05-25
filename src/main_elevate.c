#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <mach-o/dyld.h>
#include <limits.h>
#include <libgen.h>

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

    char *str;
    int len = snprintf(
        NULL, 
        0, 
        "do shell script \"sudo %s/cslol-manager >/dev/null 2>&1 &\""
        " with prompt \"CSLOL-Manager needs administrator privileges to restart.\""
        " with administrator privileges"
        " without altering line endings",
        parentDir
    );

    //
    //  Something went wrong trying
    //  to obtain the necessary length of the string.
    //
    if (len <= 0)
        return 1;

    str = (char *)malloc(len + 1);

    snprintf(
        str, 
        len + 1,
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

    free(str);

    return code;
}