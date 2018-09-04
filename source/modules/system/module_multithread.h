#pragma once 

#include "modules/module.h" 
#include <future>


class CModuleMultithread : public IModule
{
public:

	CModuleMultithread(const std::string& aname) : IModule(aname) { }
	virtual bool start() override;
	virtual void update(float delta) override;

	int const getThreadsNumber() { return _ndefThreads; }
	bool const isMultithreadingEnabled() { return _enabledMultithreading; }
	std::future<bool> fut;

private:

	int _ndefThreads;
	bool _enabledMultithreading = true;
};