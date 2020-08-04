// util/config-parse-options.h
//
// author: hubo
// time  : 2016/9/7
//

#ifndef __UTIL_CONFIGPARSE_OPTIONS_H__
#define __UTIL_CONFIGPARSE_OPTIONS_H__


#include <map>
#include <string>
#include <vector>
#include "util/log-message.h"

// the class ConfigParseOption is for parsing command-line options

class ConfigParseOptions
{
public:
	explicit ConfigParseOptions(const char *usage):
		_print_args(true), _help(false), _usage(usage), 
		_argc(0), _argv(NULL), _prefix(""), _other_parser(NULL)
	{
		RegisterStandard("config", &_config, "Configuration file to read (this "
				"option may be repeated)");
		RegisterStandard("print-args", &_print_args,
				"Print the command line arguments (to stderr)");
		RegisterStandard("help", &_help, "Print out usage message");
		RegisterStandard("verbose", &g_verbose_level,
				"Verbose level (higher->more logging)");
		RegisterStandard("log-file", &g_log_file,
				"Log file , default print stderr.");
   	}

	~ConfigParseOptions() {}

	// Methods from the interface
	void Register(const std::string &name,
			bool *ptr, const std::string &doc);
	void Register(const std::string &name,
			int *ptr, const std::string &doc);
	void Register(const std::string &name,
			short *ptr, const std::string &doc);
	void Register(const std::string &name,
			unsigned int *ptr, const std::string &doc);
	void Register(const std::string &name,
			float *ptr, const std::string &doc);
	void Register(const std::string &name,
			double *ptr, const std::string &doc);
	void Register(const std::string &name,
			std::string *ptr, const std::string &doc);

	/// This one is used for registering standard parameters of all the programs
	template<typename T> void RegisterStandard(const std::string &name,
			T *ptr, const std::string &doc);

	/*
	 * Parses the command line options and fills the ParseOptions-registered
	 * variables. This must be called after all the variables were registered!!!
	 * */
	int Read(int argc, const char *const *argv);

	// Prints the usage documentation [provided in the constructor].
	void PrintUsage(bool print_command_line = false);
	// Prints the actual configuration of all the registered variables
	void PrintConfig(std::ostream &os);

	// Reads the options values from a config file.  Must be called after
	// registering all options.  This is usually used internally after the
	// standard --config option is used, but it may also be called from a
	// program.
	void ReadConfigFile(const std::string &filename);

	// Number of positional parameters (c.f. argc-1).
	int NumArgs() const;

	// Returns one of the positional parameters; 1-based indexing for argc/argv
	// compatibility. Will crash if param is not >=1 and <=NumArgs().
	std::string GetArg(int param) const;

	std::string GetOptArg(int param) const 
	{
		return (param <= NumArgs() ? GetArg(param) : "");
	}

	void DisableOption(const std::string &name);

private:

	template<typename T> 
	void RegisterCommon(const std::string &name, T *ptr,
			const std::string &doc, bool is_standard);

	// Template to register various variable types,
	// used for program-specific parameters
	template<typename T> void RegisterTmpl(const std::string &name,
		   	T *ptr, const std::string &doc);

	// Following functions do just the datatype-specific part of the job
	// Register boolean variable
	void RegisterSpecific(const std::string &name, const std::string &idx,
			bool *b, const std::string &doc, bool is_standard);
	// Register int32 variable
	void RegisterSpecific(const std::string &name, const std::string &idx,
			int *i, const std::string &doc, bool is_standard);
	// Register unsinged  int32 variable
	void RegisterSpecific(const std::string &name, const std::string &idx,
			unsigned int *u,const std::string &doc, bool is_standard);
	// Register float variable
	void RegisterSpecific(const std::string &name, const std::string &idx,
			float *f, const std::string &doc, bool is_standard);
	// Register double variable [useful as we change BaseFloat type].
	void RegisterSpecific(const std::string &name, const std::string &idx,
			double *f, const std::string &doc, bool is_standard);
	// Register string variable
	void RegisterSpecific(const std::string &name, const std::string &idx,
			std::string *s, const std::string &doc,bool is_standard);
	
	// Does the actual job for both kinds of parameters
	// "has_equal_sign" is used to allow --x for a boolean option x,
	// and --y=, for a string option y.
	bool SetOption(const std::string &key, const std::string &value,
			bool has_equal_sign);

	bool ToBool(std::string str);
	int ToInt(const std::string &str);
	unsigned int ToUint(const std::string &str);
	float ToFloat(const std::string &str);
	double ToDouble(const std::string &str);

	// maps for option variables
	std::map<std::string, bool*> _bool_map;
	std::map<std::string, int*> _int_map;
	std::map<std::string, unsigned int*> _uint_map;
	std::map<std::string, float*> _float_map;
	std::map<std::string, double*> _double_map;
	std::map<std::string, std::string*> _string_map;

	/*
	 * Structure for options' documentation
	 **/
	struct DocInfo 
	{
		DocInfo() {}
		DocInfo(const std::string &name, const std::string &usemsg)
			: _name(name), _use_msg(usemsg), _is_standard(false) {}
		DocInfo(const std::string &name, const std::string &usemsg, bool is_standard)
			: _name(name), _use_msg(usemsg),  _is_standard(is_standard) {}
		std::string _name;
		std::string _use_msg;
		bool _is_standard;
	};

	typedef std::map<std::string, DocInfo> DocMapType;
	DocMapType _doc_map;  //< map for the documentation

private:
	bool _print_args;  // variable for the implicit --print-args parameter
	bool _help;        // variable for the implicit --help parameter
	std::string _config;// variable for the implicit --config parameter
	std::vector<std::string> _positional_args;
	const char *_usage;
	int _argc;
	const char *const *_argv;

	// These members are not normally used. They are only used when the object
	// is constructed with a prefix
	std::string _prefix;
	ConfigParseOptions *_other_parser;
	//OptionsItf *_other_parser;
protected:
	// SplitLongArg parses an argument of the form --a=b, --a=, or --a,
	// and sets "has_equal_sign" to true if an equals-sign was parsed..
	// this is needed in order to correctly allow --x for a boolean option
	// x, and --y= for a string option y, and to disallow --x= and --y.
	void SplitLongArg(std::string in, std::string *key, std::string *value,
			bool *has_equal_sign);

	void NormalizeArgName(std::string *str);
};

#endif
