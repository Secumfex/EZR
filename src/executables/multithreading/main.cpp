/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>
 
#include <thread>  

////////////////////// PARAMETERS /////////////////////////////
const int NUM_THREADS = 10;

void* call_from_thread(int tid) 
{
	std::cout << "Launched from thread" << tid <<std::endl;
	return NULL;
}
int main()
{
	//auto window = generateWindow();

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////// multithreading /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	std::thread t[NUM_THREADS];

	for (int i = 0; i < NUM_THREADS; ++i)
	{
		t[i]=std::thread(call_from_thread, i);
	}

	std::cout << "launched from main" << std::endl;

	// Join with main thread
	for (int i = 0; i < NUM_THREADS; i++)
	{
		t[i].join();
	}

	/////////////////////////////////////////////////////////////////////////////////

	//while (!shouldClose(window))
	//{

	//}

	return 0;
}