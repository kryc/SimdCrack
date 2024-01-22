#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <gmpxx.h>

#include "WordGenerator.hpp"

#define D(val) (std::cout << "DBG: " << val << std::endl)

//
// Static
//
std::string
WordGenerator::GenerateWord(
    const size_t Value,
    const std::string& Charset,
    const bool Reverse
)
{
    std::string out;
    int64_t i;
    
    i = (int64_t)Value;
    
    while (i > 0)
    {
        i--;
        out += Charset[i % Charset.length()];
        i /= Charset.length();
    }

    if (Reverse)
    {
        std::reverse(std::begin(out), std::end(out));
    }
     
    return out;
}

const std::string
WordGenerator::Generate(
    const size_t Value,
    const bool Reverse
)
{
    return m_Prefix + GenerateWord(Value, m_Charset, Reverse) + m_Postfix;
}

//
// Static
//
std::string 
WordGenerator::GenerateWord(
    const mpz_class Value,
    const std::string& Charset,
    const bool Reverse
)
{
    std::string out;

    mpz_class i(Value);
    mpz_class r;
    
    while (i > 0)
    {
        i--;
        r = i % Charset.length();
        out += Charset[r.get_ui()];
        i /= Charset.length();
    }

    if (Reverse)
    {
        std::reverse(std::begin(out), std::end(out));
    }
     
    return out;
}

const std::string
WordGenerator::Generate(
    const mpz_class Value,
    const bool Reverse
)
{
    return m_Prefix + GenerateWord(Value, m_Charset, Reverse) + m_Postfix;
}