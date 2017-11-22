
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include "config-parse-options.h"
#include "text-util.h"
/*
ConfigParseOptions::ConfigParseOptions(const std::string &prefix,OptionsItf *other):
	 _print_args(false), _help(false), _usage(""), _argc(0), _argv(NULL)
{
	ConfigParseOptions *po = dynamic_cast<ConfigParseOptions*>(other);
	if (po != NULL && po->_other_parser != NULL)
	{
		// we get here if this constructor is used twice, recursively.
		_other_parser = po->_other_parser;
	}
	else
	{
		_other_parser = other;
	}
	if (po != NULL && po->prefix_ != "")
	{
		_prefix = po->_prefix + std::string(".") + prefix;
	}
	else
	{
		_prefix = prefix;
	}
}
*/
void ConfigParseOptions::Register(const std::string &name,
		bool *ptr, const std::string &doc) 
{
	RegisterTmpl(name, ptr, doc);
}

void ConfigParseOptions::Register(const std::string &name,
		int *ptr, const std::string &doc) 
{
	RegisterTmpl(name, ptr, doc);
}

void ConfigParseOptions::Register(const std::string &name,
		unsigned int *ptr, const std::string &doc) 
{
	RegisterTmpl(name, ptr, doc);
}

void ConfigParseOptions::Register(const std::string &name,
		float *ptr, const std::string &doc)
{
	RegisterTmpl(name, ptr, doc);
}

void ConfigParseOptions::Register(const std::string &name,
		double *ptr, const std::string &doc)
{
	RegisterTmpl(name, ptr, doc);
}

void ConfigParseOptions::Register(const std::string &name,
		std::string *ptr, const std::string &doc)
{
	RegisterTmpl(name, ptr, doc);
}

template<typename T> 
void ConfigParseOptions::RegisterTmpl(const std::string &name, T *ptr, 
		const std::string &doc)
{
	if(_other_parser == NULL)
	{
		this->RegisterCommon(name, ptr, doc, false);
	}
	else
	{
		assert(_prefix != "" && 
				"Cannot use empty prefix when registering with prefix.");
		std::string new_name = _prefix + '.' + name; // name becomes prefix.name
		_other_parser->Register(new_name, ptr, doc);
	}
}

void ConfigParseOptions::NormalizeArgName(std::string *str)
{
	std::string out;
	std::string::iterator it;

	for(it = str->begin(); it != str->end(); ++it)
	{
		if (*it == '_')
			out += '-'; // convert _ to -
		else
			out += std::tolower(*it);
	}
	*str = out;

	assert(str->length() > 0);
}

// used to register standard parameters (those that are present in all of the
// applications)
template<typename T>
void ConfigParseOptions::RegisterStandard(const std::string &name, T *ptr,
		const std::string &doc)
{
	this->RegisterCommon(name, ptr, doc, true);
}

// does the common part of the job of registering a parameter
template<typename T>
void ConfigParseOptions::RegisterCommon(const std::string &name, T *ptr,
		const std::string &doc, bool is_standard) 
{
	assert(ptr != NULL);
	std::string idx = name;
	NormalizeArgName(&idx);
	if (_doc_map.find(idx) != _doc_map.end())
		std::cerr << "Registering option twice, ignoring second time: " << name << std::endl;
	this->RegisterSpecific(name, idx, ptr, doc, is_standard);
}

void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, bool *b, 
		const std::string &doc, bool is_standard)
{
	_bool_map[idx] = b;
	_doc_map[idx] = DocInfo(name, doc + " (bool, default = "
			+ ((*b)? "true)" : "false)"), is_standard);
}

void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, int *i,
		const std::string &doc, bool is_standard)
{
	_int_map[idx] = i;
	std::ostringstream ss;
	ss << doc << " (int, default = " << *i << ")";
	_doc_map[idx] = DocInfo(name, ss.str(), is_standard);
}

void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, unsigned int *u,
		const std::string &doc, bool is_standard)
{
	_uint_map[idx] = u;
	std::ostringstream ss;
	ss << doc << " (uint, default = " << *u << ")";
	_doc_map[idx] = DocInfo(name, ss.str(), is_standard);

}

void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, float *f,
		const std::string &doc, bool is_standard)
{
	_float_map[idx] = f;
	std::ostringstream ss;
	ss << doc << " (float, default = " << *f << ")";
	_doc_map[idx] = DocInfo(name, ss.str(), is_standard);
}

void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, double *f,
		const std::string &doc, bool is_standard)
{
	_double_map[idx] = f;
	std::ostringstream ss;
	ss << doc << " (double, default = " << *f << ")";
	_doc_map[idx] = DocInfo(name, ss.str(), is_standard);
}


void ConfigParseOptions::RegisterSpecific(const std::string &name,
		const std::string &idx, std::string *s,
		const std::string &doc, bool is_standard)
{
	_string_map[idx] = s;
	_doc_map[idx] = DocInfo(name, doc + " (string, default = \"" + *s + "\")",
			is_standard);
}

