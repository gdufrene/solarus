#include "solarus/net/Http.h"
#include "SDL.h"
#include "SDL_net.h"
#include <stdio.h>
#include <functional>

#define CRLF "\r\n"


using namespace std;

class Conn {
    public:
    Conn(string host, int port) {
        if(SDLNet_ResolveHost(&ip, host.c_str(), port)==-1)
        {
            throw NetException( string("SDLNet_ResolveHost: ").append(host).append(SDLNet_GetError()).c_str() );
        }

        sock = SDLNet_TCP_Open(&ip);
        if(!sock)
        {
            throw NetException( string("SDLNet_TCP_Open: ").append(SDLNet_GetError()).c_str() );
        }
        isOpen = true;
    }
    int send(const char* data) {
        return SDLNet_TCP_Send(sock, data, strlen(data));
    }
    void onRead( std::function<void(char*)> onPart ) {
        char * buff = (char *) malloc(2000);
        int read;
        while ( (read = SDLNet_TCP_Recv(sock, buff, 1999)) > 0 ) {
            buff[read] = 0;
            onPart(buff);
        }
        free(buff);
        if ( read < 0 ) {
            throw NetException( string("SDLNet_TCP_Recv: ").append(SDLNet_GetError()).c_str() );
        }
    }
    ~Conn() {
        if ( isOpen ) SDLNet_TCP_Close(sock);
    }
    private:
    IPaddress ip;
    TCPsocket sock;
    bool isOpen = false;
    

};

namespace Solarus::Http
{

    class Url {
        public:
        string hostname;
        int port;
        string path;
        string queryString;

        Url(string url) {
            parse(url);
        }

        void parse(string url) {
            size_t beginHost = url.find("//", 0);
            if (beginHost == string::npos) beginHost = 0;
            else beginHost += 2;
            size_t endHostAndPort = url.find("/", beginHost);
            if (endHostAndPort == string::npos) endHostAndPort = url.size();
            size_t beginPort = url.find(":", beginHost);
            port = 80;
            size_t endHost = endHostAndPort;
            if ( beginPort != string::npos && beginPort < endHostAndPort ) {
                beginPort++;
                port = stoi( url.substr(beginPort, endHostAndPort - beginPort) );
                endHost = beginPort - 1;
            }
            hostname = url.substr(beginHost, endHost - beginHost);
            printf("host: %s, port: %d\n", hostname.c_str(), port);
            size_t endPath = url.find("?", endHostAndPort);
            if (endPath == string::npos) {
                path = string("/");
                endPath = url.size();
                queryString = string("");
            } else {
                path = url.substr(endHostAndPort+1, endPath - endHostAndPort);
                queryString = url.substr(endPath + 1, url.size() - endPath);
            }
        }
    };

    class ConnResponse : public Response {
        public:
        ConnResponse(Conn *conn) : conn(conn) {} ;
        Conn *conn;
        string body() {
            if ( bodyInit ) return bodyContent;
            conn->onRead( [&] (char* part) { bodyContent.append(part); } );
            bodyInit = true;
            return bodyContent;
        };
        ~ConnResponse() {
            delete conn;
        }
        private:
        bool bodyInit = false;
        string bodyContent;

    };

    const char* commonHeaders = 
		// " HTTP/1.1" CRLF
		// "Host: localhost:8080" CRLF
		"Connection: close" CRLF
		"User-Agent: Solarus/1.6.x" CRLF
		"Accept: */*" CRLF
		"Accept-Encoding: deflate" CRLF
		;

    char* genHeaders(string method, Url url) {
        char* from = (char*) malloc(2000);
        char* res = from;
        res += sprintf(res, "%s %s", method.c_str(), url.path.c_str());
        if ( url.queryString.size() > 0 ) {
            res += sprintf(res, "?%s", url.queryString.c_str());
        }
        res += sprintf(res, " HTTP/1.1" CRLF
            "Host: %s:%d" CRLF, url.hostname.c_str(), url.port);
        res += sprintf(res, commonHeaders);
        return from;
    }

    Response* get(string urlParam) {
        return query("GET", urlParam, "", "");
    }

    Response* query(string method, string urlParam, string additionalHeaders, string body) {
        Url url = Url(urlParam);
        Conn *conn = new Conn(url.hostname, url.port);
        string head = string();
        char* header = genHeaders(method, url);
        strcat(header, additionalHeaders.c_str());
        strcat(header, CRLF);
        printf("%s\n", header);
        if ( conn->send( header ) < 0 ) {
            free(header);
            throw NetException("unable to send request");
        }
        free(header);
        if ( !body.empty() && conn->send( body.c_str() ) < 0 ) {
            throw NetException("unable to send body");
        }

        return new ConnResponse( conn );
    };
    
}; // namespace Net::Http

