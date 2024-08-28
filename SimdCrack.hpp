//
//  SimdCrack.hpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include <gmpxx.h>

#include "PreimageContext.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"
#include "DispatchQueue.hpp"
#include "SharedRefptr.hpp"
#include "simdhash.h"

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
    void SetAlgorithm(const HashAlgorithm Algo) { m_Algorithm = Algo; m_HashWidth = GetHashWidth(m_Algorithm); };
    void SetHashList(const std::filesystem::path File) { m_HashList = File; };
    void SetBinaryHashList(const std::filesystem::path File) { m_BinaryHashList = File; };
    void SetThreads(const size_t Threads) { m_Threads = Threads; };
    void SetOutFile(const std::filesystem::path Outfile) { m_Outfile = Outfile; }
    void SetResume(const mpz_class Resume) { m_Resume = Resume; };
private:
    void GenerateBlock(PreimageContext* Context, mpz_class& Index, const size_t Step);
    void GenerateBlocks(const size_t ThreadId, const mpz_class Start, const size_t Step);
    void BlockProcessed(const std::vector<uint8_t> Hash, const std::string Result);
    void FoundResult(const std::vector<uint8_t> Hash, const std::string Result);
    bool ProcessHashList(void);
    bool AddHashToList(const std::string Hash);
    bool AddHashToList(const uint8_t* Hash);
    void ThreadPulse(const size_t ThreadId, const uint64_t BlockTime, const std::string Last);

    dispatch::DispatcherPoolPtr m_DispatchPool;
    std::vector<std::vector<uint8_t>> m_Target;
    std::map<size_t, PreimageContext> m_Contexts;
    WordGenerator m_Generator;
    size_t m_Found = 0;
    size_t m_Threads = 0;
    size_t m_Blocksize = 500000;
    HashAlgorithm m_Algorithm = HashUnknown;
    size_t m_HashWidth = SHA256_SIZE;
    std::filesystem::path m_HashList;
    std::filesystem::path m_BinaryHashList;
    FILE* m_BinaryFd = nullptr;
    uint8_t* m_Targets = nullptr;
    size_t   m_TargetsAllocated;
    size_t   m_TargetsCount;
    std::vector<uint8_t*> m_TargetOffsets;
	std::vector<size_t> m_TargetCounts;
    size_t   m_BlocksCompleted;
    std::map<size_t, uint64_t> m_LastBlockMs;
    std::string m_LastWord;
    std::string m_Outfile;
    std::ofstream m_OutfileStream;
    mpz_class m_Resume;
};

#endif // SimdCrack_hpp