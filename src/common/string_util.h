
#pragma once

#include <string>
#include <set>
#include <list>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <unordered_set>
#include <cmath>

namespace bdf { 

namespace common{

int CodeConverter(
  char *szInBuf, int nInBuflen, char *szOutBuf, int nOutBuflen,
  const char *szFromCharset = "gbk", const char *szToCharset = "utf-8");

void ToLow(std::string& src);
void ToUpper(std::string& src);

std::string UrlEncode(const std::string& src, bool is_unicode = false);
std::string UrlDecode(const std::string& src, bool is_unicode = false);
std::string UrlEncode_ext(const std::string& str, bool is_unicode = false);

void SafeBase64ToBase64(std::string& url64);
std::string Base64Decode(const char *str, size_t len = 0);
std::string Base64Encode(const void *src, size_t len, int wraps = 0);

void BinaryTo16Hex(const std::string& src, std::string& dest);

template<typename T>
T GetFromPrecision(T number, int after_point_nums){
  return ((int)(number*pow(10, after_point_nums) + 0.5)) / (pow(10, after_point_nums)*1.0);
}

template<typename T>
T exchange(std::string& src){
  return atoll(src.c_str());
}

template<>
inline double exchange(std::string& src){
  return atof(src.c_str());
}

template<>
inline float exchange(std::string& src){
  return atof(src.c_str());
}

template<>
inline std::string exchange(std::string& src){
  return src;
}

template<typename T>
void Split(std::vector<T>& ret, const std::string &str, char delim, bool ignoreEmpty = true){
  if (str.empty()){
    return;
  }
  ret.clear();

  size_t n = str.size();
  size_t s = 0;

  while (s <= n){
    size_t i = str.find_first_of(delim, s);
    size_t len = 0;
    //T tmp;

    if (i == std::string::npos){
      len = n - s;
    }
    else{
      len = i - s;
    }

    if (false == ignoreEmpty || 0 != len){
      std::string tmp = str.substr(s, len);
      ret.emplace_back(exchange<T>(tmp));
    }

    s += len + 1;
  }
}

template<typename T>
void Split(std::set<T>& ret, const std::string &str, char delim, bool ignoreEmpty = true){
  if (str.empty()){
    return;
  }
  ret.clear();

  size_t n = str.size();
  size_t s = 0;

  while (s <= n){
    size_t i = str.find_first_of(delim, s);
    size_t len = 0;
    //T tmp;

    if (i == std::string::npos){
      len = n - s;
    }
    else{
      len = i - s;
    }

    if (false == ignoreEmpty || 0 != len){
      std::string tmp = str.substr(s, len);
      ret.insert(std::move(exchange<T>(tmp)));
    }

    s += len + 1;
  }
}

template<typename T>
void Split(std::unordered_set<T>& ret, const std::string &str, char delim, bool ignoreEmpty = true){
  if (str.empty()){
    return;
  }
  ret.clear();

  size_t n = str.size();
  size_t s = 0;

  while (s <= n){
    size_t i = str.find_first_of(delim, s);
    size_t len = 0;
    //T tmp;

    if (i == std::string::npos){
      len = n - s;
    }
    else{
      len = i - s;
    }

    if (false == ignoreEmpty || 0 != len){
      std::string tmp = str.substr(s, len);
      ret.insert(std::move(exchange<T>(tmp)));
    }

    s += len + 1;
  }
}

#define UrlSafe64GetEncodedSize(size) ((4 * (size) + 2) / 3)
int UrlSafeBase64Encode(const uint8_t *src, int src_len, char *dst);
int UrlSafeBase64Decode(const char *src, int src_len, uint8_t *dst);

std::string& Trim(std::string &s);

#ifdef WIN32
#	define __strtol		strtol
#	define __strtoul	strtoul
#	define __strtoll   _strtoi64
#	define __strtoull	_strtoui64
#	define __strtof		strtod
#	define __strtod		strtod
#	define __strtold	strtod
#else
#	define __strtol		strtol
#	define __strtoul	strtoul
#	define __strtoll	strtoll
#	define __strtoull	strtoull
#	define __strtof		strtof
#	define __strtod		strtod
#	define __strtold	strtold
#endif

//////////////////////////////////////////////////////////////////////
// _StringToValueImpl

template <typename T>
struct _StringToValueImpl{};

#define _BUILD_STRING_TO_VALUE_IMPL_(type)          \
  template<>                        \
  struct _StringToValueImpl<type>             \
  {                             \
    static type toValue(\
    const char * str, \
    type default_value)               \
  {                           \
    return (type)(_StringToValueImpl<long>      \
    ::toValue(str, default_value));   \
  }                           \
  };                            \
    \
    template<>                        \
  struct _StringToValueImpl<unsigned type>        \
  {                             \
    static unsigned type toValue(\
    const char * str, \
    unsigned type default_value)          \
  {                           \
    return (unsigned type)(\
    _StringToValueImpl<unsigned long>       \
    ::toValue(str, default_value));       \
  }                           \
  };                            \

template<>
struct _StringToValueImpl<long>{
  static long toValue(const char * str, long default_value){
    // in order to use the smart judgement with strtoul and 
    // avoid the unexpected case, wo need to check the origin
    // str.
    // For strtoul:
    // the third parameter determined what's the format to parse
    // the str value, if it's 0, the systeam will choid the better
    // way to parse, this will cause a problem. explained under the
    // floowing step.
    // 1. if the two prefix character is '0x', will think it as sixteen format
    // 2. if the is '0*', will think it as octal format(must be wrong in here)
    // E.g : if str is "00123", the function will parse it as octal, that may
    // not what we want.
    // if (str[0] == '\0') return default_value;
    // if (str[0] == '0' && str[1] != 'x')
    // {
        // use str atoi to avoid the unexpected situation
    // }

    char * end = NULL;
    long n = __strtol(str, &end, 10);
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<unsigned long>{
  static unsigned long toValue(const char * str, unsigned long default_value){
    char * end = NULL;
    long n = ::__strtoul(str, &end, 10);
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<long long>{
  static long long toValue(const char * str, long long default_value){
    char * end = NULL;
    long long n = __strtoll(str, &end, 10);
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<unsigned long long>{
  static unsigned long long toValue(const char * str, unsigned long long default_value){
    char * end = NULL;
    long long n = __strtoull(str, &end, 10);
    // ???? end will be never be null, default value will be no longer used
    // any more. we should judge whether the *end is '\0' or not to pick the
    // default value.
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<float>{
  static float toValue(const char * str, float default_value){
    char * end = NULL;
    float n = __strtof(str, &end);
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<double>{
  static double toValue(const char * str, double default_value){
    char * end = NULL;
    double n = __strtod(str, &end);
    return end == NULL ? default_value : n;
  }
};

template<>
struct _StringToValueImpl<long double>{
  static long double toValue(const char * str, long double default_value){
    char * end = NULL;
    long double n = __strtold(str, &end);
    return end == NULL ? default_value : n;
  }
};

_BUILD_STRING_TO_VALUE_IMPL_(char);
_BUILD_STRING_TO_VALUE_IMPL_(short);
_BUILD_STRING_TO_VALUE_IMPL_(int);

#undef __strtol
#undef __strtoul
#undef __strtoll
#undef __strtoull
#undef __strtof
#undef __strtod
#undef __strtodl

template<typename T>
static T ToValue(const char * str, T default_value = 0)
{
  return _StringToValueImpl<T>::toValue(str, default_value);
}

template<typename T>
static T ToValue(const std::string& str, T default_value = 0)
{
  return _StringToValueImpl<T>::toValue(str.c_str(), default_value);
}

template<typename T>
static std::string ToString(T value)
{
  return std::to_string(value);
}

}
}
