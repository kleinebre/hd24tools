/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc.  Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation version
 * 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * END COPYRIGHT BLOCK **/
#ifdef WINDOWS
#define XP_WIN32
#endif
#ifndef MAX_PATH
#define MAX_PATH 127
#endif
#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifdef XP_WIN32
  #include <windows.h>
  #include <regstr.h>
  #include <direct.h>
  #include <io.h>    /* For _findfirst */
#else


  #include <sys/types.h>
  #include <sys/statvfs.h>
  #include <sys/socket.h>         /* socket, bind */
  #include <netinet/in.h>         /* htonl */
  #include <netdb.h>              /* gethostbyname */
  #include <arpa/inet.h>          /* inet_ntoa */
  #include <dirent.h>
  #include <assert.h>

#endif


#define MINPORT       1024
#define MAXPORT       65535



  /*********************************************************************
  **
  ** FUNCTION: setupIsDirEmpty
  ** Replaced: IsDirEmpty
  **
  ** DESCRIPTION: checks to see if directory empty
  **
  **
  ** INPUTS:
  **
  ** RETURN:
  **   TRUE or FALSE
  **
  ** SIDE EFFECTS:
  **      none
  ** RESTRICTIONS:
  **      None
  ** MEMORY:
  **********************************************************************/

int setupIsDirEmpty(const char * pszPath)
  {
#ifdef XP_WIN32
    int iReturn = TRUE;
    char szFileSpec[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE ff;

    if (!(pszPath))
    {
      //setupLog(NULL, "setupIsDirEmpty failed: passed in NULL parameter for directory Name");
      return FALSE;
    }
    snprintf(szFileSpec, sizeof(szFileSpec), "%s\\*", pszPath);
    if (!memchr(szFileSpec, 0, sizeof(szFileSpec))) return FALSE; /*overflow*/

    ff = FindFirstFile(szFileSpec, &fd);
    if (ff != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
        {
          iReturn = FALSE;
          break;
        }
      } while (FindNextFile(ff, &fd));
      FindClose(ff);
    }

    return (iReturn);
#else
    DIR *dirp;
    struct dirent *dp;
    bool rc = TRUE;

    dirp = opendir(pszPath);
    while ((dp = readdir(dirp)) != NULL)
    {
      if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
      {
        rc = FALSE;
        break;
      }
    }
    closedir(dirp);
    return rc;
#endif

  }

  /*********************************************************************
  **
  ** FUNCTION: setupFileExist
  ** Replaced: FileExist
  **
  ** DESCRIPTION: returns 1 if file exists.
  **
  ** INPUTS:
  **
  ** RETURN:
  **   TRUE or FALSE
  **
  ** SIDE EFFECTS:
  **      none
  ** RESTRICTIONS:
  **      None
  ** MEMORY:
  **********************************************************************/
int setupFileExists(const char * pszFileName)
  {
    struct stat fi;

    if ((stat (pszFileName, &fi) != -1) && ((fi.st_mode & S_IFDIR) == 0))
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }


  /*********************************************************************
  **
  ** FUNCTION: setupDirExists
  ** Replaced: DirExists
  **
  ** DESCRIPTION: checks to see if directory exists
  **
  **
  ** INPUTS:
  **
  ** RETURN:
  **   TRUE or FALSE
  **
  ** SIDE EFFECTS:
  **      none
  ** RESTRICTIONS:
  **      None
  ** MEMORY:
  **********************************************************************/
int setupDirExists(const char * pszDirName)
  {
#ifdef XP_WIN32
    unsigned int dwAttrs;
    if (!(pszDirName))
    {
      //setupLog(NULL, "DirExists failed: passed in NULL parameter for directory");
      return FALSE;
    }
    dwAttrs = GetFileAttributes(pszDirName);
    if ((dwAttrs != 0xFFFFFFFF) && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY))
    {
      return TRUE;
    }
    return FALSE;
#else
    struct stat fi;

    if (stat (pszDirName, &fi) == -1 || !S_ISDIR(fi.st_mode))
    {
      return FALSE;
    }
    else
    {
      return TRUE;
    }
#endif
  }

  /*********************************************************************
  **
  ** FUNCTION: setupCreateDir
  ** Replaced: CreateDir
  **
  ** DESCRIPTION: creates a directory
  **
  **
  ** INPUTS:
  **
  ** RETURN:
  **   NT:  TRUE or FALSE
  **   UNIX:
  **       0 : OK
  **      -1 : Can't create directory
  **      -2 : input exists and is not a directory
  **      -3 : Can't write to directory
  **
  ** SIDE EFFECTS:
  **      none
  ** RESTRICTIONS:
  **      None
  ** MEMORY:
  **
  **********************************************************************/
