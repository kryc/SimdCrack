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
	const size_t TargetCount,
	const uint8_t** TargetLookup,
	const size_t* TargetLookupCount
)
{
	m_Algorithm = Algo;
	m_Targets = Targets;
	m_TargetsCount = TargetCount;
	m_TargetLookup = TargetLookup;
	m_TargetLookupCounts = TargetLookupCount;
	m_SimdLanes = SimdLanes();

    if (m_Algorithm == Algorithm::sha256)
    {
        m_HashWidth = SHA256_SIZE;
    }
    else if (m_Algorithm == Algorithm::sha1)
    {
        m_HashWidth = SHA1_SIZE;
    }
    else if (m_Algorithm == Algorithm::md5)
    {
        m_HashWidth = MD5_SIZE;
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
		m_Buffer.reserve(m_Length * m_SimdLanes);
		m_BufferPointers.resize(m_SimdLanes);
		
		for (size_t i = 0; i < m_SimdLanes; i++)
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

const uint8_t*
binary_search(
	const uint8_t* arr,
	const uint8_t* x,
	const size_t len,
	const size_t Width
)
{
	size_t low = 0;
    size_t high = len - 1;
    while (low <= high) {
        size_t mid = (low + high) / 2;
		if (mid > len)
			break;
        int cmp = memcmp(arr + mid * Width, x, Width);
        if (cmp == 0) {
            return arr + mid * Width;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return nullptr;
}

void
PreimageContext::CheckAndHandle(
	ResultHandler Callback
)
{
	SimdHashContext shaContext;
	uint8_t hashes[m_HashWidth * SIMD_COUNT];

	if (m_Algorithm == Algorithm::sha256)
	{
		SimdSha256Init(&shaContext);
		SimdHashUpdateAll(&shaContext, m_Length, (const uint8_t**)&m_BufferPointers[0]);
		SimdSha256Finalize(&shaContext);
		SimdHashGetHashes(&shaContext, (uint8_t*)hashes);
	}
	else if (m_Algorithm == Algorithm::sha1)
	{
		SimdSha1Init(&shaContext);
		SimdHashUpdateAll(&shaContext, m_Length, (const uint8_t**)&m_BufferPointers[0]);
		SimdSha1Finalize(&shaContext);
		SimdHashGetHashes(&shaContext, (uint8_t*)hashes);
	}
	else if (m_Algorithm == Algorithm::md5)
	{
		SimdMd5Init(&shaContext);
		SimdHashUpdateAll(&shaContext, m_Length, (const uint8_t**)&m_BufferPointers[0]);
		SimdMd5Finalize(&shaContext);
		SimdHashGetHashes(&shaContext, (uint8_t*)hashes);
	}


	// ssize_t found;
	for (size_t index = 0; index < GetEntryCount(); index++)
	{
		// Binary search
		uint8_t lookupByte = hashes[index * m_HashWidth];
		if (m_TargetLookup[lookupByte] == nullptr)
		{
			continue;
		}
		auto found = binary_search(
			m_TargetLookup[lookupByte],
			&hashes[index * m_HashWidth],
			m_TargetLookupCounts[lookupByte],
			m_HashWidth
		);

		if (found != nullptr)
		{
			// const uint8_t* foundhash = m_Targets + found * m_HashWidth;
			std::vector<uint8_t> hash(found, found + m_HashWidth);
			m_Match = std::string((char*)m_BufferPointers[index], m_Length);
			Callback(std::move(hash), m_Match);
			m_Matched++;
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
