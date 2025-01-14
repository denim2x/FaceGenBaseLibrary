//
// Copyright (c) 2019 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//

#include "stdafx.h"

#include "FgStdString.hpp"
#include "FgTypes.hpp"

using namespace std;

namespace Fg {

string
fgToFixed(double val,uint fractionalDigits)
{
    ostringstream   os;
    os << std::fixed << std::setprecision(fractionalDigits) << val;
    return os.str();
}

string
fgToPercent(double val,uint fractionalDigits)
{return fgToFixed(val*100.0,fractionalDigits) + "%"; }

std::string
fgToLower(const std::string & s)
{
    string  retval;
    retval.reserve(s.size());
	// Can't use std::transform since 'tolower' causes warning by converting int->char
    // std::transform(s.begin(),s.end(),retval.begin(),::tolower);
	for (char ch : s)
		retval.push_back(char(std::tolower(ch)));
    return retval;
}

std::string
fgToUpper(const std::string & s)
{
    string  retval;
    retval.reserve(s.size());
	// Can't use std::transform since 'tolower' causes warning by converting int->char
	// std::transform(s.begin(),s.end(),retval.begin(),::toupper);
	for (char ch : s)
		retval.push_back(char(std::toupper(ch)));
	return retval;
}

std::vector<string>
fgSplitAtSeparators(
    const std::string & str,
    char                sep)
{
    vector<string>  ret;
    string          tmp;
    string::const_iterator  it = str.begin();
    while (it != str.end())
    {
        if (*it == sep)
        {
            ret.push_back(tmp);
            tmp.clear();
            it++;
        }
        else
            tmp += *it++;
    }
    if (!tmp.empty())
        ret.push_back(tmp);
    return ret;
}

std::string
fgReplace(
    const std::string & str,
    char                orig,
    char                repl)
{
    std::string     ret = str;
    for (size_t ii=0; ii<ret.size(); ++ii)
        if (ret[ii] == orig)
            ret[ii] = repl;
    return ret;
}

string
fgPad(const string & str,size_t len,char ch)
{
    string  ret = str;
    if (len > str.size())
        ret.resize(len,ch);
    return ret;
}

string
cat(const vector<string> & strings,const string & separator)
{
    string      ret;
    for (size_t ii=0; ii<strings.size(); ++ii) {
        ret += strings[ii];
        if (ii < strings.size()-1)
            ret += separator;
    }
    return ret;
}

template<>
Opt<int>
fgFromStr(const string & str)
{
    Opt<int>              ret;
    if (str.empty())
        return ret;
    bool                    neg = false;
    string::const_iterator  it = str.begin();
    if (*it == '-') {
        neg = true;
        ++it;
    }
    else if (*it == '+')
        ++it;
    int64                   acc = 0;
    while (it != str.end()) {
        int                 digit = int(*it++) - int('0');
        if ((digit < 0) || (digit > 9))
            return ret;
        acc = 10 * acc + digit;
        if (acc > numeric_limits<int>::max())   // Negative mag can be one greater but who cares.
            return ret;
    }
    acc = neg ? -acc : acc;
    ret = int(acc);
    return ret;
}

template<>
Opt<uint>
fgFromStr<uint>(const string & str)
{
    Opt<uint>             ret;
    if (str.empty())
        return ret;
    string::const_iterator  it = str.begin();
    if (*it == '+')
        ++it;
    uint64                  acc = 0;
    while (it != str.end()) {
        int                 digit = int(*it++) - int('0');
        if ((digit < 0) || (digit > 9))
            return ret;
        acc = 10 * acc + uint64(digit);
        if (acc > numeric_limits<uint>::max())
            return ret;
    }
    ret = uint(acc);
    return ret;
}

}

// */
