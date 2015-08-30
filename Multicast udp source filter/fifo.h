#ifndef H_FIFO
#define H_FIFO
#include "structs.h"
#include <Streams.h>

#define BUFFER_T_NUMBER 5 // Number of recv buffers

class CFifo	// Type I
{
	private:
		CRITICAL_SECTION m_CriticalSection;
		unit_t*  head;
        unit_t*	 tail;
		unit_t unit_array[BUFFER_T_NUMBER];
        int count;
		int first_free_index();
		bool dyn_buf;

    public:
		 CFifo(bool dynamic);
		~CFifo();
		int push(buffer_t *);
		void pop();
		int items();
		unit_t* front(int* count);
		bool used_indexes[BUFFER_T_NUMBER];
		int get_free_index();
		HANDLE m_data_rcvd_event;

};

class CFifoES // Type II
{
	private:
		CRITICAL_SECTION m_CriticalSection;
		unitES_t*  head;
        unitES_t*	 tail;
        int count;

    public:
		 CFifoES();
		~CFifoES();
		void push(bufferES_t *);
		void pop();
		int items();
		void empty();
		void lock(bool);
		unitES_t* front();

};








#endif