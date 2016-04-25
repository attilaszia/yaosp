/* Return the initial module search path. */

#include "Python.h"
#include "osdefs.h"

#include <sys/types.h>
#include <string.h>

#ifdef __cplusplus
 extern "C" {
#endif

#ifndef VERSION
#define VERSION "2.2"
#endif

#ifndef PREFIX
#  ifdef __VMS
#    define PREFIX ""
#  else
#    define PREFIX "/usr/local"
#  endif
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX PREFIX
#endif

#ifndef PYTHONPATH
#define PYTHONPATH PREFIX "/lib/python" VERSION ":" \
              EXEC_PREFIX "/lib/python" VERSION "/lib-dynload"
#endif

static char prefix[MAXPATHLEN+1];
static char exec_prefix[MAXPATHLEN+1];
static char progpath[MAXPATHLEN+1];
static char *module_search_path = NULL;

static int
isxfile(char *filename)         /* Is executable file */
{
    struct stat buf;
    if (stat(filename, &buf) != 0)
        return 0;
    if (!S_ISREG(buf.st_mode))
        return 0;
    //if ((buf.st_mode & 0111) == 0) // TODO not implemented yet
    //   return 0;
    return 1;
}


/* Add a path component, by appending stuff to buffer.
   buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
   NUL-terminated string with no more than MAXPATHLEN characters (not counting
   the trailing NUL).  It's a fatal error if it contains a string longer than
   that (callers must be careful!).  If these requirements are met, it's
   guaranteed that buffer will still be a NUL-terminated string with no more
   than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
   stuff as fits will be appended.
*/
static void
joinpath(char *buffer, char *stuff)
{
    size_t n, k;
    if (stuff[0] == SEP)
        n = 0;
    else {
        n = strlen(buffer);
        if (n > 0 && buffer[n-1] != SEP && n < MAXPATHLEN)
            buffer[n++] = SEP;
    }
    if (n > MAXPATHLEN)
    	Py_FatalError("buffer overflow in getpath.c's joinpath()");
    k = strlen(stuff);
    if (n + k > MAXPATHLEN)
        k = MAXPATHLEN - n;
    strncpy(buffer+n, stuff, k);
    buffer[n+k] = '\0';
}

/* copy_absolute requires that path be allocated at least
   MAXPATHLEN + 1 bytes and that p be no more than MAXPATHLEN bytes. */
static void
copy_absolute(char *path, char *p)
{
    if (p[0] == SEP)
        strcpy(path, p);
    else {
        getcwd(path, MAXPATHLEN);
        if (p[0] == '.' && p[1] == SEP)
            p += 2;
        joinpath(path, p);
    }
}

/* absolutize() requires that path be allocated at least MAXPATHLEN+1 bytes. */
static void
absolutize(char *path)
{
    char buffer[MAXPATHLEN + 1];

    if (path[0] == SEP)
        return;
    copy_absolute(buffer, path);
    strcpy(path, buffer);
}


static void calculate_path(void)
{
	extern char *Py_GetProgramName(void);

	char *path = getenv("PATH");
	char *prog = Py_GetProgramName();

	strncpy(prefix, PREFIX, MAXPATHLEN);
	strncpy(exec_prefix, EXEC_PREFIX, MAXPATHLEN);
	module_search_path = PYTHONPATH;
	
	/* If there is no slash in the argv0 path, then we have to
	 * assume python is on the user's $PATH, since there's no
	 * other way to find a directory to start the search from.  If
	 * $PATH isn't exported, you lose.
	 */
	if (strchr(prog, SEP)) {		
		strncpy(progpath, prog, MAXPATHLEN);		
	}
	else if (path) {
		while (1) {
			char *delim = strchr(path, DELIM);

			if (delim) {
				size_t len = delim - path;
				if (len > MAXPATHLEN)
					len = MAXPATHLEN;				
				strncpy(progpath, path, len);
				*(progpath + len) = '\0';
			}
			else
				strncpy(progpath, path, MAXPATHLEN);

			joinpath(progpath, prog);
			if (isxfile(progpath))
				break;

			if (!delim) {
				progpath[0] = '\0';
				break;
			}
			path = delim + 1;
		}
	}
	else
		progpath[0] = '\0';
	if (progpath[0] != SEP)
		absolutize(progpath);
}


/* External interface */

char *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

char *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

char *
Py_GetExecPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return exec_prefix;
}

char *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();
    return progpath;
}


#ifdef __cplusplus
}
#endif

