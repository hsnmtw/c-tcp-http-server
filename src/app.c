#include <stdio.h>
#include "http.h"

int main(void) {
    printf("[app] Starting web app ...\n");

    http_start_server();

    printf("-------------------- [exit: 0]\n");
    return 0;
}