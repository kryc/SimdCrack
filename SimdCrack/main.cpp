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

size_t g_CurrentIndex = 0;
std::vector<uint8_t> g_Target;
std::map<size_t, std::shared_ptr<PreimageContext>> g_Contexts;
std::vector<std::shared_ptr<PreimageContext>> g_Queue;
std::vector<std::thread> g_Threads;
std::mutex g_Lock;
std::atomic<bool> g_Stop;

void
ClearBacklog(void)
{
	for (auto& [l, c] : g_Contexts)
	{
		if (c->GetLastIndex() >= g_CurrentIndex + 1000)
		{
			if (c->GetEntryCount() > 0)
				g_Queue.push_back(c);
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
			break;
		
		g_Lock.lock();
		if (!g_Queue.empty())
		{
			ctx = g_Queue.back();
			g_Queue.pop_back();
			gotJob = true;
		}
		g_Lock.unlock();
		
		if (gotJob && ctx->Check())
		{
			std::lock_guard<std::mutex> l(g_Lock);
			std::cout << ctx->GetMatch() << std::endl;
			g_Stop = true;
		}
	}
}

void
AddWord(std::string Word)
{
	size_t length = Word.size();
	
	if (Word.size() == 0)
		return;
	
	std::lock_guard<std::mutex> l(g_Lock);
	
	g_CurrentIndex++;
	
	if (g_Contexts[length] == nullptr)
	{
		g_Contexts[length] = std::make_shared<PreimageContext>(length, g_Target);
		g_Contexts[length]->Initialize(length, g_Target);
	}
	
	g_Contexts[length]->AddEntry(Word, g_CurrentIndex);
	if (g_Contexts[length]->IsFull())
	{
		g_Queue.push_back(g_Contexts[length]);
		g_Contexts.erase(length);
	}
	
	//
	// Check if we have any partial contexts that we
	// should process because we haven't seen any objects
	// of that length for a while
	//
	if ((g_CurrentIndex & (1024-1)) == 0)
	{
		ClearBacklog();
	}
}

int main(int argc, const char * argv[]) {
	std::string targetHex;
	
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <target>" << std::endl;
		return 0;
	}
	
	g_Target = Util::ParseHex(argv[1]);
	
	//
	// Start worker threads
	//
	for (size_t i = 0; i < std::thread::hardware_concurrency(); i++)
		g_Threads.push_back(std::thread(Worker));
	
	std::string line;
	while (std::getline(std::cin, line) && g_Stop == false)
	{
		AddWord(line);
	}
	
	{
		std::lock_guard<std::mutex> l(g_Lock);
		ClearBacklog();
	}
	
	g_Stop = true;
	for (auto& t : g_Threads)
		t.join();
	
	return 0;
}
