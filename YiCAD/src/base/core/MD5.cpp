/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file MD5.cpp
/// @brief MD5哈希值计算类实现

#include "MD5.h"

using namespace std;

// 右移的时候，高位一定要补零，而不是补充符号位
#define SHIFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

constexpr unsigned int MD5_A = 0x67452301;
constexpr unsigned int MD5_B = 0xefcdab89;
constexpr unsigned int MD5_C = 0x98badcfe;
constexpr unsigned int MD5_D = 0x10325476;

// strByte的长度
unsigned int MD5::strlength = 0;
// A,B,C,D的临时变量
unsigned int MD5::atemp = 0;
unsigned int MD5::btemp = 0;
unsigned int MD5::ctemp = 0;
unsigned int MD5::dtemp = 0;

const unsigned int MD5::k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8,
        0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193,
        0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51,
        0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905,
        0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681,
        0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60,
        0xbebfbc70, 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244,
        0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
        0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314,
        0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

const unsigned int MD5::s[] = { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7,
        12, 17, 22, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 6, 10,
        15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };

const char MD5::str16[] = "0123456789abcdef";

std::string MD5::getMD5(const std::string& source)
{
    atemp = MD5_A;    // 初始化
    btemp = MD5_B;
    ctemp = MD5_C;
    dtemp = MD5_D;
    unsigned int* strByte = add(source);
    for (unsigned int i = 0; i < strlength / 16; i++)
    {
        unsigned int num[16];
        for (unsigned int j = 0; j < 16; j++)
        {
            num[j] = strByte[i * 16 + j];
        }
        mainLoop(num);
    }
    return changeHex(atemp).append(changeHex(btemp)).append(changeHex(ctemp)).append(changeHex(dtemp));
}

void MD5::mainLoop(unsigned int M[])
{
    unsigned int f = 0;
    unsigned int g = 0;
    unsigned int a = atemp;
    unsigned int b = btemp;
    unsigned int c = ctemp;
    unsigned int d = dtemp;
    for (unsigned int i = 0; i < 64; i++)
    {
        if (i < 16)
        {
            f = F(b, c, d);
            g = i;
        }
        else if (i < 32)
        {
            f = G(b, c, d);
            g = (5 * i + 1) % 16;
        }
        else if (i < 48)
        {
            f = H(b, c, d);
            g = (3 * i + 5) % 16;
        }
        else
        {
            f = I(b, c, d);
            g = (7 * i) % 16;
        }
        unsigned int tmp = d;
        d = c;
        c = b;
        b = b + SHIFT((a + f + k[i] + M[g]), s[i]);
        a = tmp;
    }
    atemp = a + atemp;
    btemp = b + btemp;
    ctemp = c + ctemp;
    dtemp = d + dtemp;
}

unsigned int* MD5::add(std::string str)
{
    const unsigned int BLOCK_SIZE_BYTES = 64;
    const unsigned int INTS_PER_BLOCK = 16;    // 64/4=16,所以有16个整数
    const unsigned int BITS_PER_BYTE = 8;
    const unsigned int SHIFT_1_BYTE = 0x80;

    // 以512位,64个字节为一组
    unsigned int num = (unsigned int)((str.length() + 8) / BLOCK_SIZE_BYTES) + 1;
    unsigned int* strByte = new unsigned int[(unsigned int)(num * INTS_PER_BLOCK)];
    strlength = num * INTS_PER_BLOCK;
    for (unsigned int i = 0; i < num * INTS_PER_BLOCK; i++)
    {
        strByte[i] = 0;
    }
    for (unsigned int i = 0; i < str.length(); i++)
    {
        // 一个整数存储四个字节，i>>2表示i/4 一个unsigned int对应4个字节，保存4个字符信息
        strByte[i >> 2] |= (str[i]) << ((i % 4) * BITS_PER_BYTE);
    }
    // 尾部添加1 一个unsigned int保存4个字符信息,所以用128左移
    strByte[str.length() >> 2] |= SHIFT_1_BYTE << (((str.length() % 4)) * BITS_PER_BYTE);
    /*
     * 添加原长度，长度指位的长度，所以要乘8，然后是小端序，所以放在倒数第二个,这里长度只用了32位
     */
    strByte[num * INTS_PER_BLOCK - 2] = (unsigned int)(str.length() * BITS_PER_BYTE);
    return strByte;
}

std::string MD5::changeHex(int a)
{
    const int BYTE_COUNT = 4;
    const int NIBBLE_COUNT = 2;
    const int BITS_PER_BYTE = 8;
    const int NIBBLE_MASK = 0xff;

    int b = 0;
    string str1;
    string str = "";
    for (int i = 0; i < BYTE_COUNT; i++)
    {
        str1 = "";
        b = ((a >> i * BITS_PER_BYTE) % (1 << BITS_PER_BYTE)) & NIBBLE_MASK;   // 逆序处理每个字节
        for (int j = 0; j < NIBBLE_COUNT; j++)
        {
            str1.insert(0, 1, str16[b % 16]);
            b = b / 16;
        }
        str += str1;
    }
    return str;
}
