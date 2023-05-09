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
#include <simdhash.h>

class PreimageContext{
public:
	PreimageContext(void) = default;
	PreimageContext(const size_t Length, std::vector<uint8_t>& Target);
	~PreimageContext(void);
	void   Initialize(const size_t Length, std::vector<uint8_t>& Target);
	void   Reset(void);
	void   AddEntry(std::string& Value);
	void   AddEntry(std::string& Value, const size_t Index);
	std::string GetEntry(const size_t Index);
	size_t GetEntryCount(void);
	bool   IsFull(void);
	bool   Check(void);
	std::string GetMatch(void);
	size_t GetLength(void) { return mLength; };
	size_t GetLastIndex(void) { return mLastIndex; };
private:
	// ALIGN(32) SimdShaContext mSha2Context;
	// SimdSha2SecondPreimageContext mSha2PreimageContext;
	size_t mLength = 0;
	std::vector<uint8_t> mTarget;
	uint8_t* mBuffer = nullptr;
	uint8_t* mBufferPointers[SIMD_COUNT] = {nullptr};
	size_t   mNextEntry = 0;
	std::string mMatch;
	size_t   mLastIndex = 0;
};

using PreimageContextPtr = std::unique_ptr<PreimageContext>;

#endif /* PreimageContext_hpp */
