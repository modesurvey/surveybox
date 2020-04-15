/* 
  Inclusion of copyright notice from original source below. Work has been adapted to
  allow for url-safe base64 conversions.


  base64.cpp and base64.h

  base64 encoding and decoding with C++.

  Version: 1.01.00

  Copyright (C) 2004-2017 René Nyffenegger

  This source code is provided 'as-is', without any express or implied
  warranty. In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this source code must not be misrepresented; you must not
     claim that you wrote the original source code. If you use this source code
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original source code.

  3. This notice may not be removed or altered from any source distribution.

  René Nyffenegger rene.nyffenegger@adp-gmbh.ch
*/

#include "base64.hpp"

namespace
{
  const std::string BASE64_URL_CHARS = 
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "abcdefghijklmnopqrstuvwxyz"
              "0123456789-_";
}

namespace base64
{
  std::string encode_urlsafe(const std::string& data)
  {
      return encode_urlsafe(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
  }

  std::string encode_urlsafe(const uint8_t* data, size_t len)
  {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (len--)
    {
      char_array_3[i++] = *(data++);
      if (i == 3)
      {
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int t = 0; t < i + 1; t++)
          ret += BASE64_URL_CHARS[char_array_4[t]];
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
        ret += BASE64_URL_CHARS[char_array_4[t]];
    }

    return ret;
  }
}