bool setupCreateDir(const char * pszDirName, int mode)
  {
#ifdef XP_WIN32
    char *pszDelim;
    char szDirName[MAX_PATH];
    char szTmpDir[MAX_PATH] = "";
    int  cbLen;

    if (!(pszDirName))
    {
      //setupLog(NULL, "CreateDir failed: passed in NULL parameter for directory");
      return FALSE;
    }

    if (strlen(pszDirName) >= sizeof(szDirName))
    {
      //setupLog(NULL, "CreateDir failed: passed in too long directory");
      return FALSE;
    }

    snprintf(szDirName, sizeof(szDirName), "%s", pszDirName);
    if (!memchr(szDirName, 0, sizeof(szDirName))) return FALSE; /*overflow*/
    pszDelim = szDirName;
    while ((pszDelim = strchr(pszDelim, '/')) != NULL)
    {
      *pszDelim = '\\';
    }

    pszDelim = szDirName;
    while ((pszDelim = strchr(pszDelim, '\\')) != NULL)
    {
      cbLen = (pszDelim - szDirName);
      strncpy(szTmpDir, szDirName, cbLen);
      szTmpDir[cbLen] = '\0';
      CreateDirectory(szTmpDir, NULL);
      pszDelim++;
    }

    if (!CreateDirectory(szDirName, NULL))
    {
//      char szLog[MAX_PATH+50];
	return FALSE;
//      snprintf(szLog, sizeof(szLog),
//                      "setupCreateDir failed to create '%s'", pszDirName);
//      if (!memchr(szLog, 0, sizeof(szLog))) return FALSE; /*overflow*/
      //setupLog(NULL, szLog);
    }
    return TRUE;
#else
    struct stat fi;
    char *s;
    char *t;

    s = strdup(pszDirName);
    t = s + 1;

    while (1)
    {
      t = strchr(t, '/');

      if (t)
        *t = '\0';
      if (stat(s, &fi) == -1)
      {
        if (mkdir(s, mode) == -1)
        {
          return FALSE;
        }
      }
      else if (!S_ISDIR(fi.st_mode))
      {
        return FALSE;
      }
      if (t)
        *t++ = '/';
      else
        break;
    }

    if (access(pszDirName, W_OK) == -1)
    {
      return FALSE;
    }
    return TRUE;
#endif
  }
  /*********************************************************************
  **
  ** FUNCTION: setupConvertsPathSlashes
  ** Replaced: ConvertsPathSlashes
  **
  ** DESCRIPTION: converts slashes TRUE UNIX to NT, FALSE NT to UNIX,
  **
  ** INPUTS:
  **
  ** RETURN:
  **   TRUE or FALSE
  **
  ** SIDE EFFECTS:
  **      none
  ** RESTRICTIONS:
  **      None
  ** MEMORY:
  **********************************************************************/
int   setupConvertPathSlashes(const char *  lpszPath, char *  lpszNewPath, int bType )
  {
    if (lpszPath == NULL)
      return FALSE;

    /* create reverse slashes and escape them */
    while (*lpszPath)
    {
      if ((*lpszPath == '\\') || (*lpszPath == '/'))
      {
        if (bType)
          *lpszNewPath = '\\';
        else
          *lpszNewPath = '/';
      }
      else
        *lpszNewPath = *lpszPath;
      lpszPath++;
      lpszNewPath++;
    }
    return TRUE;
  }


unsigned int setupExecProgram(const char *program, const char *where)
{
   char curdir[MAX_PATH];
   unsigned int ret;
#ifdef XP_WIN32
   char szWhere[MAX_PATH];
   char szProgram[MAX_PATH];
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   DWORD dwCreationFlags;
#endif

   if (!program)
   {
      //setupLogMessage("info", "setup", "setupExecProgram called with no program");
      return 0;
   }
   if (!setupFileExists(program))
   {
      //setupLogMessage("info", "setup", "setupExecProgram called with invalid program %s", program);
   }
   getcwd(curdir, sizeof(curdir));

   if (where)
   {
      chdir(where);
   }
#ifndef XP_WIN32
   ret = system(program);
#else
   snprintf(szProgram, sizeof(szProgram), "%s", program);
   if (!memchr(szProgram, 0, sizeof(szProgram))) {
      //setupLogMessage("info", "setup", "Too long program %s", program);
      return 0;
   }
   snprintf(szWhere, sizeof(szWhere), "%s", where);
   if (!memchr(szWhere, 0, sizeof(szWhere))) {
      //setupLogMessage("info", "setup", "Too long arg %s", where);
      return 0;
   }
   memset(&si,0,sizeof(si));
   si.cb = sizeof(si);
   GetStartupInfo(&si);
   si.wShowWindow = SW_SHOWDEFAULT;
   dwCreationFlags = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS;
   ret = CreateProcess(NULL, szProgram, NULL, NULL, FALSE, dwCreationFlags, NULL, szWhere, &si, &pi);
#endif

   if (where)
   {
      chdir(curdir);
   }

   return ret;
}


/*********************************************************************
**
** FUNCTION:      setupGetDiskFreeSpace
** DESCRIPTION:   get available disk space in Kilobyte
**
** INPUTS:        Path name
** OUTPUTS:
** RETURN:        available disk space in kilobyte
** SIDE EFFECTS:
**      None
** RESTRICTIONS:
**      None
** MEMORY:
**********************************************************************
*/

unsigned long setupGetDiskFreeSpace(const char *path)
{
#ifdef XP_WIN32
  DWORD dwSectorsPerCluster;
  DWORD dwBytesPerSector;
  DWORD dwFreeClusters;
  DWORD dwTotalClusters;

  if (GetDiskFreeSpace(path, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters))
  {
     long long clusterSize;

     clusterSize = dwSectorsPerCluster * dwBytesPerSector;

     return (unsigned long)((clusterSize / 1024.0) * dwFreeClusters);
  }

  return 0;
#else
  struct statvfs buf;

  statvfs(path, &buf);

  if (buf.f_frsize == 0)
    return (unsigned long)(((unsigned long long)buf.f_bavail / 1024.0) * buf.f_bsize);
  else
    return (unsigned long)(((unsigned long long)buf.f_bavail / 1024.0) * buf.f_frsize);
#endif
}

