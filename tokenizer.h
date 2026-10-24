#ifndef tokenizer_h
#define tokenizer_h 


#include <string>
#include <vector>

enum TokenType
{
    TOKEN_COMMAND,              // the program name 
    TOKEN_ARGUMENT,             // an argument
    TOKEN_REDIRECT,             // >
    TOKEN_APPEND,               // >>
    TOKEN_INPUT,                // <
    TOKEN_ERROR,                // 2>
    TOKEN_PIPE,                 // |
    TOKEN_BACKGROUND,           // &
    TOKEN_REDIRECT_AND_ERROR,   // >>&
    TOKEN_EOF,                  // end of input
};

struct Token
{
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string &input);

#endif
