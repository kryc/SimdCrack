//
//  PreimageContext.cpp
//  SimdCrack
//
//  Created by Kryc on 14/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

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
	mTarget = Target;
	mLength = Length;
	
	if (mBuffer != nullptr)
	{
		free(mBuffer);
	}
	
	mBuffer = (uint8_t*) calloc(mLength * SIMD_COUNT, sizeof(uint8_t));
	if (mBuffer == nullptr)
	{
		// We should handle this...
	}
	
	for (size_t i = 0; i < SIMD_COUNT; i++)
	{
		mBufferPointers[i] = &mBuffer[mLength * i];
	}
}

PreimageContext::~PreimageContext(
	void)
{
	if (mBuffer != nullptr)
	{
		free(mBuffer);
		mBuffer = nullptr;
	}
}

void
PreimageContext::AddEntry(std::string& Value)
{
	if (IsFull())
		return;
	
	if (Value.size() != mLength)
	{
		std::cerr << "[!] Invalid length passed to AddEntry" << std::endl;
		return;
	}
	
	char* nextEntry = (char*)mBufferPointers[mNextEntry++];
	memcpy(nextEntry, &Value[0], mLength);
}

void
PreimageContext::AddEntry(std::string& Value, const size_t Index)
{
	mLastIndex = Index;
	AddEntry(Value);
}

std::string
PreimageContext::GetEntry(
	const size_t Index
)
{
	return std::string((char*)mBufferPointers[Index], mLength);
}

size_t
PreimageContext::GetEntryCount(void)
{
	return mNextEntry;
}

bool
PreimageContext::IsFull(void)
{
	return mNextEntry == SIMD_COUNT;
}

bool
PreimageContext::Check(void)
{
	ALIGN(32) SimdShaContext sha2Context;
	SimdSha256Init(&sha2Context, SIMD_COUNT);
#ifndef USE_SECONDPREIMAGE
	uint8_t hashes[SHA256_SIZE * SIMD_COUNT];

	SimdSha256Update(&sha2Context, mLength, (const uint8_t**)mBufferPointers);
	SimdSha256Finalize(&sha2Context);
	SimdSha256GetHashes(&sha2Context, (uint8_t*)hashes);

	for (size_t index = 0; index < GetEntryCount(); index++)
	{
		if (memcmp(&hashes[index * SHA256_SIZE], &mTarget[0], SHA256_SIZE) == 0)
		{
			mMatch = std::string((char*)mBufferPointers[index], mLength);
			return true;
		}
	}
	return false;
#else
	ALIGN(32) SimdSha2SecondPreimageContext sha2PreimageContext;
	
	SimdSha256SecondPreimageInit(&sha2PreimageContext, &sha2Context, mTarget.data());


	size_t result = SimdSha256SecondPreimage(&sha2PreimageContext, (const size_t)mLength, (const uint8_t**)mBufferPointers);
	if (result != (size_t)-1)
	{
		mMatch = std::string((char*)mBufferPointers[result], mLength);
		return true;
	}
	return false;
#endif
}

std::string
PreimageContext::GetMatch(void)
{
	return mMatch;
}

void
PreimageContext::Reset(void)
{
	memset(mBuffer, 0x00, mLength * SIMD_COUNT);
	mNextEntry = 0;
	mMatch.resize(0);
	
	// SimdShaContext sha2Context;
	// SimdSha256Init(&mSha2Context, SIMD_COUNT);
	// SimdSha256SecondPreimageInit(&mSha2PreimageContext, &sha2Context, mTarget.data());
}