void ConfigParseOptions::DisableOption(const std::string &name)
{
	if (_argv != NULL)
		std::cerr << "DisableOption must not be called after calling Read().";
	if (_doc_map.erase(name) == 0)
		std::cerr << "Option " << name
			<< " was not registered so cannot be disabled: ";
	_bool_map.erase(name);
	_int_map.erase(name);
	_uint_map.erase(name);
	_float_map.erase(name);
	_double_map.erase(name);
	_string_map.erase(name);
}

int ConfigParseOptions::NumArgs() const 
{
	return _positional_args.size();
}


std::string ConfigParseOptions::GetArg(int i) const
{	// use KALDI_ERR if code error
	if (i < 1 || i > static_cast<int>(_positional_args.size()))
		std::cerr << "ParseOptions::GetArg, invalid index " << i;
	return _positional_args[i];
}


int ConfigParseOptions::Read(int argc, const char *const argv[])
{
	_argc = argc;
	_argv = argv;

	std::string key, value;
	// first pass: look for config parameter, look for priority
	int i = 0;
	for(i = 1; i < argc; ++i)
	{
		if(std::strncmp(argv[i], "--", 2) == 0)
		{
			if(std::strcmp(argv[i], "--") == 0)
			{
				// a lone "--" marks the end of named options
				break;
			}
			bool has_equal_sign;
			SplitLongArg(argv[i], &key, &value, &has_equal_sign);
			NormalizeArgName(&key);
			if(key.compare("config") == 0)
			{
				ReadConfigFile(value);
			}
			if(key.compare("help") == 0)
			{
				PrintUsage();
				exit(0);
			}
		}
	}
//	bool double_dash_seen = false;
	// second pass: add the command line options
	for(i = 1; i < argc; i++)
	{
		if (std::strncmp(argv[i], "--", 2) == 0)
		{
			if (std::strcmp(argv[i], "--") == 0)
			{
				// A lone "--" marks the end of named options.
				// Skip that option and break the processing of named options
				i += 1;
//				double_dash_seen = true;
				break;
			}
			bool has_equal_sign;
			SplitLongArg(argv[i], &key, &value, &has_equal_sign);
			NormalizeArgName(&key);
			if (!SetOption(key, value, has_equal_sign))
			{
				PrintUsage(true);
				std::cerr << "Invalid option " << argv[i] << std::endl;
			}
		}
		else
		{
			break;
		}
	}
	// process remaining arguments as positional
	for(i = 0 ; i < argc ; ++i)
	{
		if((std::strncmp(argv[i], "--",2) == 0))
		{
			continue;
		}
		else
		{
			_positional_args.push_back(std::string(argv[i]));
		}
	}
	return i;
}


void ConfigParseOptions::PrintUsage(bool print_command_line)
{
	std::cerr << '\n' << _usage << '\n';
	DocMapType::iterator it;
	// first we print application-specific options
	bool app_specific_header_printed = false;
	for(it = _doc_map.begin(); it != _doc_map.end(); ++it)
	{
		if (it->second._is_standard == false) 
		{  // application-specific option
			if (app_specific_header_printed == false) 
			{  // header was not yet printed
				std::cerr << "Options:" << '\n';
				app_specific_header_printed = true;
			}
			std::cerr << "  --" << std::setw(25) << std::left << it->second._name
				 << " : " << it->second._use_msg << '\n';
		}
	}

	if(app_specific_header_printed == true)
	{
		std::cerr << '\n';
	}
	// then the standard options
	std::cerr << "Standard options:" << '\n';
	for (it = _doc_map.begin(); it != _doc_map.end(); ++it)
	{
		if (it->second._is_standard == true)
	   	{  // we have standard option
			std::cerr << "  --" << std::setw(25) << std::left << it->second._name
				<< " : " << it->second._use_msg << '\n';
		}
	}
	std::cerr << '\n';
	if (print_command_line)
	{
		std::ostringstream strm;
		strm << "Command line was: ";
		for (int j = 0; j < _argc; j++)
			strm << _argv[j] << " ";
		strm << '\n';
		std::cerr << strm.str() << std::flush;
	}
}


void ConfigParseOptions::ReadConfigFile(const std::string &filename)
{
	std::ifstream is(filename.c_str(), std::ifstream::in);
	if (!is.good())
	{
		std::cerr << "Cannot open config file: " << filename;
		exit(-1);
	}

	std::string line, key, value;
	int line_number = 0;
	while (std::getline(is, line))
	{
		line_number++;
		size_t pos;
		if ((pos = line.find_first_of('#')) != std::string::npos)
		{
			line.erase(pos);
		}
		// skip empty lines
		TrimString(line);
		if (line.length() == 0) 
			continue;

		if (line.substr(0, 2) != "--")
		{
			std::cerr << "Reading config file " << filename
				<< ": line " << line_number << " does not look like a line "
				<< "from a Kaldi command-line program's config file: should "
				<< "be of the form --x=y.  Note: config files intended to "
				<< "be sourced by shell scripts lack the '--'.";
		}

		// parse option
		bool has_equal_sign;
		SplitLongArg(line, &key, &value, &has_equal_sign);
		NormalizeArgName(&key);
		if (!SetOption(key, value, has_equal_sign))
		{
			PrintUsage(true);
			std::cerr << "Invalid option " << line << " in config file " << filename << std::endl;
		}
	}
}

