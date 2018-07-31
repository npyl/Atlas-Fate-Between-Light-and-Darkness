#pragma once 

#include "modules/module.h" 


class CModuleMultithread : public IModule
{
public:

	CModuleMultithread(const std::string& aname) : IModule(aname) { }
	virtual bool start() override;
	virtual void update(float delta) override;

	int const getThreadsNumber() { return _ndefThreads; }
	bool const isMultithreadingEnabled() { return _enabledMultithreading; }

private:

	int _ndefThreads;
	bool _enabledMultithreading = false;
};