#include "solarus/net/Http.h"
#include <stdio.h>


int main(int argr, char** argv) {
    Solarus::Http::Url url = Solarus::Http::Url("http://localhost:8080/cave/sum?a=18&b=53");
    printf("hostname %s\n", url.hostname.c_str());
    printf("path %s\n", url.path.c_str());
    printf("port %d\n", url.port);
    printf("queryString %s\n", url.queryString.c_str());
    return 0;
}