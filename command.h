
#ifndef command_h
#define command_h


struct SimpleCommand
{
	int _numberOfAvailableArguments; // [ ] space allocated for arguments.
	int _numberOfArguments;          // # arguments currently stored.
	char **_arguments;               // argv-style list: command + args, ending with NULL.

	SimpleCommand();
	void insertArgument(char *argument);
};


struct Command
{
	int _numberOfAvailableSimpleCommands; // [ ] space allocated for pipeline commands.
	int _numberOfSimpleCommands;          // # commands in the pipeline (e.g. `a | b` = 2).
	SimpleCommand **_simpleCommands;      // Pipeline stages, in order: `a | b`.
	char *_outFile;                       // `>` output destination (stdout).
	char *_inputFile;                     // `<` input source (stdin).
	char *_errFile;                       // `2>` error destination (stderr).
	int  _out_error;                      // `>&` / `2>&1`: combine error and output redirection.
	int _background;                      // `&`: run without waiting in the foreground.
	int _append;                          // `>>`: append output instead of replacing file contents.

	void prompt();
	void print();
	void execute();
	void clear();

	Command();
	void insertSimpleCommand(SimpleCommand *simpleCommand); 

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
