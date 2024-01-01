//
//  PreimageContext.cpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include "PreimageContext.hpp"
#include <simdhash.h>

PreimageContext::PreimageContext(
	const size_t Length,
	std::vector<uint8_t>& Target)
{
	Initialize(Length, Target);
}

void
PreimageContext::Initialize(
	const size_t Length,
	std::vector<uint8_t>& Target)
{
	size_t previousLength;

	previousLength = m_Length;

	m_Target = Target;
	m_NextEntry = 0;
	m_LastIndex = 0;

	if (previousLength != Length)
	{
		m_Length = Length;
		m_Buffer.reserve(m_Length * SIMD_COUNT);
		
		for (size_t i = 0; i < SIMD_COUNT; i++)
		{
			m_BufferPointers[i] = &m_Buffer[m_Length * i];
		}
	}
}

PreimageContext::~PreimageContext(
	void
)
{
	// if (m_Buffer != nullptr)
	// {
	// 	free(m_Buffer);
	// 	m_Buffer = nullptr;
	// }
}

void
PreimageContext::AddEntry(
	std::string& Value
)
{
	assert(!IsFull());
	if (IsFull())
		return;
	
	if (Value.size() != m_Length)
	{
		std::cerr << "[!] Invalid length passed to AddEntry" << std::endl;
		return;
	}
	
	char* nextEntry = (char*)m_BufferPointers[m_NextEntry++];
	memcpy(nextEntry, &Value[0], m_Length);
}

void
PreimageContext::AddEntry(std::string& Value, const size_t Index)
{
	m_LastIndex = Index;
	AddEntry(Value);
}

const std::string
PreimageContext::GetEntry(
	const size_t Index
) const
{
	return std::string((char*)m_BufferPointers[Index], m_Length);
}

const size_t
PreimageContext::GetEntryCount(
	void
) const
{
	return m_NextEntry;
}

bool
PreimageContext::Check(void)
{
	ALIGN(32) SimdShaContext sha2Context;
	SimdSha256Init(&sha2Context, SIMD_COUNT);
#ifndef USE_SECONDPREIMAGE
	uint8_t hashes[SHA256_SIZE * SIMD_COUNT];

	SimdSha256Update(&sha2Context, m_Length, (const uint8_t**)m_BufferPointers);
	SimdSha256Finalize(&sha2Context);
	SimdSha256GetHashesUnrolled(&sha2Context, (uint8_t*)hashes);

	for (size_t index = 0; index < GetEntryCount(); index++)
	{
		if (memcmp(&hashes[index * SHA256_SIZE], &m_Target[0], SHA256_SIZE) == 0)
		{
			m_Match = std::string((char*)m_BufferPointers[index], m_Length);
			m_Matched = true;
			return true;
		}
	}

	m_Matched = false;
	return m_Matched;
#else
	ALIGN(32) SimdSha2SecondPreimageContext sha2PreimageContext;
	
	SimdSha256SecondPreimageInit(&sha2PreimageContext, &sha2Context, m_Target.data());


	size_t result = SimdSha256SecondPreimage(&sha2PreimageContext, (const size_t)m_Length, (const uint8_t**)m_BufferPointers);
	if (result != (size_t)-1)
	{
		m_Match = std::string((char*)m_BufferPointers[result], m_Length);
		return true;
	}
	return false;
#endif
}

void
PreimageContext::CheckAndHandle(
	ResultHandler Callback
)
{
	if (Check())
	{
		Callback(m_Match);
	}
}

void
PreimageContext::Reset(void)
{
	memset(&m_Buffer[0], 0x00, m_Length * SIMD_COUNT);
	m_NextEntry = 0;
	m_Match.resize(0);
	
	// SimdShaContext sha2Context;
	// SimdSha256Init(&mSha2Context, SIMD_COUNT);
	// SimdSha256SecondPreimageInit(&mSha2PreimageContext, &sha2Context, m_Target.data());
}
