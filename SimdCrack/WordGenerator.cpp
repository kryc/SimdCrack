#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include "WordGenerator.hpp"

#define D(val) (std::cout << "DBG: " << val << std::endl)

//
// Static
//
std::string
WordGenerator::Generate(
    const size_t Value,
    const std::string& Charset,
    const bool Reverse
)
{
    std::string out;
    int64_t i;
    // int64_t d, r;
    
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

std::string
WordGenerator::Next(
    void
)
{
    std::string next = Generate(m_Counter, m_Charset);
    m_Counter += m_Step;
    return next;
}