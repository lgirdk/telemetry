#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "curlinterface.h"
#include "curlinterface.c"
#include "reportprofiles.h"
#include "t2MtlsUtils.h"
#include "t2log_wrapper.h"
#include "t2common.h"
#include "busInterface.h"

#ifdef LIBRDKCONFIG_BUILD
#include "rdkconfig.h"
#endif

#define PREFIX "cJSON Report = "

sigset_t blocking_signal;

void processArguments(int count, char *arguments[]) {
    
    int i;
    T2Info("Number of arguments: %d\n", count);
    for (i = 0; i < count; i++) {
        T2Info("Argument %d: %s\n", i, arguments[i]);
    }
}


int main(int argc, char *argv[])

{

   LOGInit();

   T2Info("http send binary started !!!\n");
   
   if (argc < 2){
       T2Info("less argumenst \n");
		return 0;
    }

    processArguments(argc, argv);

    FILE *fp;
    char *buffer = NULL;
    size_t buffer_size = 0;
    size_t prefix_len = strlen(PREFIX);

    fp = fopen(argv[2], "r");
    if (fp == NULL) {
        T2Error("Error opening file");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    buffer_size = ftell(fp);
    rewind(fp);

    buffer = (char *)malloc(buffer_size + 1);
    if (buffer == NULL) {
        T2Error("Error allocating memory");
        fclose(fp);
        return 1;
    }

    fread(buffer, 1, buffer_size, fp);
    buffer[buffer_size] = '\0';  // Null-terminate the buffer

    fclose(fp);

    if (strncmp(buffer, PREFIX, prefix_len) == 0) {
        char *new_content = strdup(buffer + prefix_len);
        if (new_content == NULL) {
            T2Error("Error duplicating string");
            free(buffer);
            return 1;
        }

        T2ERROR ret = sendReportOverHTTP_bin(argv[1], new_content, NULL);
        if (ret != T2ERROR_SUCCESS) {
            T2Debug("Failed to send report over HTTP:\n");
            free(new_content);
            free(buffer);
            return 1;
        }
        free(new_content);
    }

    free(buffer);

    return 0;

}
