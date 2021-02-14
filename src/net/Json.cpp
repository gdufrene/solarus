#include "solarus/net/Json.h"
#include "solarus/third_party/json/jsmin.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace Solarus::Json {

    class _Parser: public Parser {
        private:
        jsmntok_t *tokens;
        jsmn_parser parser;

        std::function<void(double)> onNumberFn;
        std::function<void(std::string)> onStringFn;
        std::function<void(bool)> onBooleanFn;
        std::function<void(int)> onArrayFn, onObjectFn;
        std::function<void(std::string)> onErrorFn;
        std::function<void(int)> afterArrayElm;
        std::function<void()> afterObjectElm;

/*
        void push_json_object(char *json);
        void push_json_value(char *json);
        void push_json_array(char *json);
*/

        void push_json_array(const char *json) {
            int size = tokens->size;
            //DEBUG printf("push array size %d\n", size);
            int idx = 1;
            onArrayFn(size);
            while (size-- > 0) {
                tokens++;
                push_json_value(json);
                afterArrayElm(idx++);
            }
        }

        void push_json_value(const char *json) {
            const char *ptr = json + tokens->start;
            //DEBUG printf("Push value\n");

            if ( tokens->type == JSMN_STRING ) {
                // json[ tokens->end ] = '\0';
                std::string str = std::string(ptr, tokens->end - tokens->start);
                //DEBUG printf("  string -> %s\n", str.c_str());
                // lua_pushstring(l, ptr);
                return onStringFn(str);
            }
            
            if ( tokens->type == JSMN_PRIMITIVE ) {
                std::string str = std::string(ptr, tokens->end - tokens->start);
                char c = str.at(0);
                if ( c >= '0' && c <= '9' ) {
                    double value;
                    // json[ tokens->end ] = '\0';
                    // strncpy( buffer, ptr, tokens[idx].end - tokens[idx].start );
                    value = std::stod(str);
                    //DEBUG printf("  number -> %.2f\n", value);
                    // lua_pushnumber(l, value);
                    return onNumberFn(value);
                } else {
                    int boolvalue = (c == 't' || c == 'T');
                    // lua_pushboolean(l, boolvalue);
                    //DEBUG printf("  bool -> %s\n", (boolvalue ? "true" : "false"));
                    return onBooleanFn( boolvalue );
                }
            }

            if ( tokens->type == JSMN_OBJECT ) {
                return push_json_object(json);
            }

            if ( tokens->type == JSMN_ARRAY ) {
                return push_json_array(json);
            }
            
            onStringFn((char*)"#Json Unknown type#");
        }

        void push_json_object(const char *json) {
            int size = tokens->size;
            
            //DEBUG printf("push objects size %d\n", size);
            // lua_newtable(l);
            onObjectFn(size);
            while (size-- > 0) {
                // Key (must be string)
                tokens++;
                const char *ptr = json + tokens->start;
                // json[ tokens->end ] = 0;
                std::string key = std::string(ptr, tokens->end - tokens->start);
                //DEBUG printf("key -> %s\n", key.c_str());
                // lua_pushstring(l, ptr);
                onStringFn( key );
                // value ...
                tokens++;
                push_json_value(json);
                afterObjectElm();
                // lua_settable(l, -3);
            }
        }



        public:
        _Parser() {};
        void parse(const char* json) {
            jsmn_init(&parser);
            jsmntok_t _tokens[100];
            tokens = _tokens; // temp assign tokens
            //DEBUG printf("Parser initialising ...\n");
            int r = jsmn_parse( &parser, json, strlen(json), tokens, 100 );
            //DEBUG printf("   result ... %d\n", r);

            if ( r == JSMN_ERROR_NOMEM ) return onErrorFn("Json contains too much nodes");
            if ( r == JSMN_ERROR_INVAL ) return onErrorFn("Json contains invalid characters");
            if ( r == JSMN_ERROR_PART ) return onErrorFn("incomplete or malformated Json");

            if (r < 1 || tokens[0].type != JSMN_OBJECT) {
                return onErrorFn("Body must be a json object");
            }

            push_json_object(json);
        }
        Parser* onNumber(std::function<void(double)> consumer) {
            this->onNumberFn = consumer;
            return this;
        }
        Parser* onString(std::function<void(std::string)> consumer) {
            this->onStringFn = consumer;
            return this;
        }
        Parser* onBoolean(std::function<void(bool)> consumer) {
            this->onBooleanFn = consumer;
            return this;
        }
        Parser* onArray(
            std::function<void(int)> consumer,
            std::function<void(int)> afterArrayElm
        ) {
            this->onArrayFn = consumer;
            this->afterArrayElm = afterArrayElm;
            return this;
        }
        Parser* onObject(
            std::function<void(int)> consumer,
            std::function<void()> afterObjectElm
        ) {
            this->onObjectFn = consumer;
            this->afterObjectElm = afterObjectElm;
            return this;
        }
        Parser* onError(std::function<void(std::string)> consumer) {
            this->onErrorFn = consumer;
            return this;
        }
    };


    Parser* createParser() {
        return new _Parser();
    };


};