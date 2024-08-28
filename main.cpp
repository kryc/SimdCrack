//
//  main.cpp
//  SimdCrack
//
//  Created by Kryc on 13/09/2020.
//  Copyright © 2020 Kryc. All rights reserved.
//

#include <map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <sstream>
#include <filesystem>
#include <assert.h>
#include <gmpxx.h>

#include "PreimageContext.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"
#include "DispatchQueue.hpp"
#include "SimdCrack.hpp"

int main(
	int argc,
	const char * argv[]
)
{
	std::string prefix;
	std::string postfix;
	std::vector<uint8_t> target;
	std::vector<std::vector<uint8_t>> targets;
	std::string charset;
	std::string extra;
	size_t blocksize;
	HashAlgorithm algo;
	std::filesystem::path hashlist;
	std::filesystem::path binaryHashlist;
	size_t threads;
	std::string outfile;
	std::string resume;
	mpz_class resumeIndex;

	algo = HashUnknown;
	threads = 0;
	blocksize = 0;

	std::cerr << "SIMDCrack Hash Cracker" << std::endl;

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << "[options] <target>" << std::endl;
		std::cerr << "SIMD Lanes: " << SimdLanes() << std::endl;
		return 0;
	}
	
	charset = ASCII;

	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if (arg == "--outfile" || arg == "-o")
		{
			if (argc <= i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			outfile = argv[++i];
		}
		else if (arg == "--resume" || arg == "-r")
		{
			if (argc <= i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			resume = argv[++i];
		}
		else if (arg == "--blocksize" || arg == "-b")
		{
			if (argc <= i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			std::stringstream ss(argv[++i]);
			ss >> blocksize;
		}
		else if (arg == "--threads" || arg == "-t")
		{
			if (argc <= i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			std::stringstream ss(argv[++i]);
			ss >> threads;
		}
		else if (arg == "--prefix" || arg == "-f")
		{
			if (argc <= i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			prefix = argv[++i];
		}
		else if (arg == "--postfix" || arg == "-a")
		{
			if (argc == i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}
			postfix = argv[++i];
		}
		else if (arg == "--charset" || arg == "-c")
		{
			if (argc == i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}

			std::string charset_str(argv[++i]);
			charset = ParseCharset(charset_str);
			if (charset == "")
			{
				std::cerr << "Unrecognised character set" << std::endl;
				return 1;
			}
		}
		else if (arg == "--extra" || arg == "-e")
		{
			if (argc == i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}

			extra = argv[++i];
		}
		else if (arg == "--sha256")
		{
			algo = HashSha256;
		}
		else if (arg == "--sha1")
		{
			algo = HashSha1;
		}
		else if (arg == "--md5")
		{
			algo = HashMd5;
		}
		else if (arg == "--list" || arg == "-l")
		{
			if (argc == i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}

			hashlist = argv[++i];
			if (!std::filesystem::exists(hashlist))
			{
				std::cerr << "Hash list file not found " << hashlist << std::endl;
				return 1;
			}
		}
		else if (arg == "--binarylist" || arg == "-bl")
		{
			if (argc == i)
			{
				std::cerr << "No value specified for " << arg << std::endl;
				return 1;
			}

			binaryHashlist = argv[++i];
			if (!std::filesystem::exists(binaryHashlist))
			{
				std::cerr << "Hash list file not found " << binaryHashlist << std::endl;
				return 1;
			}
		}
		else
		{
			assert(arg[0] != '-');
			target = Util::ParseHex(arg);
			targets.push_back(target);
		}
	}

	//
	// Append any extra characters
	//
	charset += extra;

	std::cerr << "Using character set: " << charset << std::endl;
	
	auto generator = WordGenerator(charset, prefix, postfix);

	if (!resume.empty())
	{
		resumeIndex = generator.Parse(resume);
		std::cout << "Resuming from '" << resume << "' (Index " << resumeIndex.get_str() << ") " << std::endl;
	}

	auto cracker = new SimdCrack(std::move(targets), std::move(generator));
	cracker->SetAlgorithm(algo);
	
	if (!hashlist.empty())
	{
		cracker->SetHashList(hashlist);
	}

	if (!binaryHashlist.empty())
	{
		cracker->SetBinaryHashList(binaryHashlist);
	}
	
	if (blocksize != 0)
	{
		cracker->SetBlocksize(blocksize);
	}

	if (threads != 0)
	{
		cracker->SetThreads(threads);
	}

	if (!outfile.empty())
	{
		cracker->SetOutFile(outfile);
	}

	if (!resume.empty())
	{
		cracker->SetResume(std::move(resumeIndex));
	}

	//
	// Create the main dispatcher
	//
	auto mainDispatcher = dispatch::CreateAndEnterDispatcher(
		"main",
		dispatch::bind(
			&SimdCrack::InitAndRun,
			cracker
		)
	);

	return 0;
}
