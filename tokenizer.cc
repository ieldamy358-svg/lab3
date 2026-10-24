#include "tokenizer.h"
#include <sstream>
#include <iostream>

std::vector<Token> tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    // Read whitespace-separated words; operators must be separate words (for example, `ls > out`).
    std::istringstream stream(input);
    std::string word;

    while (stream >> word)
    {
        Token token;

        // Recognize shell operators by exact match.
        if (word == "|")
            token.type = TOKEN_PIPE;
        else if (word == ">")
            token.type = TOKEN_REDIRECT;
        else if (word == ">>")
            token.type = TOKEN_APPEND;
        else if (word == "<")
            token.type = TOKEN_INPUT;
        else if (word == "&")
            token.type = TOKEN_BACKGROUND;
        else if (word == "2>")
            token.type = TOKEN_ERROR;
        else if (word == ">>&")
            token.type = TOKEN_REDIRECT_AND_ERROR;
        else
            {
                // Non-operator words are classified using the previous token and
                // simple hints such as option prefixes and path characters.
                if (!tokens.empty())
                {
                    Token &prev = tokens.back();
                    if (prev.type == TOKEN_REDIRECT ||
                        prev.type == TOKEN_APPEND ||
                        prev.type == TOKEN_INPUT ||
                        prev.type == TOKEN_ERROR || 
                        prev.type == TOKEN_REDIRECT_AND_ERROR ||
                        prev.type == TOKEN_COMMAND || 
                        prev.type == TOKEN_ARGUMENT || 
                        word[0] == '-' ||
                        word.find('/') != std::string::npos ||
                        word.find('.') != std::string::npos

                    )
                    {
                        token.type = TOKEN_ARGUMENT;
                    }
                    else
                    {
                        token.type = TOKEN_COMMAND;
                    }
                }
                else
                {
                    token.type = TOKEN_COMMAND;
                }
            }

        // Keep the original word as the token's value, then add it in input order.
        token.value = word;
        tokens.push_back(token);
    }

    // Append an explicit end marker so callers can detect the end of the token list.
    tokens.push_back({TOKEN_EOF, ""});
    return tokens;
}
