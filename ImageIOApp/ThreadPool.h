#pragma once

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	
	void Start(unsigned char num);
	void Stop();
	
	typedef void (*JobFuncPtr)(unsigned char slotNo, void* param);
	void SetSlot(unsigned char slotNo, JobFuncPtr func, void* param);
	void ExecJob(unsigned char slotNo);
	void ExecJobs();
	void WaitJobs(int timeOutMilliseconds = -1);
	
private:
	class ThreadPoolImpl* pImpl_;
};

