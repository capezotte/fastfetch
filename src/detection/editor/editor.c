#include "editor.h"
#include "common/processing.h"
#include "util/stringUtils.h"

#include <stdlib.h>

#if defined(MAXPATH)
#define FF_EXE_PATH_LEN MAXPATH
#elif defined(PATH_MAX)
#define FF_EXE_PATH_LEN PATH_MAX
#else
#define FF_EXE_PATH_LEN 260
#endif

#ifdef _WIN32
static inline char* realpath(const char* restrict file_name, char* restrict resolved_name)
{
    char* result = _fullpath(resolved_name, file_name, _MAX_PATH);
    if(result)
    {
        resolved_name[1] = resolved_name[0]; // Drive Name
        resolved_name[0] = '/';
    }
    return result;
}
#endif

const char* ffDetectEditor(FFEditorResult* result)
{
    ffStrbufSetS(&result->name, getenv("VISUAL"));
    if (result->name.length == 0)
        ffStrbufSetS(&result->name, getenv("EDITOR"));
    if (result->name.length == 0)
        return "$VISUAL or $EDITOR not set";

    #ifndef _WIN32
    if (result->name.chars[0] != '/')
    {
        if (ffProcessAppendStdOut(&result->path, (char* const[]){
            FASTFETCH_TARGET_DIR_USR "/bin/which",
            result->name.chars,
            NULL,
        }) != NULL || result->path.length == 0)
            return NULL;
    }
    else
        ffStrbufSet(&result->path, &result->name);

    char buf[FF_EXE_PATH_LEN];
    if (!realpath(result->path.chars, buf))
        return NULL;

    ffStrbufSetS(&result->path, buf);
    const char* exe = &result->path.chars[ffStrbufLastIndexC(&result->path, '/')];
    if (!*exe) return NULL;
    ++exe;
    result->exe = exe;

    const char* param = NULL;
    if (
        ffStrEquals(exe, "nano") ||
        ffStrEquals(exe, "vim") ||
        ffStrEquals(exe, "nvim") ||
        ffStrEquals(exe, "micro") ||
        ffStrEquals(exe, "emacs") ||
        ffStrStartsWith(exe, "emacs-") || // emacs-29.3
        ffStrEquals(exe, "hx") ||
        ffStrEquals(exe, "code") ||
        ffStrEquals(exe, "sublime_text")
    ) param = "--version";
    else if (
        ffStrEquals(exe, "kak") ||
        ffStrEquals(exe, "pico")
    ) param = "-version";
    else if (
        ffStrEquals(exe, "ne")
    ) param = "-h";
    else return NULL;

    ffProcessAppendStdOut(&result->version, (char* const[]){
        result->path.chars,
        (char*) param,
        NULL,
    });

    if (result->version.length == 0)
        return NULL;

    ffStrbufSubstrBeforeFirstC(&result->version, '\n');
    for (uint32_t iStart = 0; iStart < result->version.length; ++iStart)
    {
        char c = result->version.chars[iStart];
        if (c >= '0' && c <= '9')
        {
            for (uint32_t iEnd = iStart + 1; iEnd < result->version.length; ++iEnd)
            {
                char c = result->version.chars[iEnd];
                if (isspace(c))
                {
                    ffStrbufSubstrBefore(&result->version, iEnd);
                    break;
                }
            }
            if (iStart > 0)
                ffStrbufSubstrAfter(&result->version, iStart - 1);
            break;
        }
    }
    #endif

    return NULL;
}
