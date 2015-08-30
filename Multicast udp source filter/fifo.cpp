#include "fifo.h"
#include <windows.h>

// FIFO type I

// Constructor
CFifo::CFifo(bool dynamic)
{
	CFifo::count = 0;
	CFifo::head = NULL;
	CFifo::tail = NULL;
	ZeroMemory(used_indexes,sizeof(used_indexes));
	InitializeCriticalSection(&m_CriticalSection);
	m_data_rcvd_event =  CreateEvent (NULL, FALSE, FALSE, NULL) ; //auto reset non signaled
	dyn_buf = dynamic;
}

//Destructor
CFifo::~CFifo()
{
    while (count!=0)
	{
		if (dyn_buf)
		{
			delete[] head->buffer->buf;
			delete head->buffer;
		}
		CFifo::pop();
	}
	DeleteCriticalSection(&m_CriticalSection);
	CloseHandle(m_data_rcvd_event);
}


int CFifo::push(buffer_t * buffer)
{
	EnterCriticalSection(&m_CriticalSection);

	unit_t* ut;
	if (dyn_buf)
	{
		ut = new unit_t;
	}
	else
	{
		used_indexes[buffer->index]=true;
		ut = &unit_array[buffer->index];
	}
	
	ut->buffer=buffer;

	if (count >= 1)
		CFifo::tail->ref = ut;
	
	tail = ut;

	if (count == 0)
	{
		CFifo::head = ut;
		CFifo::head->ref = NULL;
		CFifo::tail->ref = NULL;
	}
	
	count++;
	int indx = first_free_index();
	
	LeaveCriticalSection(&m_CriticalSection);

	SetEvent(m_data_rcvd_event);

	return indx;
}


void CFifo::pop()
{
	EnterCriticalSection(&m_CriticalSection);

	if (count > 0)
	{
		if (dyn_buf)
		{
			unit_t* newhead=CFifo::head->ref;
			delete CFifo::head;
			CFifo::head=NULL;
			CFifo::head=newhead;
		}
		else
		{
			used_indexes[CFifo::head->buffer->index]=false;
			CFifo::head= CFifo::head->ref;
		}

        count--;
	}

	LeaveCriticalSection(&m_CriticalSection);
}


unit_t* CFifo::front(int* items)
{
    EnterCriticalSection(&m_CriticalSection);
	unit_t* answer;
	answer = CFifo::head;
	*items=count;
	LeaveCriticalSection(&m_CriticalSection);
	return answer;
}

int CFifo::items()
{
	int answer;
	EnterCriticalSection(&m_CriticalSection);
	answer =count;
	LeaveCriticalSection(&m_CriticalSection);
    return answer;
}

int CFifo::get_free_index()
{
	int answer;
	EnterCriticalSection(&m_CriticalSection);
	answer = first_free_index();
	LeaveCriticalSection(&m_CriticalSection);
	return answer;
}

int CFifo::first_free_index()
{
	int answer = -1;
	for (int i = 0; i< BUFFER_T_NUMBER; i++)
	{
		if (used_indexes[i]==false)
		{
			answer = i;
			break;
		}
	}
	return answer;
}




// Fifo type II

CFifoES::CFifoES()
{
	CFifoES::count = 0;
	CFifoES::head = NULL;
	CFifoES::tail = NULL;
	InitializeCriticalSection(&m_CriticalSection);
}

//Destructor
CFifoES::~CFifoES()
{
    while (count!=0)
	{
		delete[] head->buffer->buf;
		delete head->buffer;
		CFifoES::pop();
	}
	DeleteCriticalSection(&m_CriticalSection);
}

void CFifoES::empty()
{
	EnterCriticalSection(&m_CriticalSection);
	while (count!=0)
	{
		delete[] head->buffer->buf;
		delete head->buffer;
		CFifoES::pop();
	}
	LeaveCriticalSection(&m_CriticalSection);
}

void CFifoES::lock(bool lck)
{
    if (lck)
		EnterCriticalSection(&m_CriticalSection);
	else
		LeaveCriticalSection(&m_CriticalSection);
}

void CFifoES::push(bufferES_t * buffer)
{
	
	EnterCriticalSection(&m_CriticalSection);

	unitES_t* u = new unitES_t;
	u->buffer = buffer;

	if (count >= 1)
		CFifoES::tail->ref = u;
	
	tail = u;

	if (count == 0)
	{
		CFifoES::head = u;
		CFifoES::head->ref = NULL;
		CFifoES::tail->ref = NULL;
	}
	
	count++;

	LeaveCriticalSection(&m_CriticalSection);
}


void CFifoES::pop()
{
	EnterCriticalSection(&m_CriticalSection);

	if (count > 0)
	{
		unitES_t* todelete;
		unitES_t* newhead=NULL;
		todelete=CFifoES::head;
		if(CFifoES::head->ref != NULL)
		    newhead = CFifoES::head->ref;

		delete todelete;
		CFifoES::head=newhead;
        count--;
	}

	LeaveCriticalSection(&m_CriticalSection);
}

unitES_t* CFifoES::front()
{
    EnterCriticalSection(&m_CriticalSection);
	unitES_t* answer =CFifoES::head;
	LeaveCriticalSection(&m_CriticalSection);
	return answer;
}

int CFifoES::items()
{
	int answer;
	EnterCriticalSection(&m_CriticalSection);
	answer = count;
	LeaveCriticalSection(&m_CriticalSection);
    return answer;
}
