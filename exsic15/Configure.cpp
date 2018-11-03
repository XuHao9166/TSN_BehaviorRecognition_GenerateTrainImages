//#include "stdafx.h"

#include "Configure.h"
#include <sstream>
#include <stdlib.h>

using namespace std;

Configure::Configure() :m_prefix("property")
{
}

Configure::Configure(const std::string& prefix) :m_prefix(prefix)
{
}

Configure::~Configure() 
{
}

std::string Configure::ltrim(const std::string& str,const std::string& trimlets)
{
	std::string::size_type length;
	length = str.find_first_not_of(trimlets);
	if (length != std::string::npos && length != 0)
	{
		return str.substr(length);
	}
	return str;
}

char* Configure::rtrim(char *s,const char trimlet)
{
	size_t i, l ;

	l = strlen(s);
	for(i=l-1;i>=0;i--)
	{
		if(s[i]!=trimlet) break;
		s[i] = 0 ;
	}
	return(s);
}

std::string Configure::rtrim(const std::string& str,const std::string& trimlets)
{
	std::string::size_type length;
	length = str.find_last_not_of(trimlets);
	if (length != std::string::npos && length != str.size() - 1)
	{
		return str.substr(0,length+1);
	}
	return str;
}

bool Configure::isnumstr(const char* s)
{

    if (*s == 0)
        return false;

	if (*s == '-' || *s == '+')
		++s;

    while (*s != 0)
	{
		if (!isdigit(*s))
	   	 return false;
		++s;
	}
    return true;
}

void Configure::Load(int argc, char** argv)
{
	clear();
	
	std::string position;
	std::string value;
	
	for (int i=0; i<argc; ++i)
	{
		std::ostringstream ost;
		ost <<"arg"<<i;
		position = ost.str();
		value = std::string(argv[i]);
		insert(value_type(position, value));
	}
}

void Configure::Load(std::istream& in) 
{
	clear();
	
	std::string fullLine, command;
	std::string leftSide, rightSide;
	char line[4096] = {0};
	std::string::size_type length;
	
	memset(line,0,sizeof(line));
	while (in.getline(line, sizeof(line))) 
	{
		fullLine = line;
		
		/* if the line contains a # then it is a comment
		if we find it anywhere other than the beginning, then we assume 
		there is a command on that line, and it we don't find it at all
		we assume there is a command on the line (we test for valid 
		command later) if neither is true, we continue with the next line
		*/
		length = fullLine.find('#');
		if (length == std::string::npos) 
		{
			command = fullLine;
		} 
		else if (length > 0) 
		{
			command = fullLine.substr(0, length);
		} 
		else 
		{
			continue;
		}
		
		// check the command and handle it
		length = command.find('=');
		if (length != std::string::npos) 
		{
			leftSide = command.substr(0, length);
			rightSide = command.substr(length + 1, command.size() - length);
			leftSide = ltrim(rtrim(leftSide," \t")," \t");
			rightSide = rtrim(rightSide,"\r\n");
			rightSide = ltrim(rtrim(rightSide," \t")," \t");
			SubstituteVariables(rightSide);
		} 
		else 
		{
			continue;
		}
		
		// strip off the PROGNAME prefix 
		length = leftSide.find('.');
		if (leftSide.substr(0, length) == m_prefix)
			leftSide = leftSide.substr(length + 1);
		
		// add to the map of Configure
		insert(value_type(leftSide, rightSide));
	}
}


void Configure::Save(std::ostream& out) 
{
	for (const_iterator i = begin(); i != end(); ++i) 
	{
		out << (*i).first << "=" << (*i).second << std::endl;
	}
}

int Configure::GetInt(const std::string& property, int defaultValue) 
{
	const_iterator key = find(property);
	return (key == end()) ? defaultValue : atoi((*key).second.c_str());
}


bool Configure::GetBool(const std::string& property, bool defaultValue) 
{
	const_iterator key = find(property);
	return (key == end()) ? defaultValue : ((*key).second == "true");
}

std::string Configure::GetString(const std::string& property, const char* defaultValue) 
{
	const_iterator key = find(property);
	return (key == end()) ? std::string(defaultValue) : (*key).second;
}

int Configure::GetInt(const std::string& property) throw ()
{
	const_iterator key = find(property);
	if (key==end())
	{
		throw ConfigureException(property+" is not configured");
	}
	else if (!isnumstr((*key).second.c_str()))
	{
		throw ConfigureException(property+" is not integer value");
	}
	return atoi((*key).second.c_str());
}

bool Configure::GetBool(const std::string& property) throw ()
{
	const_iterator key = find(property);
	if (key == end())
	{
		throw ConfigureException(property+" is not configured");
	}
	return ((*key).second == "true");
}

std::string Configure::GetString(const std::string& property) throw ()
{
	const_iterator key = find(property);
	if (key == end())
	{
		throw ConfigureException(property+" is not configured");
	}
	return (*key).second;
}

void Configure::SubstituteVariables(std::string& value) 
{
	std::string result;
	
	std::string::size_type left = 0;
	std::string::size_type right = value.find("${", left);
	
	if (right == std::string::npos) 
	{
		// bail out early for 99% of cases
		return;
	}
	
	while (true) 
	{
		result += value.substr(left, right - left);
		if (right == std::string::npos) 
		{
			break;
		}
		
		left = right + 2;
		right = value.find('}', left);
		if (right == std::string::npos) 
		{
			// no close tag, use string literally
			result += value.substr(left - 2);
			break;
		}
		else 
		{
			const std::string key = value.substr(left, right - left);
			if (key == "${") 
			{
				result += "${";
			}
			else
			{
				char* value = getenv(key.c_str());
				if (value) 
				{
					result += value;
				}
				else 
				{
					const_iterator it = find(key);
					
					if (it == end()) // not found assume empty;
					{
					}
					else
					{
						result += (*it).second;
					}
				}
			}
			left = right + 1;
		}
		
		right = value.find("${", left);
	}
	
	value = result;
}
