#include <iostream>
#include "xconfclientMocks/fileMock.h"
using namespace std;
extern FileioMock *m_fileIOMock;


// Mock Method
extern "C" FILE* popen(const char * command, const char * type)
{
    if (!m_fileIOMock)
    {
        return NULL;
    }
    return m_fileIOMock->popen(command, type);
}

extern "C" int pclose(FILE * stream)
{
    if (!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->pclose(stream);
}

extern "C" int pipe(int str[2])
{
    if (!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->pipe(str);
}


extern "C" int fclose(FILE * stream)
{
    if (!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->fclose(stream);
}

extern "C" size_t fread (void* ptr, size_t size, size_t nmemb,FILE* stream)
{
    if (!m_fileIOMock)
    {
            return 0;
    }
    return m_fileIOMock->fread(ptr, size, nmemb, stream);
}

extern "C" int fseek(FILE *stream, long offset, int whence)
{
    if (!m_fileIOMock)
    {
            return 0;
    }
    return m_fileIOMock->fseek(stream, offset, whence);
}

/*extern "C" int fprintf(FILE * stream, const char *str, ...)
{
    if(!g_fileIOMock)
    {
	    return 0;
    }
    return g_fileIOMock->fprintf(stream, format);
}
*/
extern "C" ssize_t getline(char **str, size_t *n, FILE * stream)
{
    if(!m_fileIOMock)
    {
          return 0;
    }
    return m_fileIOMock->getline(str, n , stream);
}

extern "C" struct dirent * readdir(DIR *rd)
{
     if(!m_fileIOMock)
     {
            return 0;
     }
     return m_fileIOMock->readdir(rd);
}

extern "C" ssize_t read(int fd, void *buf, size_t count)
{
     if(!m_fileIOMock)
     {
             return 0;
     }
     return m_fileIOMock->read(fd, buf, count);
}

extern "C" FILE* fopen(const char *fp, const char * str)
{
    if(!m_fileIOMock)
    {
        return (FILE*)NULL;
    }
    return m_fileIOMock->fopen(fp, str);
}

extern "C" int closedir(DIR * cd)
{
    if(!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->closedir(cd);
}

extern "C" int mkdir (const char * str, mode_t mode)
{
    if(!m_fileIOMock)
    {
        return 0;
    }
    return  m_fileIOMock->mkdir(str, mode);
}


extern "C" int close( int cd)
{
    if(!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->close(cd);
}

extern "C" DIR *opendir( const char *name)
{
    if(!m_fileIOMock)
    {
        return (DIR *)NULL;
    }
    return  m_fileIOMock->opendir(name);
}

extern "C" int open(const char *pathname, int flags)
{
    if(!m_fileIOMock)
    {
        return 0;
    }
    return m_fileIOMock->open(pathname, flags);
}

extern "C" int fstat(int fd, struct stat *buf)
{
    if(!m_fileIOMock)
    {
            return 0;
    }
    return m_fileIOMock->fstat(fd, buf);
}


extern "C" size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE * stream)
{
    if(!m_fileIOMock)
    {
            return 0;
    }
    return m_fileIOMock->fwrite(ptr, size, nitems, stream);
}

extern "C" CURLcode curl_easy_perform(CURL *easy_handle)
{
     if(!m_fileIOMock)
     {

             return (CURLcode)0;
     }
      return m_fileIOMock->curl_easy_perform(easy_handle);
}

extern "C" struct curl_slist *curl_slist_append(struct curl_slist *list, const char * string)
{
     if(!m_fileIOMock)
     {
             return (struct curl_slist *)NULL;
     }
     return m_fileIOMock->curl_slist_append(list, string);
}

extern "C" CURL* curl_easy_init()
{
     if(!m_fileIOMock)
     {
             return NULL;
     }
     return m_fileIOMock->curl_easy_init();
}

extern "C" int access(const char * pathname, int mode)
{
    if (!m_fileIOMock)
    {
        return -1;
    }
    return m_fileIOMock->access(pathname, mode);
}
