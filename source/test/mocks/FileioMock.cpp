#include <iostream>
#include "test/mocks/FileioMock.h"
using namespace std;
extern FileIOMock *g_fileIOMock;

// Mock Method
/*extern "C" char * fgets(char * str, int n, FILE * stream)
{
    if (!g_fileIOMock)
    {
        return NULL;
    }
    return g_fileIOMock->fgets(str, n, stream);
}

extern "C" FILE * popen(const char * command, const char * type)
{
    if (!g_fileIOMock)
    {
        return NULL;
    }
    return g_fileIOMock->popen(command, type);
}

extern "C" int pclose(FILE * stream)
{
    if (!g_fileIOMock)
    {
        return 0;
    }
    return g_fileIOMock->pclose(stream);
}*/

extern "C" int fclose(FILE * stream)
{
    if (!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->fclose(stream);
}

/*extern "C" int fprintf(FILE * stream, const char *str, ...)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->fprintf(stream, str, ...);
}*/

extern "C" ssize_t getline(char **str, size_t *n, FILE * stream)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->getline(str, n , stream);
}

extern "C" struct dirent * readdir(DIR *rd)
{
     if(!g_fileIOMock)
     {
	     return 0;
     }
     return g_fileIOMock->readdir(rd);
}

extern "C" ssize_t read(int fd, void *buf, size_t count)
{
     if(!g_fileIOMock)
     {
             return 0;
     }
     return g_fileIOMock->read(fd, buf, count);
}
 
extern "C" FILE * fopen(const char *fp, const char * str)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->fopen(fp, str);
}

extern "C" int closedir(DIR * cd)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->closedir(cd);
}

extern "C" int mkdir (const char * str, mode_t mode)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return  g_fileIOMock->mkdir(str, mode);
}

/*extern "C" ssize_t read ( int n , void * ptr, size_t s)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->read(n, ptr, s);
}*/

extern "C" int close( int cd)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->close(cd);
}

extern "C" DIR *opendir( const char *name)
{
    if(!g_fileIOMock)
    {
        return (DIR *)NULL;
    }
    return  g_fileIOMock->opendir(name);
}

extern "C" int open(const char *pathname, int flags)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->open(pathname, flags);
}

extern "C" int fstat(int fd, struct stat *buf)
{
    if(!g_fileIOMock)
    {
            return 0;
    }
    return g_fileIOMock->fstat(fd, buf);
}


extern "C" size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE * stream)
{
    if(!g_fileIOMock)
    {
            return 0;
    }
    return g_fileIOMock->fwrite(ptr, size, nitems, stream);
}

/*extern "C" CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* destURL)
{
     if(!g_fileIOMock)
     {
     
             return 0;
     }
     return g_fileIOMock->curl_easy_setopt(curl, option, destURL);
}*/

extern "C" struct curl_slist *curl_slist_append(struct curl_slist *list, const char * string)
{ 
     if(!g_fileIOMock)
     {
             return NULL;
     }
     return g_fileIOMock->curl_slist_append(list, string);
}

/*extern "C" CURL* curl_easy_init()
{
     if(!g_fileIOMock)
     {
             return NULL;
     }
     return g_fileIOMock->curl_easy_init();
}

extern "C" CURLcode curl_easy_getinfo(CURL curl, CURLINFO info, ...)
{
     if(!g_fileIOMock)
     {
             return CURLE_COULDNT_CONNECT;
     }
     return g_fileIOMock->curl_easy_getinfo(curl, info, ...);
}
*/
