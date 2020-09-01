
#include <pthread.h>
#include "src/util/log-message.h"

#include "src/util/namespace-start.h"

int g_verbose_level = 0;
const char *g_program_name = NULL;
static LogHandler g_log_handler = NULL; // function pointer.

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
// If the program name was set (g_program_name != ""), GetProgramName
// returns the program name (without the path).
// Otherwise it returns the empty string "".
const char *GetProgramName()
{
	pthread_mutex_lock(&log_mutex);
	const char *tmp_name = (g_program_name == NULL ? "" : g_program_name);
	pthread_mutex_unlock(&log_mutex);
	return tmp_name;
	//return g_program_name == NULL ? "" : g_program_name;
}

/***** HELPER FUNCTIONS *****/

// Given a filename like "/a/b/c/d/e/f.cc",  GetShortFileName
// returns "e/f.cc".  Does not currently work if backslash is
// the filename separator.
static const char *GetShortFileName(const char *filename) 
{
	const char *last_slash = strrchr(filename, '/');
	if (!last_slash)
   	{
		return filename;
	} 
	else 
	{
		while (last_slash > filename && last_slash[-1] != '/')
			last_slash--;
		return last_slash;
	}
}

/***** LOGGING *****/

MessageLogger::MessageLogger(LogMessageEnvelope::Severity severity,
		const char *func, const char *file, int line) 
{
	// Obviously, we assume the strings survive the destruction of this object.
	_envelope.severity = severity;
	_envelope.func = func;
	_envelope.file = GetShortFileName(file);  // Pointer inside 'file'.
	_envelope.line = line;
}


MessageLogger::~MessageLogger() NOEXCEPT(false) 
{
	// remove trailing '\n',
	std::string str = _ss.str();
	while (!str.empty() && str[str.length() - 1] == '\n')
		str.resize(str.length() - 1);

	// print the mesage (or send to logging handler),	
	MessageLogger::HandleMessage(_envelope, str.c_str());	  
}

void MessageLogger::HandleMessage(const LogMessageEnvelope &envelope,
		const char *message) 
{
	// Send to a logging handler if provided.
	if (g_log_handler != NULL)
	{
		g_log_handler(envelope, message);
	}
	else
	{
		// Otherwise, we use the default logging.
		// Build the log-message 'header',
		std::stringstream header;
		if (envelope.severity > LogMessageEnvelope::kInfo)
		{
			header << "VLOG_COM[" << envelope.severity << "] (";
		}
		else
		{
			switch(envelope.severity)
			{
				case LogMessageEnvelope::kInfo :
					header << "LOG_COM (";
					break;
				case LogMessageEnvelope::kWarning :
					header << "WARNING (";
					break;
				case LogMessageEnvelope::kError :
					header << "ERROR (";
					break;
				case LogMessageEnvelope::kAssertFailed :
					header << "ASSERTION_FAILED (";
					break;
				default:
					abort(); // coding error (unknown 'severity'),
			}
		}
		// fill the other info from the envelope,
		header << GetProgramName() << "[" VERSION "]" << ':'
			<< envelope.func << "():" << envelope.file << ':' << envelope.line
			<< ")";
		// Printing the message,
		if (envelope.severity >= LogMessageEnvelope::kWarning) 
		{
			// VLOG_COM, LOG_COM, WARNING:
			fprintf(stderr, "%s %s\n", header.str().c_str(), message);
		}
		else
		{
			// ERROR, ASSERT_FAILED (print with stack-trace):
			fprintf(stderr, "%s %s\n\n", header.str().c_str(), message);
		}
	}

	// Should we throw exception, or abort?
	switch (envelope.severity)
	{
		case LogMessageEnvelope::kAssertFailed:
			abort(); // ASSERT_FAILED,
			break;
		case LogMessageEnvelope::kError:
			if (!std::uncaught_exception())
			{
				// throw exception with empty message,
				throw std::runtime_error(""); // ERR,
			}
			else
			{
				// If we got here, this thread has already thrown exception,
				// and this exception has not yet arrived to its 'catch' clause...
				// Throwing a new exception would be unsafe!
				// (can happen during 'stack unwinding', if we have 'ERR << msg'
				// in a destructor of some local object).
				abort();
			}
			break;
	}
}

/***** ASSERTS *****/
void AssertFailure_(const char *func, const char *file,
		int line, const char *cond_str) 
{
	MessageLogger ml(LogMessageEnvelope::kAssertFailed, func, file, line);
	ml.stream() << ": '" << cond_str << "' ";
}

/***** THIRD-PARTY LOG-HANDLER *****/

LogHandler SetLogHandler(LogHandler new_handler) 
{
	LogHandler old_handler = g_log_handler;
	g_log_handler = new_handler;
	return old_handler;
}

/***** LOG FILE SET *****/
FILE *g_log_file_pointer = NULL;
std::string g_log_file;

void ProcessLogHandler(const LogMessageEnvelope &envelope,
		const char *message)
{
	// Otherwise, we use the default logging.
	// Build the log-message 'header',
	std::stringstream header;
	if (envelope.severity > LogMessageEnvelope::kInfo)
	{
		header << "VLOG_COM[" << envelope.severity << "] (";
	}
	else
	{
		switch(envelope.severity)
		{
			case LogMessageEnvelope::kInfo :
				header << "LOG_COM (";
				break;
			case LogMessageEnvelope::kWarning :
				header << "WARNING (";
				break;
			case LogMessageEnvelope::kError :
				header << "ERROR (";
				break;
			case LogMessageEnvelope::kAssertFailed :
				header << "ASSERTION_FAILED (";
				break;
			default:
				abort(); // coding error (unknown 'severity'),
		}
	}
	// fill the other info from the envelope,
	header << GetProgramName() << "[" VERSION "]" << ':'
		<< envelope.func << "():" << envelope.file << ':' << envelope.line
		<< ")";
	// Printing the message,
	if (envelope.severity >= LogMessageEnvelope::kWarning) 
	{
		// VLOG_COM, LOG_COM, WARNING:
		fprintf(g_log_file_pointer, "%s %s\n", header.str().c_str(), message);
	}
	else
	{
		// ERROR, ASSERT_FAILED (print with stack-trace):
		fprintf(g_log_file_pointer, "%s %s\n\n", header.str().c_str(), message);
	}
}

void CreateLogHandler(const char *log_file)
{
	if(g_log_file.empty())
	{
		if(log_file == NULL)
			return;
		else
			g_log_file = log_file;
	}
	g_log_file_pointer = fopen(g_log_file.c_str(),"a");
	if(g_log_file_pointer == NULL)
	{
		fprintf(stderr,"fopen %s failed.\n",g_log_file.c_str());
		abort();
	}
	g_log_handler = ProcessLogHandler;
}

void CloseLogHandler()
{
	fclose(g_log_file_pointer);
	g_log_file.clear();
	g_log_file_pointer = NULL;
	g_log_handler = NULL;
}
#include "src/util/namespace-end.h"
