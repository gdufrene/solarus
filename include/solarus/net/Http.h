#ifndef SOLARUS_NET_HTTP_H
#define SOLARUS_NET_HTTP_H

#include <string>

#define MAX_SOCKETS 10

using namespace std;

class NetException: exception {
    public:
    NetException(const char* msg) throw() :message(msg) {} 
    const char* what() const throw() { 
        return message;
    }
    private:
    const char* message;
};

namespace Solarus::Http {
    class Query {
        public:
        string method;
        string hostname;
        string path;
        int port;
        string queryString;

    };

    class Response {
        public:
        char* headers[32];
        int code = -1;
        string message;

        virtual string body() = 0;
        Response() {
            for(int i = 0; i < 32; i++) headers[i] = NULL;
        }
        // virtual ~Response() = 0;
    };

    Response* get(string urlParam);
    Response* query(string method, string url, string headers, string body);

};

#endif