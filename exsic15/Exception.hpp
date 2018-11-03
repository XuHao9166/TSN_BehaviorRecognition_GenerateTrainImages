#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <stdexcept>

class ConfigureException : public std::logic_error
{
public:
	ConfigureException(const std::string& what) throw ()
		: std::logic_error(what) {};
};

#endif //~EXCEPTION_H