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

#include <gmpxx.h>

static const std::string LOWER = "abcdefghijklmnopqrstuvwxyz";
static const std::string UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const std::string ALPHA = LOWER + UPPER;
static const std::string NUMERIC = "0123456789";
static const std::string ASCII_SPECIAL = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
static const std::string ALPHANUMERIC = LOWER + UPPER + NUMERIC;
static const std::string ASCII = LOWER + UPPER + NUMERIC + ASCII_SPECIAL;

class WordGenerator
{
public:
    WordGenerator(void) : m_Charset(ALPHANUMERIC) {};
    WordGenerator(const std::string& Charset) : m_Charset(Charset) {};
    WordGenerator(const std::string& Charset, std::string Prefix, std::string Postfix)
        : m_Charset(Charset),m_Prefix(Prefix),m_Postfix(Postfix) {};
    static std::string GenerateWord(const size_t Value, const std::string& Charset, const bool Reverse = true);
    const std::string Generate(const size_t Value, const bool Reverse = true);
    static std::string GenerateWord(const mpz_class Value, const std::string& Charset, const bool Reverse = true);
    const std::string Generate(const mpz_class Value, const bool Reverse = true);
    static const std::vector<uint8_t> GenerateParsingLookupTable(const std::string& Charset);
    const std::vector<uint8_t> GenerateParsingLookupTable(void) const { return GenerateParsingLookupTable(m_Charset); };
    static const mpz_class Parse(const std::string& Word, const std::string& Charset);
    static const mpz_class Parse(const std::string& Word, const std::vector<uint8_t>& LookupTable);
    const mpz_class Parse(const std::string& Word) const { return Parse(Word, m_Charset); };
    void SetPrefix(std::string& Prefix) { m_Prefix = Prefix; };
    void SetPostfix(std::string& Postfix) { m_Postfix = Postfix; };
    const std::string GetCharset(void) { return m_Charset; };
private:
    const std::string m_Charset;
    const std::vector<uint8_t> m_LookupTable;
    std::string m_Prefix;
    std::string m_Postfix;
};

#endif /* WordGenerator_hpp */
