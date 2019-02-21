
#include "string_util.h"
#include <string.h>
#include <limits.h>
#include <algorithm>
#include <iconv.h>

namespace bdf{

namespace common {

int CodeConverter(
  char *szInBuf, int nInBuflen, char *szOutBuf, int nOutBuflen,
  const char *szFromCharset, const char *szToCharset){
  if (szInBuf == NULL || nInBuflen <= 0 || szOutBuf == NULL || nOutBuflen <= 0)
    return 0;

  memset(szOutBuf, 0, nOutBuflen);
#ifdef WIN32  
  int nRelSize = 0;
  if (strcmp(szToCharset, "utf-8") == 0){
    nRelSize = MultiByteToWideChar(936, 0, szInBuf, -1, NULL, 0) + 1;
    wchar_t* pSrc = new wchar_t[nRelSize];
    memset(pSrc, 0, nRelSize * sizeof(wchar_t));
    nRelSize = MultiByteToWideChar(936, 0, szInBuf, nInBuflen, pSrc, nRelSize);
    nRelSize = WideCharToMultiByte(CP_UTF8, 0, pSrc, nRelSize, szOutBuf, nOutBuflen, NULL, NULL);
  }
  else{
    nRelSize = MultiByteToWideChar(CP_UTF8, 0, szInBuf, -1, NULL, 0);
    wchar_t* pSrc = new wchar_t[nRelSize + 1];
    pSrc[nRelSize] = 0;
    nRelSize = MultiByteToWideChar(CP_UTF8, 0, szInBuf, nInBuflen, pSrc, nRelSize);
    nRelSize = WideCharToMultiByte(936, 0, pSrc, nRelSize, szOutBuf, nOutBuflen, NULL, NULL);
  }
#else
  size_t nRelSize = 0;
  iconv_t convertHandler;
  if ((convertHandler = iconv_open(szToCharset, szFromCharset)) != (iconv_t)-1){
    char **pin = &szInBuf;
    char **pout = &szOutBuf;
    size_t temp_in_buffer_len = nInBuflen;
    size_t temp_out_buffer_len = nOutBuflen;
    memset(szOutBuf, 0, nOutBuflen);

    nRelSize = iconv(convertHandler, (char**)pin, &temp_in_buffer_len, pout, &temp_out_buffer_len);

    iconv_close(convertHandler);
    if (nRelSize == (size_t)(-1)){
      //      printf("iconv fail:nSrcLen=%d,nDestLen=%d,size=%zd,rel:%s err: %s\n",
      //        nInBuflen, nOutBuflen, nRelSize, szOutBuf, strerror(errno));
      nRelSize = -1;
    }
    else if (nRelSize == 0){
      nRelSize = nInBuflen;
    }
  }

#endif

  return nRelSize;
}

void ToLow(std::string& src)
{
  for (size_t idx = 0; idx < src.length(); idx++)
  {
    if (src[idx] >= 'A' && src[idx] <= 'Z')
    {
      src[idx] += 32;
    }
  }
}

void ToUpper(std::string& src){
  for (size_t idx = 0; idx < src.length(); idx++){
    if (src[idx] >= 'a' && src[idx] <= 'z'){
      src[idx] -= 32;
    }
  }
}

unsigned char ToHex(unsigned char x){
  return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x){
  unsigned char y = 0;
  if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
  else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
  else if (x >= '0' && x <= '9') y = x - '0';
  //else assert(0);
  return y;
}

std::string UrlEncode(const std::string& str, bool is_unicode){
  std::string* tmp = const_cast<std::string*>(&str);
  std::string unicode_str;
  if (is_unicode){
    unicode_str.resize(str.size() * 3);
    CodeConverter((char*)(str.c_str()), str.size(), (char*)(unicode_str.c_str()), unicode_str.size());
    unicode_str.resize(strlen(unicode_str.c_str()));
    tmp = &unicode_str;
  }
  std::string strTemp = "";
  size_t length = tmp->length();
  for (size_t i = 0; i < length; i++){
    if (isalnum((unsigned char)((*tmp)[i])) ||
      ((*tmp)[i] == '-') ||
      ((*tmp)[i] == '_') ||
      ((*tmp)[i] == '.') ||
      ((*tmp)[i] == '~'))
      strTemp += (*tmp)[i];
    else if ((*tmp)[i] == ' ')
      strTemp += "+";
    else{
      strTemp += '%';
      strTemp += ToHex((unsigned char)(*tmp)[i] >> 4);
      strTemp += ToHex((unsigned char)(*tmp)[i] % 16);
    }
  }
  return strTemp;
}

std::string UrlDecode(const std::string& str, bool is_unicode){
  std::string* tmp = const_cast<std::string*>(&str);
  std::string utf8_str;
  if (is_unicode){
    utf8_str.resize(str.size() * 3);
    CodeConverter((char*)(str.c_str()), str.size(), (char*)(utf8_str.c_str()), utf8_str.size(), "utf-8", "gbk");
    utf8_str.resize(strlen(utf8_str.c_str()));
    tmp = &utf8_str;
  }
  std::string strTemp = "";
  size_t length = tmp->length();
  for (size_t i = 0; i < length; i++){
    if ((*tmp)[i] == '+') strTemp += ' ';
    else if ((*tmp)[i] == '%'){
      //assert(i + 2 < length);
      unsigned char high = FromHex((unsigned char)(*tmp)[++i]);
      unsigned char low = FromHex((unsigned char)(*tmp)[++i]);
      strTemp += high * 16 + low;
    }
    else strTemp += (*tmp)[i];
  }
  return strTemp;
}

////////for web brower
std::string UrlEncode_ext(const std::string& str, bool is_unicode){
  std::string* tmp = const_cast<std::string*>(&str);
  std::string unicode_str;
  if (is_unicode){
    unicode_str.resize(str.size() * 3);
    CodeConverter((char*)(str.c_str()), str.size(), (char*)(unicode_str.c_str()), unicode_str.size());
    unicode_str.resize(strlen(unicode_str.c_str()));
    tmp = &unicode_str;
  }
  std::string strTemp = "";
  size_t length = tmp->length();
  for (size_t i = 0; i < length; i++){
    if (isalnum((unsigned char)((*tmp)[i])) ||
      ((*tmp)[i] == '-') ||
      ((*tmp)[i] == '_') ||
      ((*tmp)[i] == '.') ||
      ((*tmp)[i] == '~'))
      strTemp += (*tmp)[i];
    else if ((*tmp)[i] == ' ')
      strTemp += "%20";
    else{
      strTemp += '%';
      strTemp += ToHex((unsigned char)(*tmp)[i] >> 4);
      strTemp += ToHex((unsigned char)(*tmp)[i] % 16);
    }
  }
  return strTemp;
}

// in must be at least len bytes
// len must be 1, 2, or 3
// buf must be a buffer of at least 4 bytes
static void _base64_encode(
  const uint8_t *in, uint32_t len, uint8_t *buf) {
  static const uint8_t *kEncodeTable = (const uint8_t *)
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
    "ghijklmnopqrstuvwxyz0123456789+/";
  buf[0] = kEncodeTable[(in[0] >> 2) & 0x3F];
  if (len == 3) {
    buf[1] = kEncodeTable[((in[0] << 4) + (in[1] >> 4)) & 0x3f];
    buf[2] = kEncodeTable[((in[1] << 2) + (in[2] >> 6)) & 0x3f];
    buf[3] = kEncodeTable[in[2] & 0x3f];
  }
  else if (len == 2) {
    buf[1] = kEncodeTable[((in[0] << 4) + (in[1] >> 4)) & 0x3f];
    buf[2] = kEncodeTable[(in[1] << 2) & 0x3f];
  }
  else  { // len == 1
    buf[1] = kEncodeTable[(in[0] << 4) & 0x3f];
  }
}

// buf must be a buffer of at least 4 bytes
// buf will be changed to contain output bytes
// len is number of bytes to consume from input (must be 2, 3, or 4)
static void _base64_decode(uint8_t *buf, uint32_t len) {
  static const uint8_t kDecodeTable[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };
  buf[0] = (kDecodeTable[buf[0]] << 2)
    | (kDecodeTable[buf[1]] >> 4);
  if (len > 2){
    buf[1] = ((kDecodeTable[buf[1]] << 4) & 0xf0)
      | (kDecodeTable[buf[2]] >> 2);
    if (len > 3) buf[2] = ((kDecodeTable[buf[2]] << 6) & 0xc0)
      | (kDecodeTable[buf[3]]);
  }
}

static int go_base64_encode(const uint8_t *src,
  int src_len, char *dst, int dst_len, int wraps) {
  uint8_t buf[512];
  char *mark = dst;
  int cnt = 0;
  if (wraps > (int)sizeof(buf)-4) {
    wraps = sizeof(buf)-4; // left 4 bytes
  }
  while (src_len >= 3) {
    _base64_encode(src, 3, &buf[cnt]);
    src += 3;
    src_len -= 3;
    cnt += 4;
    if (cnt >= wraps){
      if (wraps > 0) buf[cnt++] = '\n';
      if (dst_len < cnt) return -1;
      memcpy(dst, buf, cnt);
      dst += cnt;
      dst_len -= cnt;
      cnt = 0;
    }
  }
  if (src_len > 0){
    int j = src_len + 1;
    _base64_encode(src, src_len, &buf[cnt]);
    cnt += j;
    while (j++ < 4) buf[cnt++] = '=';
  }
  if (cnt > 0){
    if (dst_len < cnt) return -1;
    memcpy(dst, buf, cnt);
    dst += cnt;
    dst_len -= cnt;
  }
  if (dst_len > 0) *dst = '\0';
  return (dst - mark);
}

static int go_base64_decode(
  const char *src, int src_len, uint8_t *dst, int dst_len){
  uint8_t buf[4], *mark = dst;
  int cnt = 0, stop = 0;
  for (; stop == 0 && src_len > 0; src++, src_len--){
    switch (*src) { // skip all white spaces.
    case '\n': case '\r':
    case '\t': case ' ':
    case '\f': case '\b':
    case 0177: break;
    case '=': { stop = 1; break; }
    default: { buf[cnt++] = *src; }
    }
    if (cnt == 4 || (stop && cnt > 1)){
      _base64_decode(buf, cnt--);
      if (dst_len < cnt) return -1;
      memcpy(dst, buf, cnt);
      dst += cnt;
      dst_len -= cnt;
      cnt = 0;
    }
  }
  return (dst - mark);
}

static inline char* getArray(std::string &str){
  std::string::iterator it = str.begin();
  return str.empty() ? NULL : &(*it);
}

void SafeBase64ToBase64(std::string& url64){
  std::replace(url64.begin(), url64.end(), '-', '+');
  std::replace(url64.begin(), url64.end(), '_', '/');
  int len = url64.length();
  int fill_len = (4 - len%4)%4;
  for(int i = 0; i < fill_len; ++i){
    url64 += '=';
  }
}

std::string Base64Decode(const char *str, size_t len){
  size_t siz = (len > 0 ? len : strlen(str));
  size_t dst_len = siz + 1;
  std::string result;
  result.resize(dst_len);

  uint8_t *dst = (uint8_t *)getArray(result);
  int j = go_base64_decode(str, siz, dst, dst_len);
  result.resize(j);
  return result;
}
std::string Base64Encode(const void *src, size_t len, int wraps){
  size_t dst_len = (4 * len + 2) / 3 + 3 +
    (wraps > 0 ? len / (wraps > 64 ? 64 : wraps) : 0);
  std::string result;
  result.resize(dst_len);
  char *dst = getArray(result);
  int j = go_base64_encode(
    (const uint8_t *)src, len, dst, dst_len, wraps);
  result.resize(j);
  return result;
}

void BinaryTo16Hex(const std::string& src, std::string& dest){
  static const char HEX_STR[] = "0123456789abcdef";
  dest.reserve(src.size() * 2 + 1);
  dest.clear();
  for (size_t idx = 0; idx < src.size(); idx++)
  {
    dest += HEX_STR[(src[idx] >> 4) & 0xF];
    dest += HEX_STR[src[idx] & 0xF];
  }
}

static inline std::string& ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end 
static inline std::string& rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

std::string& Trim(std::string& s) {
  return ltrim(rtrim(s));
}

int UrlSafeBase64Encode(const uint8_t *src, int src_len, char *dst){
  static const char _t64[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
  };
  int i, loop = src_len / 3;
  char *mark = dst;
  for (i = 0; i < loop; ++i, src += 3){
    *dst++ = _t64[((src[0] >> 2))];
    *dst++ = _t64[((src[0] & 0x03) << 4) | (src[1] >> 4)];
    *dst++ = _t64[((src[1] & 0x0f) << 2) | (src[2] >> 6)];
    *dst++ = _t64[((src[2] & 0x3f))];
  }
  switch (src_len % 3){
  case 2:
    *dst++ = _t64[((src[0] >> 2))];
    *dst++ = _t64[((src[0] & 0x03) << 4) | (src[1] >> 4)];
    *dst++ = _t64[((src[1] & 0x0f) << 2)];
    break;
  case 1:
    *dst++ = _t64[((src[0] >> 2))];
    *dst++ = _t64[((src[0] & 0x03) << 4)];
    break;
  }
  return dst - mark;
}

int UrlSafeBase64Decode(const char *src, int src_len, uint8_t *dst)
{
  static const char _t256[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };
  int i, loop = src_len / 4;
  uint8_t *mark = dst;
  char p0, p1, p2, p3;
  const uint8_t *s = (const uint8_t *)src;
  for (i = 0; i < loop; ++i){
    if ((p0 = _t256[(int)*s++]) < 0) return -1;
    if ((p1 = _t256[(int)*s++]) < 0) return -1;
    if ((p2 = _t256[(int)*s++]) < 0) return -1;
    if ((p3 = _t256[(int)*s++]) < 0) return -1;
    *dst++ = (p0 << 2) | (p1 >> 4);
    *dst++ = ((p1 << 4) & 0xf0) | (p2 >> 2);
    *dst++ = ((p2 << 6) & 0xc0) | (p3);
  }
  switch (src_len % 4){
  case 0:
    break;
  case 3:
    if ((p0 = _t256[(int)*s++]) < 0) return -1;
    if ((p1 = _t256[(int)*s++]) < 0) return -1;
    if ((p2 = _t256[(int)*s++]) < 0) return -1;
    *dst++ = (p0 << 2) | (p1 >> 4);
    *dst++ = ((p1 << 4) & 0xf0) | (p2 >> 2);
    if ((p2 & 0x3) != 0) return -1;
    break;
  case 2:
    if ((p0 = _t256[(int)*s++]) < 0) return -1;
    if ((p1 = _t256[(int)*s++]) < 0) return -1;
    *dst++ = (p0 << 2) | (p1 >> 4);
    if ((p1 & 0xf) != 0) return -1;
    break;
  default:
    return -2;
  }
  return dst - mark;
}

}
}
