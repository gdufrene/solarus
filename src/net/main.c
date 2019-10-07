// socket, bind, connect ...
#include "solarus/net/net.h"
#include <stdlib.h>



int net_remoteaddress(struct sockaddr_in *s_rem, const char *rhost, int port) {
	struct hostent *h;
	s_rem->sin_family = AF_INET;
	s_rem->sin_port = htons(port);
	// resolution du nom d'hôte
	if((h=gethostbyname(rhost))==0) {
		printf("%s : unknow host ",rhost); 
		return -1;
	}
	// copy de l'adresse IP en fonction de sa taille.
	memcpy( (void *)(h->h_addr), (void *)&(s_rem->sin_addr), h->h_length );
	return 0;
}

#define CRLF "\r\n"

const char* query = 
		" HTTP/1.1" CRLF
		"Host: localhost:8080" CRLF
		"Connection: close" CRLF
		"User-Agent: Solarus/1.6.x" CRLF
		"Accept: */*" CRLF
		"Accept-Encoding: deflate" CRLF
		;

struct sockaddr_in s_rem;

int net_open_local() {
	// création d'un descripteur unix pour la socket.
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	int ret = 0;
	printf("fd socket => %d\n", sock);
	
	ret = net_remoteaddress( &s_rem, "localhost", 8080 );
	if ( ret ) { printf("erreur resolution adresse (%d)\n", ret); goto end; }
	
	// int connect(int descsocket, struct sockaddr* ptsockaddr, int lensockaddr);
	ret = connect( sock, (struct sockaddr*) &s_rem, sizeof(s_rem) );
	if ( ret < 0 ) { printf("erreur connexion socket (%d)\n", ret); goto end; }

	end:
	if ( ret ) {
		close( sock );
		// printf("returns %d\n", ret);
		return ret;
	}
	return sock;
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

int net_read_response(int sock, struct net_resp* resp) {
	char * buff = (char *) malloc(2000);
	int read = recv( sock, buff, 2000, 0);
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
	int code = -1;
	int sock = net_open_local();
	if ( sock < 0 ) return sock;

	send(sock, "GET /", 5, 0);
	code = send(sock, query, strlen(query), 0);
	if ( code < 0 ) goto end;
	
	struct net_resp resp;
	code = net_read_response( sock, &resp );
	// free( resp.body );
	
end:
	close( sock );
	return code;
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
	ptr += sprintf( ptr, "Content-Length: %lu" CRLF , content_length );
	ptr += sprintf( ptr, "%s", CRLF );
	ptr[0] = 0;
	return buffer;
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

	code = send(sock, buffer, strlen(buffer), 0);
	// printf("%s %d\n", buffer, code);
	if ( code < 0 ) goto end;
	
	if ( body != NULL ) {
		code = send(sock, body, strlen(body), 0);
		// printf("%s\n", body);
		if ( code < 0 ) goto end;
	}

	code = net_read_response( sock, resp );
	
end:
	free(buffer);
	close( sock );
	return code;
}

int net_http_post(const char* querystring, const char* body, const char* headers, struct net_resp* resp) {
	return net_http_query("POST", querystring, body, headers, resp);
}




