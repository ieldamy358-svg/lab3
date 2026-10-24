
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"


void sigchld_handler(int sig) {
    (void)sig;
    int status;
    
    while (waitpid(-1, &status, WNOHANG) > 0) {
        FILE *logfile = fopen("shell_log.txt", "a");
        if (logfile) {
            time_t now = time(NULL);
            fprintf(logfile, "[%s] A child process was terminated.\n", ctime(&now));
            fclose(logfile);
        }
    }
}

void parse(std::vector<Token> &tokens)
{
    // ---- 1. Reset the global command state ----
    Command::_currentCommand.clear();
    Command::_currentCommand._out_error  = 0;
    Command::_currentCommand._append     = 0;
    Command::_currentCommand._background = 0;

    SimpleCommand *cur = nullptr;

    // ---- 2. Walk the token list ----
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        Token &t = tokens[i];

        switch (t.type)
        {
        // --------------------------------------------------------------
        // Ordinary word: program name or argument
        // Both go into the current SimpleCommand's argv.
        // --------------------------------------------------------------
        case TOKEN_COMMAND:
        case TOKEN_ARGUMENT:
        {
            if (!cur) cur = new SimpleCommand();
            cur->insertArgument(strdup(t.value.c_str()));
            break;
        }

        // --------------------------------------------------------------
        // Pipe: finish the current SimpleCommand, start a new one.
        // --------------------------------------------------------------
        case TOKEN_PIPE:
        {
            if (!cur) {
                fprintf(stderr, "myshell: syntax error near unexpected token `|'\n");
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand.insertSimpleCommand(cur);
            cur = nullptr;
            break;
        }

        // --------------------------------------------------------------
        // Output redirect: >  (consume next token as filename)
        // --------------------------------------------------------------
        case TOKEN_REDIRECT:
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type == TOKEN_EOF) {
                fprintf(stderr, "myshell: syntax error: `>' without filename\n");
                if (cur) { /* free cur */ }
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand._outFile = strdup(tokens[++i].value.c_str());
            Command::_currentCommand._append  = 0;
            break;
        }

        // --------------------------------------------------------------
        // Output redirect, append: >>
        // --------------------------------------------------------------
        case TOKEN_APPEND:
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type == TOKEN_EOF) {
                fprintf(stderr, "myshell: syntax error: `>>' without filename\n");
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand._outFile = strdup(tokens[++i].value.c_str());
            Command::_currentCommand._append  = 1;
            break;
        }

        // --------------------------------------------------------------
        // Input redirect: <
        // --------------------------------------------------------------
        case TOKEN_INPUT:
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type == TOKEN_EOF) {
                fprintf(stderr, "myshell: syntax error: `<' without filename\n");
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand._inputFile = strdup(tokens[++i].value.c_str());
            break;
        }

        // --------------------------------------------------------------
        // Stderr redirect: 2>
        // --------------------------------------------------------------
        case TOKEN_ERROR:
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type == TOKEN_EOF) {
                fprintf(stderr, "myshell: syntax error: `2>' without filename\n");
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand._errFile = strdup(tokens[++i].value.c_str());
            break;
        }

        // --------------------------------------------------------------
        // Merge stderr into stdout, with append: >>&
        // Both _errFile and _append set, plus _out_error marker.
        // --------------------------------------------------------------
        case TOKEN_REDIRECT_AND_ERROR:
        {
            if (i + 1 >= tokens.size() || tokens[i + 1].type == TOKEN_EOF) {
                fprintf(stderr, "myshell: syntax error: `>>&' without filename\n");
                Command::_currentCommand.clear();
                return;
            }
            Command::_currentCommand._errFile   = strdup(tokens[++i].value.c_str());
            Command::_currentCommand._out_error = 1;
            Command::_currentCommand._append    = 1;
            break;
        }

        // --------------------------------------------------------------
        // Background: &
        // --------------------------------------------------------------
        case TOKEN_BACKGROUND:
        {
            Command::_currentCommand._background = 1;
            break;
        }

        // --------------------------------------------------------------
        // End of input: finalize and execute.
        // --------------------------------------------------------------
        case TOKEN_EOF:
        {
            if (cur) {
                Command::_currentCommand.insertSimpleCommand(cur);
                cur = nullptr;
            }
            Command::_currentCommand.execute();   // execute handles clear+prompt
            return;
        }
        }
    }

    // If we reach here (no EOF token — shouldn't happen), still execute.
    if (cur) Command::_currentCommand.insertSimpleCommand(cur);
    Command::_currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        delete _simpleCommands[i];
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _out_error = 0;
    _background = 0;
    _append = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    // Nothing to execute for a blank line.
    if (_numberOfSimpleCommands == 0) {
        clear();
        return;
    }

    // Built-ins must run in this process so they can affect the shell itself.
    SimpleCommand *firstCmd = _simpleCommands[0];
    if (firstCmd->_numberOfArguments > 0) {
        if (strcmp(firstCmd->_arguments[0], "exit") == 0) {
            printf("Good bye!!\n");
            exit(0);
        }
        if (strcmp(firstCmd->_arguments[0], "cd") == 0) {
            const char *dir = (firstCmd->_numberOfArguments > 1) ? firstCmd->_arguments[1] : getenv("HOME");
            if (dir && chdir(dir) < 0) {
                perror("cd");
            }
            clear();
            return;
        }
    }

    // Save the shell's stdin/stdout/stderr so they can be restored after setup.
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    // The first command reads from the input file, or from the shell's stdin.
    int infd = defaultin;
    if (_inputFile) {
        infd = open(_inputFile, O_RDONLY);
        if (infd < 0) {
            perror("myshell: input file open error");
            clear();
            return;
        }
    }

    pid_t last_pid = -1;
    int fd[2];

    // Set up and start each command in the pipeline, from left to right.
    for (int i = 0; i < _numberOfSimpleCommands; i++) {
        // Connect this command's stdin to the previous input or pipe.
        dup2(infd, 0);
        if (infd != defaultin) close(infd);

        if (i == _numberOfSimpleCommands - 1) {
            // The final command writes to the output file, or the shell's stdout.
            int outfd = defaultout;
            if (_outFile) {
                int flags = O_WRONLY | O_CREAT;
                flags |= _append ? O_APPEND : O_TRUNC;
                outfd = open(_outFile, flags, 0666);
                if (outfd < 0) {
                    perror("myshell: output file open error");
                    dup2(defaultin, 0);
                    dup2(defaultout, 1);
                    dup2(defaulterr, 2);
                    clear();
                    return;
                }
            }
            dup2(outfd, 1);
            if (outfd != defaultout) close(outfd);

            // Send stderr to its own file, or merge it with stdout when requested.
            if (_errFile) {
                int errfd = defaultout;
                int errflags = O_WRONLY | O_CREAT;
                errflags |= _append ? O_APPEND : O_TRUNC;
                errfd = open(_errFile, errflags, 0666);
                if (errfd >= 0) {
                    dup2(errfd, 2);
                    if (errfd != defaultout) close(errfd);
                }
            } else if (_out_error) {
                dup2(1, 2);
            }
        } else {
            // Intermediate commands send stdout through a pipe to the next command.
            if (pipe(fd) < 0) {
                perror("pipe");
                clear();
                return;
            }
            dup2(fd[1], 1);
            close(fd[1]);
            infd = fd[0];
        }

        // The child replaces itself with the program; the parent continues the pipeline.
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            clear();
            return;
        }

        if (pid == 0) {
            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror(_simpleCommands[i]->_arguments[0]);
            _exit(1);
        }

        last_pid = pid;
    }

    // Restore the shell's original standard streams after launching the commands.
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    // Foreground commands finish before the shell accepts another line.
    if (!_background) {
        int status;
        waitpid(last_pid, &status, 0);
    }

    clear();
}
void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);
        if (!std::getline(std::cin, input))
        {
            printf("\n");
            return;
        }
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    Command::_currentCommand.prompt();
    return 0;
}
