#pragma once 

#include "modules/module.h" 


class CModuleMultithread : public IModule
{
public:

	CModuleMultithread(const std::string& aname) : IModule(aname) { }
	virtual bool start() override;
	virtual void update(float delta) override;

	int const getThreadsNumber() { return _ndefThreads; }
private:
	int _ndefThreads;
};