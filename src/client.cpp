#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include "include/defines.h"

class Client 
{
public:
	Client(const char* mem_name, const int mem_size, const char* semaphore_name) 
		: shm_size{mem_size}, shm_name{mem_name}, sem_name{semaphore_name}, shm_index{0}
	{
		if(shm_size < MESSAGE_SIZE) {
			throw std::invalid_argument("ERROR: Shared memory size is smaller than message size");
		}

		// open shared memory
		shm_fd = shm_open(shm_name, O_RDWR, 0666);
		if(shm_fd == -1) {
			throw std::runtime_error("ERROR: Could not open shared memory");
		}
		shm_ptr = (char*) mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

		// open named semaphore
		sem = sem_open(sem_name, 0);
		if(sem == SEM_FAILED) {
			throw std::runtime_error("ERROR: Could not open named semaphore");
		}

	}

	~Client() 
	{
		sem_close(sem);

		munmap((void*)shm_ptr, shm_size);
		shm_unlink(shm_name);
	}

	void write_message(OPERATION operation, int32 message) 
	{
		// if we are at the end of the memory, wait untill server has finished 
		// processing all in-flight requests, then start writing from the beginning of the memory
		if((shm_index + MESSAGE_SIZE) > shm_size) {
			int sem_value;
			sem_getvalue(sem, &sem_value);
			while(sem_value != 0) {
				sem_getvalue(sem, &sem_value);
			}
			shm_index = 0;
		}
		write_int32_to_buff(static_cast<int32>(operation)); 
		write_int32_to_buff(message);
		sem_post(sem);
	}

private:
	int shm_size;
	int shm_fd;
	char* shm_ptr;
	int shm_index;
	const char* shm_name;
	const char* sem_name;
	sem_t* sem;

	void write_int32_to_buff(int32 value)
	{
		bytes_to_int32_t conversion;
		conversion.integer = value;
		for(int i = 0; i < 4; i++) {
			shm_ptr[shm_index + i] = conversion.bytes[i];
		}
		shm_index += 4;
	}
};

int main()
{
	try {
	Client client(SHM_NAME, SHM_SIZE, SEM_NAME);
	client.write_message(OPERATION::PRINT, 0);
	client.write_message(OPERATION::FIND, 3);
	client.write_message(OPERATION::INSERT, 3);
	client.write_message(OPERATION::FIND, 3);

	client.write_message(OPERATION::INSERT, -3);
	client.write_message(OPERATION::INSERT, -1);
	client.write_message(OPERATION::INSERT, 4);
	client.write_message(OPERATION::INSERT, 16);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, 7);
	client.write_message(OPERATION::INSERT, 9);
	client.write_message(OPERATION::INSERT, -10);
	client.write_message(OPERATION::INSERT, 411);
	client.write_message(OPERATION::FIND, 4);
	client.write_message(OPERATION::DELETE, 4);

	client.write_message(OPERATION::DELETE, -3);
	client.write_message(OPERATION::DELETE, -3);
	client.write_message(OPERATION::DELETE, -10);
	client.write_message(OPERATION::INSERT, 4);
	
	client.write_message(OPERATION::PRINT, 0);
	client.write_message(OPERATION::QUIT, 0);
	} catch (std::exception& e) {
		std::cout << "Exception encountered: " << e.what() << std::endl;
	}
}
