#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include "WordGenerator.hpp"

void
WordGenerator::Initialise(
    void
)
{
    m_Counters.resize(1);
    m_Counters[0] = (uint16_t)-1;
    m_Word = m_Charset[0];
}

#define D(val) (std::cout << "DBG: " << (val) << std::endl)

std::string
WordGenerator::Next(
    void
)
{
    for (size_t counter = 0; counter <= m_Counters.size(); counter++)
    {
        ++m_Counters[counter];
        // D(m_Counters[counter]);
        if (m_Counters[counter] < m_Charset.size())
        {
            m_Word[counter] = m_Charset[m_Counters[counter]];
            //D(m_Word[counter]);
            break;
        }
        else if (counter == m_Counters.size()-1)
        {
            m_Word[counter] = m_Charset[0];
            m_Counters[counter] = 0;
            m_Counters.push_back(0);
            m_Word += m_Charset[0];
            break;
        }
        else
        {
            m_Word[counter] = m_Charset[0];
            m_Counters[counter] = 0;
        }
    } 
    
    //D(m_Word);
    return m_Word;
}