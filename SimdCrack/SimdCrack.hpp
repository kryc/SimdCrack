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
#include <atomic>
#include <filesystem>

#include "Algorithm.hpp"
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
    SimdCrack(std::vector<std::vector<uint8_t>> Target)
        : m_Target(Target) {};
    SimdCrack(std::vector<std::vector<uint8_t>> Target, WordGenerator Generator)
        : m_Target(Target),m_Generator(Generator) {};
    ~SimdCrack(void);
    void InitAndRun(void);
    void SetBlocksize(const size_t BlockSize) { m_Blocksize = BlockSize; };
    void SetAlgorithm(const Algorithm Algo);
    void SetHashList(const std::filesystem::path File) { m_HashList = File; };
    void SetThreads(const size_t Threads) { m_Threads = Threads; };
private:
    void ProcessContext(PreimageContext*);
    void GenerateBlock(PreimageContext* Context, const size_t Start, const size_t Step, size_t* Next);
    void GenerateBlocks(const size_t Start, const size_t Step);
    void BlockProcessed(const std::vector<uint8_t> Hash, const std::string Result);
    void FoundResult(const std::vector<uint8_t> Hash, const std::string Result);
    bool ProcessHashList(void);
    bool AddHashToList(const std::string Hash);
    bool AddHashToList(const uint8_t* Hash);

    dispatch::DispatcherPoolPtr m_DispatchPool;
    std::vector<std::vector<uint8_t>> m_Target;
    std::map<size_t, PreimageContext> m_Contexts;
    WordGenerator m_Generator;
    size_t m_Found = 0;
    size_t m_Threads = 0;
    size_t m_Blocksize = 10000;
    Algorithm m_Algorithm = Algorithm::sha1;
    size_t m_HashWidth = SHA256_SIZE;
    std::filesystem::path m_HashList;
    uint8_t* m_Targets;
    size_t   m_TargetsAllocated;
    size_t   m_TargetsCount;
};

#endif // SimdCrack_hpp