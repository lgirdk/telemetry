/*
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef MOCK_FILE_IO_H
#define MOCK_FILE_IO_H
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <curl/curl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class FileIOInterface
{
    public:
        virtual ~FileIOInterface() {}
        //virtual char * fgets(char *, int, FILE *) = 0;
        //virtual FILE * popen(const char *, const char *) = 0;
        //virtual int pclose(FILE *) = 0;
	virtual int fclose(FILE *) = 0;
	//virtual int fprintf(FILE *, const char *, ...) = 0;
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
	virtual size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE * stream) = 0;
//	virtual CURLcode curl_easy_setopt(CURL *handle, CURLoption option, const char* parameter) = 0;
	virtual struct curl_slist *curl_slist_append(struct curl_slist *list, const char * string) = 0;
 //       virtual CURL* curl_easy_init() = 0;
//	virtual CURLcode curl_easy_getinfo(CURL curl, CURLINFO info, ...);
};

class FileIOMock: public FileIOInterface
{
    public:
        virtual ~FileIOMock() {}
       // MOCK_METHOD3(fgets, char *(char *, int, FILE *));
        //MOCK_METHOD2(popen, FILE *(const char *, const char *));
        //MOCK_METHOD1(pclose, int (FILE *));
	MOCK_METHOD1(fclose, int (FILE *));
       // MOCK_METHOD3(fprintf, int (FILE *, const char *, ...);
        MOCK_METHOD3(getline, ssize_t (char **, size_t *, FILE *));
        MOCK_METHOD1(opendir, DIR*(const char*));
  	MOCK_METHOD1(readdir, struct dirent *(DIR *));
	MOCK_METHOD2(fopen, FILE *(const char *, const char *));
	MOCK_METHOD1(closedir, int (DIR *));
	MOCK_METHOD2(mkdir, int (const char *, mode_t));
        //MOCK_METHOD2(read, ssize_t(int, void *, size_t));
	MOCK_METHOD2(fstat, int (int , struct stat *));
	MOCK_METHOD1(close, int(int));
	MOCK_METHOD2(open, int(const char *, mode_t));
	MOCK_METHOD3(read, ssize_t(int, void *, size_t));
	MOCK_METHOD4(fwrite, size_t(const void *, size_t, size_t, FILE *));
//	MOCK_METHOD3(curl_easy_setopt, CURLcode(CURL*, CURLoption, const char*));
	MOCK_METHOD2(curl_slist_append, struct curl_slist *(struct curl_slist *, const char *));
//	MOCK_METHOD(curl_easy_init, CURL*());
//	MOCK_METHOD3(curl_easy_getinfo, CURLcode(CURL, CURLINFO, ...));
};

#endif

