//
//  PreimageContext.hpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#ifndef PreimageContext_hpp
#define PreimageContext_hpp

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include <functional>
#include <simdhash.h>

#include "Algorithm.hpp"

using ResultHandler = std::function<void(const std::vector<uint8_t>, const std::string)>;

const size_t SIMD_COUNT = SimdLanes();

class PreimageContext{
public:
	PreimageContext(const Algorithm Algo, const uint8_t* Targets, const size_t TargetCount, const uint8_t** TargetLookup, const size_t* TargetLookupCount);
	~PreimageContext(void);
	void   	Initialize(const size_t Length);
	const bool	Initialized(void) const { return m_Length != (size_t)-1; };
	void   	Reset(void);
	void   	AddEntry(std::string& Value);
	void   	AddEntry(std::string& Value, const size_t Index);
	const std::string GetEntry(const size_t Index) const;
	const size_t 	GetEntryCount(void) const;
	const bool IsFull(void) const { return m_NextEntry == SIMD_COUNT; };
	const bool IsEmpty(void) const { return m_NextEntry == 0; };
	const size_t Remaining(void) const { return SIMD_COUNT - m_NextEntry; };
	bool   	Check(void);
	void   	CheckAndHandle(ResultHandler Callback);
	const std::string GetMatch(void) const { return m_Match; };
	const bool 		Matched(void) const { return m_Matched; };
	const size_t 	GetLength(void) const { return m_Length; };
	size_t 	GetLastIndex(void) const { return m_LastIndex; };
	const std::string GetLastEntry(void) const { return GetEntry(GetLastIndex()); };
private:
	size_t 		m_Length = (size_t)-1;
	std::vector<uint8_t>  m_Buffer;
	std::vector<uint8_t*> m_BufferPointers;
	size_t   	m_NextEntry = 0;
	std::string m_Match;
	size_t 	 	m_Matched = 0;
	size_t   	m_LastIndex = 0;
	const uint8_t* m_Targets;
    size_t   m_TargetsCount;
	size_t 	 m_HashWidth = SHA256_SIZE;
	Algorithm m_Algorithm = Algorithm::sha256;
	size_t   m_SimdLanes;
	const uint8_t** m_TargetLookup;
    const size_t* m_TargetLookupCounts;
};

#endif /* PreimageContext_hpp */
