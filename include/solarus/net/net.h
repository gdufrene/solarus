#ifdef __cplusplus
extern "C" {
#endif

#include <SDL_net.h>
#include <stdio.h>
// bcopy
#include <strings.h>

struct net_resp {
	int code;
	int content_length;
	char** headers;
	char* body;
};

struct net_request {
	char* querystring;
	char** headers;
	char* body;
};

// int net_open_local();
int net_read_response(int sock, struct net_resp* resp);
int net_main();
int net_http_get(const char* querystring, const char* headers, struct net_resp* resp);
int net_http_post(const char* querystring, const char* body, const char* headers, struct net_resp* resp);
char* net_http_header( const char* header, struct net_resp* resp );

#ifdef __cplusplus
}
#endif

