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
#include <simdhash.h>

#include "PreimageContext.hpp"
#include "SimdCrack.hpp"

PreimageContext::PreimageContext(
	const Algorithm Algo,
	const uint8_t* Targets,
	const size_t TargetCount
)
{
	m_Algorithm = Algo;
	m_Targets = Targets;
	m_TargetsCount = TargetCount;

	if (m_Algorithm == Algorithm::sha1)
	{
		m_HashWidth = SHA1_SIZE;
	}
	else
	{
		m_HashWidth = SHA256_SIZE;
	}
}

void
PreimageContext::Initialize(
	const size_t Length
)
{
	size_t previousLength;

	previousLength = m_Length;

	m_Length = Length;

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

void
PreimageContext::CheckAndHandle(
	ResultHandler Callback
)
{
	SimdShaContext shaContext;
	uint8_t hashes[m_HashWidth * SIMD_COUNT];

	if (m_Algorithm == Algorithm::sha256)
	{
		SimdSha256Init(&shaContext, SIMD_COUNT);
		SimdSha256Update(&shaContext, m_Length, (const uint8_t**)m_BufferPointers);
		SimdSha256Finalize(&shaContext);
		SimdSha256GetHashesUnrolled(&shaContext, (uint8_t*)hashes);
	}
	else if (m_Algorithm == Algorithm::sha1)
	{
		SimdSha1Init(&shaContext, SIMD_COUNT);
		SimdSha1Update(&shaContext, m_Length, (const uint8_t**)m_BufferPointers);
		SimdSha1Finalize(&shaContext);
		SimdSha1GetHashesUnrolled(&shaContext, (uint8_t*)hashes);
	}

	for (size_t index = 0; index < GetEntryCount(); index++)
	{
		for (size_t target = 0; target < m_TargetsCount; target++)
		{
			const uint8_t* nextTarget = &m_Targets[target * m_HashWidth];
			if (memcmp(&hashes[index * m_HashWidth], nextTarget, m_HashWidth) == 0)
			{
				std::vector<uint8_t> hash(nextTarget, nextTarget + m_HashWidth);
				m_Match = std::string((char*)m_BufferPointers[index], m_Length);
				Callback(std::move(hash), m_Match);
				m_Matched++;
			}
		}
	}
}

void
PreimageContext::Reset(void)
{
	memset(&m_Buffer[0], 0x00, m_Length * SIMD_COUNT);
	m_NextEntry = 0;
	m_Match.resize(0);
}
