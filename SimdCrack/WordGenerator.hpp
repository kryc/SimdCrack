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

class WordGenerator
{
public:
    const std::string ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    WordGenerator(void) : m_Charset(ALPHANUMERIC) { Initialise(); };
    WordGenerator(std::string Charset) : m_Charset(Charset) { Initialise(); };
    void Initialise(void);
    std::string Next(void);
private:
    std::string m_Charset;
    std::vector<uint16_t> m_Counters;
    std::string m_Word;
};

#endif /* WordGenerator_hpp */
