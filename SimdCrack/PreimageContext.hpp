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

using ResultHandler = std::function<void(std::string)>;

class PreimageContext{
public:
	PreimageContext(void) = default;
	PreimageContext(const size_t Length, std::vector<uint8_t>& Target);
	~PreimageContext(void);
	void   	Initialize(const size_t Length, std::vector<uint8_t>& Target);
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
	size_t 	GetLastIndex(void) { return m_LastIndex; };
private:
	size_t 		m_Length = (size_t)-1;
	std::vector<uint8_t> m_Target;
	std::vector<uint8_t> m_Buffer;
	uint8_t* 	m_BufferPointers[SIMD_COUNT] = {nullptr};
	size_t   	m_NextEntry = 0;
	std::string m_Match;
	bool 	 	m_Matched;
	size_t   	m_LastIndex = 0;
};

#endif /* PreimageContext_hpp */