void ConfigParseOptions::SplitLongArg(std::string in,
		std::string *key,
		std::string *value,
		bool *has_equal_sign)
{
	size_t pos = in.find_first_of('=', 0);
	if (pos == std::string::npos)
	{// we allow --option for bools
		// defaults to empty.  We handle this differently in different cases.
		*key = in.substr(2, in.size()-2);  // 2 because starts with --.
		*value = "";
		*has_equal_sign = false;
	}
	else if (pos == 2) 
	{  // we also don't allow empty keys: --=value
		PrintUsage(true);
		std::cerr << "Invalid option (no key): " << in << '\n';
	}
	else
	{ // normal case: --option=value
		*key = in.substr(2, pos-2);  // 2 because starts with --.
		*value = in.substr(pos + 1);
		*has_equal_sign = true;
	}
}

bool ConfigParseOptions::SetOption(const std::string &key,
		const std::string &value,
		bool has_equal_sign)
{
	if (_bool_map.end() != _bool_map.find(key))
	{
		if (has_equal_sign && value == "")
			std::cerr << "Invalid option --" << key << "=";
		*(_bool_map[key]) = ToBool(value);
	}
	else if(_int_map.end() != _int_map.find(key))
	{
		*(_int_map[key]) = ToInt(value);
	}
	else if (_uint_map.end() != _uint_map.find(key))
	{
		*(_uint_map[key]) = ToUint(value);
	}
	else if (_float_map.end() != _float_map.find(key))
	{
		*(_float_map[key]) = ToFloat(value);
	}
	else if (_double_map.end() != _double_map.find(key))
	{
			*(_double_map[key]) = ToDouble(value);
	}
	else if (_string_map.end() != _string_map.find(key))
	{
		if (!has_equal_sign)
			std::cerr << "Invalid option --" << key;
		*(_string_map[key]) = value;
	}
	else
	{
		return false;
	}
	return true;
}

bool ConfigParseOptions::ToBool(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	// allow "" as a valid option for "true", so that --x is the same as --x=true
	if ((str.compare("true") == 0) || (str.compare("t") == 0)
			|| (str.compare("1") == 0) || (str.compare("") == 0))
	{
		return true;
	}
	if((str.compare("false") == 0) || (str.compare("f") == 0)
			|| (str.compare("0") == 0))
	{
		return false;
	}
	// if it is neither true nor false:
	PrintUsage(true);
	std::cerr << "Invalid format for boolean argument [expected true or false]: "
		<< str ;
	return false;
}

int ConfigParseOptions::ToInt(const std::string &str)
{
	int ret = 0;
	if (1 != sscanf(str.c_str(), "%d", &ret))
	{
		std::cerr << "Invalid integer option \"" << str << "\"" << std::endl;
	}
	return ret;
}

unsigned ConfigParseOptions::ToUint(const std::string &str)
{
	unsigned ret=0;
	if(1 != sscanf(str.c_str(), "%u", &ret))
	{
		std::cerr << "Invalid integer option \"" << str << "\"" << std::endl;
	}
	return ret;
}

float ConfigParseOptions::ToFloat(const std::string &str) 
{
	float ret;
	if(1 != sscanf(str.c_str(), "%f", &ret))
	{
		std::cerr << "Invalid floating-point option \"" << str << "\"" << std::endl;
	}
	return ret;
}

double ConfigParseOptions::ToDouble(const std::string &str)
{
	double ret;
	if(1 != sscanf(str.c_str(), "%lf", &ret))
	{
		std::cerr << "Invalid floating-point option \"" << str << "\"" << std::endl;
	}
	return ret;
}

template void ConfigParseOptions::RegisterStandard(const std::string &name,
		bool *ptr,
		const std::string &doc);
template void ConfigParseOptions::RegisterStandard(const std::string &name,
		int *ptr,
		const std::string &doc);
template void ConfigParseOptions::RegisterStandard(const std::string &name,
		unsigned int *ptr,
		const std::string &doc);
template void ConfigParseOptions::RegisterStandard(const std::string &name,
		float *ptr,
		const std::string &doc);
template void ConfigParseOptions::RegisterStandard(const std::string &name,
		double *ptr,
		const std::string &doc);
template void ConfigParseOptions::RegisterStandard(const std::string &name,
		std::string *ptr,
		const std::string &doc);

