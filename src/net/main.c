#include "solarus/net/net.h"
#include <stdlib.h>

#define MAX_SOCKETS 10

// -- function declaration --
int net_open_local();
char* read_headers(char *response, struct net_resp* resp);
char* net_http_header( const char* header, struct net_resp* resp );
char* dump_header( const char* header, struct net_resp* resp );
int net_http_query(const char* method, const char* querystring, const char* body, 
	const char* headers, struct net_resp* resp);
// --------------------------

	
int next_ind = 0;
TCPsocket server_socket;
// Client clients[MAX_SOCKETS];
SDLNet_SocketSet socket_set;
TCPsocket sockets[MAX_SOCKETS];

#define CRLF "\r\n"

const char* query = 
		" HTTP/1.1" CRLF
		"Host: localhost:8080" CRLF
		"Connection: close" CRLF
		"User-Agent: Solarus/1.6.x" CRLF
		"Accept: */*" CRLF
		"Accept-Encoding: deflate" CRLF
		;

// struct sockaddr_in s_rem;

int net_open_local() {
	// création d'un descripteur unix pour la socket.
	if (next_ind >= MAX_SOCKETS) {
		printf("No more client socket available\n");
		return -1;
	}

	IPaddress ip;
	if(SDLNet_ResolveHost(&ip,"127.0.0.1", 8080)==-1)
	{
		printf("SDLNet_ResolveHost: %s\n",SDLNet_GetError());
		return -1;
	}

	TCPsocket sock = SDLNet_TCP_Open(&ip);
	if(!sock)
	{
		printf("SDLNet_TCP_Open: %s\n",SDLNet_GetError());
		return -1;
	}

	int i = next_ind;
	sockets[i] = sock;
	next_ind++;
	return i;
}

char* read_headers(char *response, struct net_resp* resp) {
	int endline;
	resp->headers = (char**) malloc(32*sizeof(char*));
	int i = 0;
	char* prev = response;
	char* next = response;
	while ( (next = strstr(prev, CRLF)) > prev ) {
		// printf("%i -> diff = %d\n", i, next - prev);
		next[0] = '\0';
		if (i < 32) resp->headers[i++] = prev;
		prev = next + 2;
	}
	for (;i<32;i++) resp->headers[i] = NULL;
	return next + 2;
}

// expects header with ':'
char* net_http_header( const char* header, struct net_resp* resp ) {
	for ( int i = 0; i < 32; i++ )
			if ( resp->headers[i] != NULL && strstr(header, resp->headers[i]) == resp->headers[i] )
				return resp->headers[i] + strlen(header) + 1;
	return NULL;
}

char* dump_header( const char* header, struct net_resp* resp ) {
	for ( int i = 0; i < 32; i++ )
		if ( resp->headers[i] != NULL )
			printf( "[%d] %s\n", i, resp->headers[i] );
}

int net_read_response(int s, struct net_resp* resp) {
	TCPsocket sock = sockets[s];
	char * buff = (char *) malloc(2000);
	int read = SDLNet_TCP_Recv( sock, buff, 2000);
	if ( read < 0 ) {
		printf("SDLNet_TCP_Recv: %s\n",SDLNet_GetError());
		resp->code = read;
		return read;
	}
	if ( read < 2000 ) buff[read] = '\0';
	int length = -1;
	int chunked = 0;
	char *content = NULL;
	char *chunck_at = NULL;

	resp->body = NULL;
	// printf("%s\n", buff);


	
	if ( read > 0 ) {
		resp->code = atoi( strchr(buff, ' ')+1 );
		buff = strstr(buff, CRLF) + 2;
		
		content = read_headers(buff, resp);
		chunck_at = net_http_header("Transfer-Encoding:", resp); 
		chunked = chunck_at > 0;

		if ( chunked ) {
			if ( content == buff ) {
				strcpy( buff, "chunked without content !");
				resp->content_length = strlen( buff );
				resp->body = buff;
				return resp->code;
			}
			// printf("chunked mode ! (%p)\n", chunck_at);
			resp->content_length = (int) strtol(content, NULL, 16);
			char *nl = strstr(content, "\r\n");
			content = (nl != content ? nl + 2 : content);
			// printf("content => %s\n", content);
		}
		else {
			char* contentLength = net_http_header("Content-Length:", resp); // strstr(buff, "Content-Length: ");
			resp->content_length = (contentLength == 0) ? -1 : atoi( contentLength + 16 );
			// printf( "Content-Length: %d\n", resp->content_length);
		}
	}
	
	resp->body = content;
	if ( resp->body == NULL || resp->content_length == 0 ) return resp->code;

	// printf("RESP: \n%s\n", resp->body);

	int header_size = (int) (resp->body - buff);
	if ( resp->content_length + header_size < 2000 ) {
		buff[ resp->content_length + header_size ] = '\0';
	}

	return resp->code;
}

int net_main() {
	return -1;
}

int net_http_get(const char* querystring, const char* headers, struct net_resp* resp) {
	return net_http_query("GET", querystring, NULL, headers, resp);
}

char* create_http_headers(const char* method, const char* querystring, const char* headers, int content_length) {
	char* buffer = malloc(2000);
	char* ptr = buffer;
	ptr += snprintf(ptr, 2000, "%s %s%s", method, querystring, query );

	// if ( headers != NULL ) printf("headers ... \n%s\n", headers);
	if ( headers != NULL ) ptr += sprintf(ptr, "%s", headers);
	ptr += sprintf( ptr, "Content-Length: %d" CRLF , content_length );
	ptr += sprintf( ptr, "%s", CRLF );
	ptr[0] = 0;
	return buffer;
}

void socket_close(int s)
{
	if ( s > MAX_SOCKETS ) return;
	TCPsocket sock = sockets[s];
	SDLNet_TCP_Close(sock);
	next_ind--;
	if (next_ind < 0) next_ind = 0;
}


int net_http_query(const char* method, const char* querystring, const char* body, 
	const char* headers, struct net_resp* resp) {
	int code = -1;
	int sock = net_open_local();
	if ( sock < 0 ) {
		// printf("open failed return %d\n", sock);
		return sock;
	}

	int content_length = body == NULL ? 0 : strlen(body);
	// printf("headers(2) ... \n%s\n", headers);
	char* buffer = create_http_headers(method, querystring, headers, content_length);

	// code = socketsend(sock, buffer, strlen(buffer), 0);
	code = SDLNet_TCP_Send(sockets[sock], buffer, strlen(buffer));
	// printf("%s %d\n", buffer, code);
	if ( code < 0 ) goto end;
	
	if ( body != NULL ) {
		code = SDLNet_TCP_Send(sockets[sock], body, strlen(body));
		// printf("%s\n", body);
		if ( code < 0 ) goto end;
	}

	code = net_read_response( sock, resp );
	
end:
	free(buffer);
	socket_close( sock );
	return code;
}

int net_http_post(const char* querystring, const char* body, const char* headers, struct net_resp* resp) {
	return net_http_query("POST", querystring, body, headers, resp);
}




