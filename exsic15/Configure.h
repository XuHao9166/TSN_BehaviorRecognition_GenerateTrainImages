#ifndef CONFIGURE_H
#define CONFIGURE_H

#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4996)
#pragma warning(disable:4297)
#endif

#include <string>
#include <iostream>
#include <map>
#include "Exception.hpp"

class Configure : public std::map<std::string, std::string>
{
public:
	Configure();
	Configure(const std::string& prefix);
	virtual ~Configure();

	void Load(int argc, char** argv);
	void Load(std::istream& in);
	
	void Save(std::ostream& out);
	
	int GetInt(const std::string& property, int defaultValue);
	bool GetBool(const std::string& property, bool defaultValue);
	std::string GetString(const std::string& property,const char* defaultValue);
	
	int GetInt(const std::string& property) throw ();
	bool GetBool(const std::string& property) throw ();
	std::string GetString(const std::string& property) throw ();

	
protected:
	virtual void SubstituteVariables(std::string& value);

	/*! \brief 测试字符串是否全部是数字
	 *  @param str 待测试的字符串开始地址，C风格
	 *  @return  字符串是数字返回true，否则返回false
	 */
	bool isnumstr(const char* s);

	/*! \brief 去掉字符串右边指定的字符
	 *  @param	str				字符串地址
	 *  @param  trimlets         去除字符串的字符
	 *  @return				去除指定字符后的字符串
	 */
	std::string rtrim(const std::string& str,const std::string& trimlets);

	/*! \brief 去掉字符串右边指定的字符
	 *  @param	str				字符串地址
	 *  @param  trimlet         去除字符串的字符,默认为空格
	 *  @return				字符串开始地址
	 */
	char* rtrim(char *s,const char trimlet);
	
	/*! \brief 去掉字符串左边指定的字符
	 *  @param	str				字符串地址
	 *  @param  trimlets         去除字符串的字符
	 *  @return				去除指定后的字符串
	 */
	std::string ltrim(const std::string& str,const std::string& trimlets);

private:
	std::string m_prefix;
};

#endif