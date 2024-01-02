//
//  SimdCrack.hpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#include <map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include "PreimageContext.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"
#include "DispatchQueue.hpp"
#include "SharedRefptr.hpp"

#ifndef SimdCrack_hpp
#define SimdCrack_hpp

class SimdCrack
{
public:
    SimdCrack(std::vector<uint8_t> Target)
        : m_Target(Target) {};
    SimdCrack(std::vector<uint8_t> Target, WordGenerator Generator)
        : m_Target(Target),m_Generator(Generator) {};
    void InitAndRun(void);
    void SetBlocksize(const size_t BlockSize) { m_Blocksize = BlockSize; };
private:
    void ProcessContext(PreimageContext*);
    void GenerateBlock(PreimageContext* Context, const size_t Start, const size_t Step, size_t* Next);
    void GenerateBlocks(const size_t Start, const size_t Step);
    void BlockProcessed(std::string Result);
    void FoundResult(std::string Result);

    dispatch::DispatcherPoolPtr m_DispatchPool;
    std::vector<uint8_t> m_Target;
    std::map<size_t, PreimageContext> m_Contexts;
    WordGenerator m_Generator;
    bool m_Found = false;
    std::mutex m_WordLock;
    size_t m_Threads = 0;
    size_t m_Blocksize = 10000;
};

#endif // SimdCrack_hpp