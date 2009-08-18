#include "stdafx.h"

#include "ThreadPool.h"

class ThreadPoolImpl
{
public:
	ThreadPoolImpl();
	~ThreadPoolImpl();
	
	void Start(unsigned char num);
	void Stop();
	
	void SetSlot(unsigned char slotNo, ThreadPool::JobFuncPtr func, void* param);
	void ExecJob(unsigned char slotNo);
	void ExecJobs();
	bool WaitJobs(int timeOutMilliseconds);

private:
	bool bStarted_;
	static const unsigned char MAX_THREAD_COUNT = 255;
	
	struct ThreadProcCallerInfo
	{
		ThreadPoolImpl* pThis;
		unsigned char slotId;
	};
	ThreadProcCallerInfo threadProcCallerInfos_[ MAX_THREAD_COUNT ];
	static unsigned int __stdcall ThreadProcCaller(void* param);
	void ThreadProc(unsigned char slotId);
	
	ThreadPool::JobFuncPtr jobFuncPtrs_[ MAX_THREAD_COUNT ];
	void* jobFuncParams_[ MAX_THREAD_COUNT ];
	
	unsigned char nThreads_;
	HANDLE hThreads_[ MAX_THREAD_COUNT ];
	unsigned int threadIds_[ MAX_THREAD_COUNT ];
	HANDLE hBeginEvents_[ MAX_THREAD_COUNT ];
	HANDLE hEndEvents_[ MAX_THREAD_COUNT ];
	HANDLE hShutdownEvent_;
};

ThreadPoolImpl::ThreadPoolImpl()
	:
	bStarted_(false)
{
}

ThreadPoolImpl::~ThreadPoolImpl()
{
	Stop();
}

void ThreadPoolImpl::Start(unsigned char num)
{
	hShutdownEvent_ = CreateEvent(0, TRUE, FALSE, 0);
	
	nThreads_ = num;
	for (size_t i=0; i<nThreads_; ++i) {
		ThreadProcCallerInfo& info = threadProcCallerInfos_[i];
		info.pThis = this;
		info.slotId = i;
		HANDLE hThread = (HANDLE) _beginthreadex(NULL, 0, ThreadPoolImpl::ThreadProcCaller, &info, 0, &threadIds_[i]);
		hThreads_[i] = hThread;
		
		hBeginEvents_[i] = CreateEvent(0, TRUE, FALSE, 0);
		hEndEvents_[i] = CreateEvent(0, TRUE, FALSE, 0);
	}

	bStarted_ = true;
}

/* static */
unsigned int ThreadPoolImpl::ThreadProcCaller(void* param)
{
	const ThreadProcCallerInfo& info = * (const ThreadProcCallerInfo*)(param);
	info.pThis->ThreadProc(info.slotId);
	return 0;
}

void ThreadPoolImpl::ThreadProc(unsigned char slotId)
{
	HANDLE hEndEvent = hEndEvents_[slotId];
	HANDLE hBeginEvent = hBeginEvents_[slotId];
	HANDLE hWaitEvents[2];
	hWaitEvents[0] = hShutdownEvent_;
	hWaitEvents[1] = hBeginEvent;
	while (1) {
		DWORD ret = WaitForMultipleObjects(2, hWaitEvents, FALSE, INFINITE);
		ResetEvent(hBeginEvent);
		switch (ret - WAIT_OBJECT_0) {
		case 0:
			return;
		case 1:
			ThreadPool::JobFuncPtr jobFunc = jobFuncPtrs_[slotId];
			assert(jobFunc);
			jobFunc(slotId, jobFuncParams_[slotId]);
			SetEvent(hEndEvent);
		}
	}
}

void ThreadPoolImpl::Stop()
{
	if (!bStarted_) {
		return;
	}

	SetEvent(hShutdownEvent_);
	WaitForMultipleObjects(nThreads_, hThreads_, TRUE, INFINITE);
	for (size_t i=0; i<nThreads_; ++i) {
		CloseHandle(hThreads_[i]);
		CloseHandle(hBeginEvents_[i]);
		CloseHandle(hEndEvents_[i]);
	}
	CloseHandle(hShutdownEvent_);

	bStarted_ = false;
}

void ThreadPoolImpl::SetSlot(unsigned char slotNo, ThreadPool::JobFuncPtr func, void* param)
{
	jobFuncPtrs_[slotNo] = func;
	jobFuncParams_[slotNo] = param;
}

void ThreadPoolImpl::ExecJob(unsigned char slotNo)
{
	ResetEvent(hEndEvents_[slotNo]);
	SetEvent(hBeginEvents_[slotNo]);
}

void ThreadPoolImpl::ExecJobs()
{
	for (size_t i=0; i<nThreads_; ++i) {
		ExecJob(i);
	}
}

bool ThreadPoolImpl::WaitJobs(int timeOutMilliseconds)
{
	return WAIT_TIMEOUT != WaitForMultipleObjects(nThreads_, hEndEvents_, TRUE, timeOutMilliseconds);
}

ThreadPool::ThreadPool()
	:
	pImpl_(new ThreadPoolImpl())
{
}

ThreadPool::~ThreadPool()
{
	delete pImpl_;
}

void ThreadPool::Start(unsigned char num)
{
	pImpl_->Start(num);
}

void ThreadPool::Stop()
{
	pImpl_->Stop();
}

void ThreadPool::SetSlot(unsigned char slotNo, JobFuncPtr func, void* param)
{
	pImpl_->SetSlot(slotNo, func, param);
}

void ThreadPool::ExecJob(unsigned char slotNo)
{
	pImpl_->ExecJob(slotNo);
}

void ThreadPool::ExecJobs()
{
	pImpl_->ExecJobs();
}

bool ThreadPool::WaitJobs(int timeOutMilliseconds)
{
	return pImpl_->WaitJobs(timeOutMilliseconds);
}

