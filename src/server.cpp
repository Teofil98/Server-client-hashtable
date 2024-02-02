#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <utility>
#include <unistd.h>
#include "include/defines.h"
#include "include/hashtable.h"

class Server
{
public:
	Server(const char* mem_name, const int mem_size, const char* semaphore_name, const int htable_size) 
		: shm_size{mem_size}, shm_name{mem_name}, sem_name{semaphore_name},
		  hashtable_size{htable_size}, hashtable(htable_size, Server::simple_hash)
	{
		// open shared memory
		shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
		if(shm_fd == -1) {
			// TODO: Find better exception type to throw
			throw std::invalid_argument("ERROR: Could not create shared memory");
		}
	
		ftruncate(shm_fd, shm_size);	
		shm_ptr = (char*) mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);

		// create named semaphore
		sem = sem_open(sem_name, O_CREAT, 0666, 0);
		if(sem == SEM_FAILED) {
			throw std::invalid_argument("ERROR: Could not create named semaphore");
		}
	}

	~Server() 
	{
		sem_close(sem);

		munmap((void*)shm_ptr, shm_size);
		shm_unlink(shm_name);
	}

	void run() 
	{
		while(true) {
			sem_wait(sem);
			auto [operation, value] = read_message();	

			if(operation == OPERATION::QUIT)
				break;
			
			switch (operation) {
				case OPERATION::INSERT: {
					hashtable.insert(value);
				} break;
				case OPERATION::DELETE: {
					hashtable.remove(value);
				} break;
				case OPERATION::FIND: {
					bool result = hashtable.find(value);
					if(result) std::cout << "Element: " << value << " found" << std::endl; 
					else std::cout << "Element: " << value << " not found" << std::endl; 
			} break;
				case OPERATION::PRINT: {
					hashtable.print();
				} break;
				default: break; // should not be reached
			}
		}
	}

private:
	int shm_size;
	int shm_fd;
	char* shm_ptr;
	const char* shm_name;
	const char* sem_name;
	sem_t* sem;
	const int hashtable_size;
	HashTable<int32> hashtable;

	// Very simple hash function, not optimized to minimize collisions
	static uint simple_hash(int32 value) {
		return value > 0 ? value : -value;
	}

	std::pair<OPERATION, int32> read_message() {
		// read the operation
		int32 operation = read_int32_from_buff(&shm_ptr);
		int32 value = read_int32_from_buff(&shm_ptr);
		// TODO: Compile with c++20
		return std::make_pair(static_cast<OPERATION>(operation), value);
	}

	int32 read_int32_from_buff(char** buff)
	{
		bytes_to_int32_t value;
		for(int i = 0; i < 4; i++) {
			value.bytes[i] = (*buff)[i];
		}
		(*buff) += 4;
		return value.integer;
	}
};

int main()
{
	Server server(SHM_NAME, SHM_SIZE, SEM_NAME, 7);
	server.run();
}
