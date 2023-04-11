#ifndef MOCK_FILE_IO_H
#define MOCK_FILE_IO_H
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <curl/curl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class FileIOInface
{
    public:
        virtual ~FileIOInface() {}
        virtual int access(const char *pathname , int mode) = 0;
        virtual FILE * popen(const char *, const char *) = 0;
        virtual int pclose(FILE *) = 0;
        virtual int pipe(int *) = 0;
        virtual int fclose(FILE *) = 0;
        virtual ssize_t getline(char **, size_t *, FILE *) = 0;
        virtual struct dirent * readdir(DIR *) = 0;
        virtual FILE * fopen(const char *, const char *) = 0;
        virtual int closedir(DIR *) = 0;
        virtual int mkdir(const char *, mode_t) = 0;
        virtual DIR* opendir(const char* name) = 0;
        virtual int close(int) = 0;
        virtual int open(const char *, mode_t) = 0;
        virtual int fstat(int fd, struct stat *buf) = 0;
        virtual ssize_t read(int fd, void *buf, size_t count) = 0;
        virtual struct curl_slist *curl_slist_append(struct curl_slist *list, const char * string) = 0;
        virtual int fseek(FILE *stream, long offset, int whence) = 0;
        virtual size_t fwrite(const void* ptr, size_t size, size_t nmemb,FILE* stream) = 0;
        virtual size_t fread(void* ptr, size_t size, size_t nmemb,FILE* stream) = 0;
        virtual CURLcode curl_easy_perform(CURL *easy_handle) = 0;
        virtual CURL* curl_easy_init() = 0;
};

class FileioMock: public FileIOInface
{
    public:
        virtual ~FileioMock() {}
	MOCK_METHOD2(access, int(const char *, int));
        MOCK_METHOD2(popen, FILE *(const char *, const char *));
        MOCK_METHOD1(pclose, int (FILE *));
        MOCK_METHOD1(pipe, int(int *));
        MOCK_METHOD1(fclose, int (FILE *));
        MOCK_METHOD3(getline, ssize_t (char **, size_t *, FILE *));
        MOCK_METHOD1(opendir, DIR*(const char*));
        MOCK_METHOD1(readdir, struct dirent *(DIR *));
        MOCK_METHOD2(fopen, FILE *(const char *, const char *));
        MOCK_METHOD1(closedir, int (DIR *));
        MOCK_METHOD2(mkdir, int (const char *, mode_t));
        MOCK_METHOD3(read, ssize_t(int, void *, size_t));
        MOCK_METHOD2(fstat, int (int , struct stat *));
        MOCK_METHOD1(close, int(int));
        MOCK_METHOD2(open, int(const char *, mode_t));
        MOCK_METHOD4(fwrite, size_t(const void *, size_t, size_t, FILE *));
        MOCK_METHOD2(curl_slist_append, struct curl_slist *(struct curl_slist *, const char *));
        MOCK_METHOD4(fread, size_t(void* , size_t, size_t, FILE*));
        MOCK_METHOD3(fseek, int(FILE *, long, int));
        MOCK_METHOD1(curl_easy_perform, CURLcode(CURL *));
        MOCK_METHOD0(curl_easy_init, CURL*());

};

#endif
