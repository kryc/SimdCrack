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
#include "PreimageContext.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"
#include "DispatchQueue.hpp"
#include "SimdCrack.hpp"

#if 0
void
ClearBacklog(void)
{
	for (auto& [l, c] : g_Contexts)
	{
		if (c->GetLastIndex() >= g_CurrentIndex + 1000)
		{
			if (c->GetEntryCount() > 0)
				g_Queue.push_back(std::move(c));
			// Haven't seen this length for a while so delete it entirely
			g_Contexts.erase(l);
		}
	}
}

void
Worker(void)
{
	std::shared_ptr<PreimageContext> ctx;
	bool gotJob;
	
	{
		std::lock_guard<std::mutex> l(g_Lock);
		std::cout << "Starting worker thread" << std::endl;
	}
	for (;;)
	{
		gotJob = false;
		
		if (g_Stop == true)
		{
			// std::cout << "Stopping worker thread" << std::endl;
			break;
		}
		
		g_Lock.lock();
		if (!g_Queue.empty())
		{
			ctx = std::move(g_Queue.back());
			g_Queue.pop_back();
			gotJob = true;
		}
		g_Lock.unlock();

		// if (gotJob)
		// {
		// 	std::lock_guard<std::mutex> l(g_Lock);
		// 	std::cout << ctx->GetEntry(0) << std::endl;
		// }

		if (gotJob && ctx->Check())
		{
			std::lock_guard<std::mutex> l(g_Lock);
			std::cout << ctx->GetMatch() << std::endl;
			g_Stop = true;
		}
	}
}
#endif

int main(int argc, const char * argv[]) {

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <target>" << std::endl;
		return 0;
	}
	
	auto target = Util::ParseHex(argv[1]);
	auto cracker = new SimdCrack(target);
	
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

	//
	// Read word and add
	//
	/*for(int i = 0; i < 30; i++)
	{
		std::cout << WordGenerator::Generate(i, "abcdefgABCDEFG") << std::endl;
	}*/


#if 0

	//
	// Start worker threads
	//
	for (size_t i = 0; i < std::thread::hardware_concurrency(); i++)
		g_Threads.push_back(std::thread(Worker));
	
	std::string line;
	for(;;)
	{
		line = wg.Next();
		// std::cout << line << std::endl;
		AddWord(line);
	}
	// while (std::getline(std::cin, line) && g_Stop == false)
	// {
	// 	AddWord(line);
	// }
	
	{
		std::lock_guard<std::mutex> l(g_Lock);
		ClearBacklog();
	}
	
	g_Stop = true;
	for (auto& t : g_Threads)
		t.join();
#endif
	return 0;
}
