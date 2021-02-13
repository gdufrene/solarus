#include "solarus/net/Json.h"
#include "solarus/third_party/json/jsmin.h"
#include <string.h>
#include <stdlib.h>

namespace Solarus::Json {

    class _Parser: public Parser {
        private:
        jsmntok_t *tokens;
        jsmn_parser parser;

        std::function<void(double)> onNumberFn;
        std::function<void(char *)> onStringFn;
        std::function<void(bool)> onBooleanFn;
        std::function<void(int)> onArrayFn, onObjectFn;
        std::function<void(const char*)> onErrorFn;
        std::function<void(int)> afterArrayElm;
        std::function<void()> afterObjectElm;

/*
        void push_json_object(char *json);
        void push_json_value(char *json);
        void push_json_array(char *json);
*/

        void push_json_array(char *json) {
            int size = tokens->size;
            int idx = 1;
            onArrayFn(size);
            while (size-- > 0) {
                tokens++;
                push_json_value(json);
                afterArrayElm(idx++);
            }
        }

        void push_json_value(char *json) {
            char *ptr = json + tokens->start;

            if ( tokens->type == JSMN_STRING ) {
                json[ tokens->end ] = '\0';
                // printf("string -> %s\n", ptr);
                // lua_pushstring(l, ptr);
                return onStringFn(ptr);
            }
            
            if ( tokens->type == JSMN_PRIMITIVE ) {
                char c = json[ tokens->start ];
                if ( c >= '0' && c <= '9' ) {
                    double value;
                    json[ tokens->end ] = '\0';
                    // strncpy( buffer, ptr, tokens[idx].end - tokens[idx].start );
                    value = strtod(ptr, NULL);
                    // printf("number -> %.2f\n", value);
                    // lua_pushnumber(l, value);
                    return onNumberFn(value);
                } else {
                    int boolvalue = (c == 't' || c == 'T');
                    // lua_pushboolean(l, boolvalue);
                    // printf("bool -> %s\n", (boolvalue ? "true" : "false"));
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

        void push_json_object(char *json) {
            int size = tokens->size;
            char *ptr;

            // lua_newtable(l);
            onObjectFn(size);
            while (size-- > 0) {
                // Key (must be string)
                tokens++;
                ptr = json + tokens->start;
                json[ tokens->end ] = '\0';
                // printf("key -> %s\n", ptr);
                // lua_pushstring(l, ptr);
                onStringFn(ptr);
                // value ...
                tokens++;
                push_json_value(json);
                afterObjectElm();
                // lua_settable(l, -3);
            }
        }



        public:
        _Parser() {};
        void parse(char* json) {
            jsmn_init(&parser);
            jsmntok_t tokens[100];
            int r = jsmn_parse( &parser, json, strlen(json), tokens, 100 );

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
        Parser* onString(std::function<void(char *)> consumer) {
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
        Parser* onError(std::function<void(const char*)> consumer) {
            this->onErrorFn = consumer;
            return this;
        }
    };


    Parser* createParser() {
        return new _Parser();
    };


};