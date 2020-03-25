// TODO: Give proper credit.

#include "base64.hpp"

static const std::string base64_url_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789-_";

namespace base64
{
  std::string encode_urlsafe(const std::string& data)
  {
      // TODO: What happens with utf8?
      return encode_urlsafe((uint8_t *) data.c_str(), data.length());
  }

  std::string encode_urlsafe(const uint8_t * data, size_t len)
  {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (len--) {
      char_array_3[i++] = *(data++);
      if (i == 3) {
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int t = 0; t < i + 1; t++)
          ret += base64_url_chars[char_array_4[t]];
        i = 0;
      }
    }

    if (i)
    {
      for(j = i; j < 3; j++)
        char_array_3[j] = '\0';

      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

      for (int t = 0; t < i + 1; t++)
        ret += base64_url_chars[char_array_4[t]];
    }

    return ret;
  }
}
