#ifndef SOLARUS_NET_JSON_H
#define SOLARUS_NET_JSON_H

 #include <functional>

namespace Solarus::Json {
    class Parser {
        public:
        virtual Parser* onNumber(std::function<void(double)>) = 0;
        virtual Parser* onString(std::function<void(char *)>) = 0;
        virtual Parser* onBoolean(std::function<void(bool)>) = 0;
        virtual Parser* onArray(std::function<void(int)>, std::function<void(int)>) = 0;
        virtual Parser* onObject(std::function<void(int)>, std::function<void()>) = 0;
        virtual Parser* onError(std::function<void(const char *)>) = 0;
        virtual void parse(char* json) = 0;
        // virtual ~Parser() = 0;
    };

    Parser* createParser() ;
};

#endif