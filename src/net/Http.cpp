#include "solarus/net/Http.h"
#include "SDL.h"
#include "SDL_net.h"
#include <stdio.h>
#include <functional>

#define CRLF "\r\n"


using namespace std;

class Conn {
    public:
    char * buffer = NULL;
    char * mark = NULL;
    bool stopReading = false;

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
        //DEBUG printf(data);
        return SDLNet_TCP_Send(sock, data, strlen(data));
    }
    int feed() {
        if (this->stopReading) return 0;
        if (buffer == NULL) buffer = (char *) malloc(bufferSize);
        int size = bufferSize - 1;
        char* bufferEnd = buffer + bufferSize;
        if ( mark != NULL && mark > (bufferEnd - 200)) {
            size = bufferEnd - mark;
            memcpy(buffer, mark, size);
            mark = buffer + size;
        } else {
            mark = buffer;
        }
        size = bufferEnd - mark - 1;
        int res = SDLNet_TCP_Recv(sock, buffer, size);
        if ( res < 0 ) {
            throw NetException( string("SDLNet_TCP_Recv: ").append(SDLNet_GetError()).c_str() );
        }
        //DEBUG printf("FEED %d bytes.\n", res);
        buffer[size] = 0; 
        mark = buffer;
        return res;
    }
    std::string readLine() {
        char* endLine;
        if ( mark == NULL ) feed();
        std::string res = std::string();
        while ( (endLine = strstr(mark, CRLF)) == NULL ) {
            res.append(mark);
            int read = feed();
            if ( read == 0 ) return "";
        }
        *(endLine) = 0;
        res.append(mark);
        mark = endLine + 2;
        //DEBUG printf("Readline %lld bytes: %s\n", res.size(), res.c_str());
        return res;
    }
    void onRead( std::function<void(char*)> onPart ) {
        int read;
        do {
            if ( mark != NULL ) {
                onPart(mark);
            }
            read = feed();
        } while( read > 0 );
        free(buffer);
        buffer = NULL;
    }
    ~Conn() {
        if ( buffer != NULL ) {
            free( buffer );
        }
        if ( isOpen ) {
            SDLNet_TCP_Close(sock);
        }
    }
    private:
    IPaddress ip;
    TCPsocket sock;
    bool isOpen = false;
    int bufferSize = 10000;
    
    

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
            //DEBUG printf("host: %s, port: %d\n", hostname.c_str(), port);
            size_t endPath = url.find("?", endHostAndPort);
            if (endPath == string::npos) {
                path = url.substr(endHostAndPort);
                endPath = url.size();
                queryString = string("");
            } else {
                path = url.substr(endHostAndPort+1, endPath - endHostAndPort);
                queryString = url.substr(endPath + 1);
            }
        }
    };

    class ConnResponse : public Response {
        public:
        ConnResponse(Conn *conn) : conn(conn) {
            int i = 0;
            for (i=0; i<32; i++) this->headers[i] = NULL;
            int read = conn->feed();
            if ( read < 0 ) {
                throw new NetException("Unable to read response headers.");
            }
            std::string line = conn->readLine();
            const char * buffer = line.c_str();
            //DEBUG printf("Raw Status Line: %s\n", buffer);
            int majorVersion = 0;
            int minorVersion = 0;
            char message[100] = "";
            int size = sscanf(buffer, "HTTP/%1d.%1d %3d %100s", &majorVersion, &minorVersion, &this->code, message);
            if ( size < 0 ) {
                throw new NetException("Unable to read status line in response.");
            }
            this->message = std::string(message);
            //DEBUG printf("Response [%d] [%s] with Http %d.%d\n", this->code, this->message.c_str(), majorVersion, minorVersion );
            do {
                line = conn->readLine();
                if (i < 32) this->headers[i++] = strdup(line.c_str());
                if ( line.find("Transfer-Encoding") == 0 && line.find("chunked") != std::string::npos ) {
                    chunkedMode = true;
                    //DEBUG printf("Chunked mode activated.\n");
                }
            } while( line.size() > 0 );
        };
        Conn *conn;
        string body() {
            if ( bodyInit ) return bodyContent;
            bodyContent = std::string();
            conn->onRead( chunkedMode ? this->readChunkPart() : this->readBodyPart() );
            bodyInit = true;
            return bodyContent;
        };
        ~ConnResponse() {
            delete conn;
        }
        private:
        bool bodyInit = false;
        bool chunkedMode = false;
        string bodyContent;
        unsigned int chunkSize = 0;
        
        function<void(char*)> readBodyPart() {
            return [&] (char* part) { bodyContent.append(part); };
        }
        function<void(char*)> readChunkPart() {
            return [&] (char* part) {
                newChunk:
                if ( chunkSize == 0 ) {
                    std::string line = conn->readLine();
                    if ( line.size() == 0 ) {
                        conn->stopReading = true;
                        return;
                    }
                    chunkSize = stoul(line, NULL, 16);
                    //DEBUG printf("Next chunk size: %d\n", chunkSize);
                    if ( chunkSize == 0 ) {
                        conn->stopReading = true;
                        return;
                    }
                    // return;
                    part += line.size() + 2;
                }
                unsigned int len = strlen(part);
                unsigned int size = len > chunkSize ? chunkSize : len ;
                bodyContent.append(part, size);
                chunkSize -= size;
                //DEBUG printf("Chunk size left: %d\n", chunkSize);
                if ( chunkSize == 0 ) {
                    conn->mark += size;
                    goto newChunk;
                }
            };
        }

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
        //DEBUG printf("GET %s\n", urlParam.c_str());
        return query("GET", urlParam, "", "");
    }

    Response* query(string method, string urlParam, string additionalHeaders, string body) {
        Url url = Url(urlParam);
        Conn *conn = new Conn(url.hostname, url.port);
        //DEBUG printf("connection to %s : %d\n", url.hostname.c_str(), url.port);
        string head = string();
        char* header = genHeaders(method, url);
        if ( !body.empty() ) {
            additionalHeaders = additionalHeaders
                .append("Content-Length: ")
                .append(std::to_string(body.size()))
                .append(CRLF);
        }
        strcat(header, additionalHeaders.c_str());
        strcat(header, CRLF);
        //DEBUG printf("___ Generated headers: \n%s\n", header);
        if ( conn->send( header ) < 0 ) {
            free(header);
            throw NetException("unable to send request");
        }
        free(header);
        if ( !body.empty() && conn->send( body.c_str() ) < 0 ) {
            throw NetException("unable to send body");
        }
        //DEBUG if ( !body.empty() ) printf("Body sent.\n");
        //DEBUG printf("return new connResponse.\n");
        return new ConnResponse( conn );
    };
    
}; // namespace Net::Http

