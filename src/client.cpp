#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include "include/defines.h"

class Client 
{
public:
	Client(const char* mem_name, const int mem_size, const char* semaphore_name) 
		: shm_size{mem_size}, shm_name{mem_name}, sem_name{semaphore_name}
	{
		// open shared memory
		shm_fd = shm_open(shm_name, O_RDWR, 0666);
		if(shm_fd == -1) {
			throw std::invalid_argument("ERROR: Could not open shared memory");
		}
		shm_ptr = (char*) mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

		// open named semaphore
		sem = sem_open(sem_name, 0);
		if(sem == SEM_FAILED) {
			throw std::invalid_argument("ERROR: Could not open named semaphore");
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
		write_int32_to_buff(static_cast<int32>(operation), &shm_ptr);
		write_int32_to_buff(message, &shm_ptr);
		sem_post(sem);
	}

private:
	int shm_size;
	int shm_fd;
	char* shm_ptr;
	const char* shm_name;
	const char* sem_name;
	sem_t* sem;

	void write_int32_to_buff(int32 value, char **buff)
	{
		bytes_to_int32_t conversion;
		conversion.integer = value;
		for(int i = 0; i < 4; i++) {
			(*buff)[i] = conversion.bytes[i];
		}
		// TODO: Add a current position in shm and wrap around
		(*buff) += 4;
	}
};

int main()
{
	Client client(SHM_NAME, SHM_SIZE, SEM_NAME);
	client.write_message(OPERATION::PRINT, 0);
	client.write_message(OPERATION::FIND, 3);
	client.write_message(OPERATION::INSERT, 3);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);
	client.write_message(OPERATION::FIND, 3);

	client.write_message(OPERATION::INSERT, -3);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, -1);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, 4);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, 7);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, 9);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, -10);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::INSERT, 411);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::FIND, 4);
	client.write_message(OPERATION::DELETE, 4);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::DELETE, -3);
	client.write_message(OPERATION::DELETE, -3);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);

	client.write_message(OPERATION::DELETE, -10);
	client.write_message(OPERATION::INSERT, 4);
	sleep(1);
	client.write_message(OPERATION::PRINT, 0);
	client.write_message(OPERATION::QUIT, 0);
}
