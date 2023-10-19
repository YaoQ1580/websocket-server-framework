#ifndef _KeyString_HPP_
#define _KeyString_HPP_

#include<cstring>

class KeyString
{
private:
	const char* _str = nullptr;
public:
	KeyString(const char* str)
	{
		set(str);
	}

	void set(const char* str)
	{
		_str = str;
	}

	const char* get()
	{
		return _str;
	}

	friend bool operator < (const KeyString& left, const KeyString& right);
};

bool operator < (const KeyString& left, const KeyString& right)
{
	return strcmp(left._str, right._str) < 0;
}

#endif
