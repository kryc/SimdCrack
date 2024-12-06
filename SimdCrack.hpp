//
//  SimdCrack.hpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#ifndef SimdCrack_hpp
#define SimdCrack_hpp

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include <gmpxx.h>

#include "HashList.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"
#include "DispatchQueue.hpp"
#include "SharedRefptr.hpp"
#include "simdhash.h"

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
    void SetHashList(const std::filesystem::path File) { m_HashFile = File; };
    void SetBinaryHashList(const std::filesystem::path File) { m_BinaryHashList = File; };
    void SetThreads(const size_t Threads) { m_Threads = Threads; };
    void SetOutFile(const std::filesystem::path Outfile) { m_Outfile = Outfile; }
    void SetResume(const mpz_class Resume) { m_Resume = Resume; };
private:
    void GenerateBlocks(const size_t ThreadId, const mpz_class Start, const size_t Step);
    void BlockProcessed(const std::vector<uint8_t> Hash, const std::string Result);
    void FoundResult(const std::string Hash, const std::string Result);
    void FoundResults(std::vector<std::tuple<std::string, std::string>> Results);
    bool ProcessHashList(void);
    bool AddHashToList(const std::string Hash);
    bool AddHashToList(const uint8_t* Hash);
    void ThreadPulse(const size_t ThreadId, const uint64_t BlockTime, const std::string Last);

    dispatch::DispatcherPoolPtr m_DispatchPool;
    std::vector<std::vector<uint8_t>> m_Target;
    HashList m_HashList;
    WordGenerator m_Generator;
    size_t m_Found = 0;
    size_t m_Threads = 0;
    size_t m_Blocksize = 512;
    HashAlgorithm m_Algorithm = HashAlgorithmUndefined;
    size_t m_HashWidth = SHA256_SIZE;
    std::filesystem::path m_HashFile;
    std::filesystem::path m_BinaryHashList;
    FILE* m_BinaryFd = nullptr;
    uint8_t* m_Targets = nullptr;
    size_t   m_TargetsSize;
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
    char m_Separator = ':';
};

#endif // SimdCrack_hpp