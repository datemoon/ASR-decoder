#ifndef __LOG_MESSAGE_H__
#define __LOG_MESSAGE_H__

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/util/namespace-start.h"

/* Important that this file does not depend on any other headers. */

// By adding 'NOEXCEPT(bool)' immediately after function declaration,
// we can tell the compiler that the function must-not produce
// exceptions (true), or may produce exceptions (false):
#if _MSC_VER >= 1900 || (!defined(_MSC_VER) && __cplusplus >= 201103L)
#define NOEXCEPT(Predicate) noexcept((Predicate))
#elif defined(__GXX_EXPERIMENTAL_CXX0X__) && \
	(__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#define NOEXCEPT(Predicate) noexcept((Predicate))
#else
#define NOEXCEPT(Predicate)
#endif

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

//#define VERSION "2018-10-25"
/***** VERBOSITY LEVEL *****/

/// This is set by util/parse-options.{h, cc} if you set --verbose=? option.
extern int g_verbose_level;


/// This is set by util/parse-options.{h, cc} (from argv[0]) and used (if set)
/// in error reporting code to display the name of the program (this is because
/// in our scripts, we often mix together the stderr of many programs).  it is
/// the base-name of the program (no directory), followed by ':' We don't use
/// std::string, due to the static initialization order fiasco.
extern const char *g_program_name;

/// This is save log file.
extern std::string g_log_file;
extern FILE *g_log_file_pointer;

inline int GetVerboseLevel() { return g_verbose_level; }

/// This should be rarely used; command-line programs set the verbose level
/// automatically from ParseOptions.
inline void SetVerboseLevel(int i) { g_verbose_level = i; }

/***** LOGGING *****/

/// Log message severity and source location info.
struct LogMessageEnvelope 
{
	enum Severity
	{
		kAssertFailed = -3,
		kError = -2,
		kWarning = -1,
		kInfo = 0,
	};
	// An 'enum Severity' value, or a positive number indicating verbosity level.
	int severity;
	const char *func;
	const char *file;
	int line;
};

// Class MessageLogger is invoked from the LOG_ASSERT, LOG_ERR, LOG_WARN and
// LOG_COM macros. It formats the message, then either prints it to stderr or
// passes to the log custom handler if provided, then, in case of the error,
// throws an std::runtime_exception, in case of failed LOG_ASSERT calls abort().
// 
// Note: we avoid using std::cerr for thread safety issues.
// fprintf(stderr,...) is guaranteed thread-safe, and outputs
// its formatted string atomically.
class MessageLogger 
{
public:
	/// Constructor stores the info,
	MessageLogger(LogMessageEnvelope::Severity severity,
			const char *func,
			const char *file,
			int line);

	/// Destructor, calls 'HandleMessage' which prints the message,
	/// (since C++11 a 'throwing' destructor must be declared 'noexcept(false)')
	~MessageLogger() NOEXCEPT(false);

	/// The hook for the 'insertion operator', e.g.
	/// 'LOG_COM << "Message,"',
	inline std::ostream &stream() { return _ss; }

private:
	/// The logging function,
	static void HandleMessage(const LogMessageEnvelope &env, const char *msg);

private:
	LogMessageEnvelope _envelope;
	std::ostringstream _ss;
};

/***** ASSERTS *****/

void AssertFailure_(const char *func, const char *file,
		int line, const char *cond_str);

#ifdef NAMESPACE //

// The definition of the logging macros,
#define LOG_ERR \
	::datemoon::MessageLogger(::datemoon::LogMessageEnvelope::kError, \
			__func__, __FILE__, __LINE__).stream()
#define LOG_WARN \
	::datemoon::MessageLogger(::datemoon::LogMessageEnvelope::kWarning, \
			__func__, __FILE__, __LINE__).stream()
#define LOG_COM \
	::datemoon::MessageLogger(::datemoon::LogMessageEnvelope::kInfo, \
			__func__, __FILE__, __LINE__).stream()
#define VLOG_COM(v) if ((v) <= ::datemoon::g_verbose_level)     \
	::datemoon::MessageLogger((::datemoon::LogMessageEnvelope::Severity)(v), \
					 __func__, __FILE__, __LINE__).stream()


// The original (simple) version of the code was this
//
// #define LOG_ASSERT(cond) if (!(cond))
//              ::AssertFailure_(__func__, __FILE__, __LINE__, #cond);
//
// That worked well, but we were concerned that it
// could potentially cause a performance issue due to failed branch
// prediction (best practice is to have the if branch be the commonly
// taken one).
// Therefore, we decided to move the call into the else{} branch.
// A single block {} around if /else  does not work, because it causes
// syntax error (unmatched else block) in the following code:
//
// if (condition)
//   LOG_ASSERT(condition2);
// else
//   SomethingElse();
//
// do {} while(0)  -- note there is no semicolon at the end! --- works nicely
// and compilers will be able to optimize the loop away (as the condition
// is always false).


#ifndef NDEBUG
#define LOG_ASSERT(cond) do { if (cond) (void)0; else \
	::datemoon::AssertFailure_(__func__, __FILE__, __LINE__, #cond); } while(0)
#else
#define LOG_ASSERT(cond) (void)0
#endif

#else // ifdef NAMESPACE

// The definition of the logging macros,
#define LOG_ERR \
	::MessageLogger(::LogMessageEnvelope::kError, \
			__func__, __FILE__, __LINE__).stream()
#define LOG_WARN \
	::MessageLogger(::LogMessageEnvelope::kWarning, \
			__func__, __FILE__, __LINE__).stream()
#define LOG_COM \
	::MessageLogger(::LogMessageEnvelope::kInfo, \
			__func__, __FILE__, __LINE__).stream()
#define VLOG_COM(v) if ((v) <= ::g_verbose_level)     \
	::MessageLogger((::LogMessageEnvelope::Severity)(v), \
					 __func__, __FILE__, __LINE__).stream()

#ifndef NDEBUG
#define LOG_ASSERT(cond) do { if (cond) (void)0; else \
	::AssertFailure_(__func__, __FILE__, __LINE__, #cond); } while(0)
#else
#define LOG_ASSERT(cond) (void)0
#endif

#endif // ifdef NAMESPACE
/***** THIRD-PARTY LOG-HANDLER *****/

/// Type of third-party logging function,
typedef void (*LogHandler)(const LogMessageEnvelope &envelope,
		const char *message);

/// Set logging handler. If called with a non-NULL function pointer, the
/// function pointed by it is called to send messages to a caller-provided
/// log. If called with NULL pointer, restores default error logging to
/// stderr.  SetLogHandler is obviously not thread safe.
LogHandler SetLogHandler(LogHandler);

void CreateLogHandler(const char *log_file);

void CloseLogHandler();

#include "src/util/namespace-end.h"
#endif // __LOG_MESSAGE_H__
