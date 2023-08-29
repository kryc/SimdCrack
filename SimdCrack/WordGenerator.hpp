//
//  PreimageContext.hpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#ifndef WordGenerator_hpp
#define WordGenerator_hpp

#include <cstdint>
#include <string>
#include <vector>

static const std::string ALPHANUMERIC = std::string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
static const std::string LOWER = std::string("abcdefghijklmnopqrstuvwxyz");

class WordGenerator
{
public:
    WordGenerator(void) : m_Charset(ALPHANUMERIC) {};
    WordGenerator(const std::string& Charset) : m_Charset(Charset) {};
    std::string Next(void);
    static std::string Generate(const size_t Value, const std::string& Charset, const bool Reverse = true);
private:
    const std::string m_Charset;
    size_t m_Counter = 0;
    size_t m_Step = 1;
};

#endif /* WordGenerator_hpp */
