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
	const HashAlgorithm Algo,
	const std::vector<uint8_t*>& Targets,
	const std::vector<size_t>& TargetCounts
) : m_TargetOffsets(Targets), m_TargetCounts(TargetCounts)
{
	m_Algorithm = Algo;
	m_HashWidth = GetHashWidth(Algo);
	m_SimdLanes = SimdLanes();
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
	const char* Word,
	const size_t Length
)
{
	assert(!IsFull());
	if (IsFull())
		return;
	
	if (Length != m_Length)
	{
		std::cerr << "[!] Invalid length passed to AddEntry" << std::endl;
		return;
	}
	
	char* nextEntry = (char*)m_BufferPointers[m_NextEntry++];
	memcpy(nextEntry, Word, m_Length);
}

void
PreimageContext::AddEntry(
	const std::string& Value
)
{
	AddEntry(&Value[0], Value.size());
}

void
PreimageContext::AddEntry(
	const char* Word,
	const size_t Length,
	const size_t Index
)
{
	m_LastIndex = Index;
	AddEntry(Word, Length);
}

void
PreimageContext::AddEntry(
	const std::string& Value,
	const size_t Index
)
{
	AddEntry(&Value[0], Value.size(), Index);
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

inline const uint8_t*
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
	uint8_t hashes[m_HashWidth * MAX_LANES];
	size_t lengths[MAX_LANES];

	for (size_t i = 0; i < m_SimdLanes; i++)
	{
		lengths[i] = m_Length;
	}

	SimdHashOptimized(
		m_Algorithm,
		lengths,
		(const uint8_t**)&m_BufferPointers[0],
		hashes
	);

	// ssize_t found;
	for (size_t index = 0; index < GetEntryCount(); index++)
	{
		// Binary search
		uint8_t* hash = &hashes[index * m_HashWidth];
		uint8_t firstbyte = hash[0];
		auto found = binary_search(
			m_TargetOffsets[firstbyte],
			hash,
			m_TargetCounts[firstbyte],
			m_HashWidth
		);

		if (found != nullptr)
		{
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
