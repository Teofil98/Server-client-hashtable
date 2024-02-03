#include <cstdlib>
#include <exception>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <thread>
#include <utility>
#include <unistd.h>
#include <vector>
#include "include/defines.h"
#include "include/hashtable.h"

class Server
{
public:
	Server(const char* mem_name, const int mem_size, const char* semaphore_name, 
			const int htable_size, const int max_threads = std::thread::hardware_concurrency()) 
		: shm_size{mem_size}, shm_name{mem_name}, sem_name{semaphore_name},
		  hashtable_size{htable_size}, hashtable(htable_size, Server::simple_hash),
		  max_nb_threads{max_threads}, shm_index{0}
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

		threads.reserve(max_nb_threads);

		// TODO: Probably don't want to keep rand here in the end
		srand(1234);

		std::cout << "Max hw threads: " << std::thread::hardware_concurrency() << std::endl;
	}

	~Server() 
	{
		sem_close(sem);
		if(sem_unlink(sem_name) != 0) {
			std::cout << "Semaphore not unlinked" << std::endl;
		} else { std:: cout << "Semaphore successfully unlinked" << std::endl;}

		// TODO: Use buffer_idx for accessing shm_ptr
		munmap((void*)shm_ptr, shm_size);
		shm_unlink(shm_name);
	}

	void run() 
	{
		int sem_val;
		bool quit = false; 
		bool do_print = false;
		while(!quit) {
			do_print = false;
			sem_getvalue(sem, &sem_val); 
			// Read messages and spawn threads
			while(sem_val > 0 && threads.size() < max_nb_threads) {
				std::cout << "Sem val: " << sem_val << std::endl;
				auto [operation, value] = read_message();	
				// let client know that a message has been read
				// important for sitations where the buffer can hold a single message
				// to let the client know when it is safe to read the next message
				sem_wait(sem);
				sem_getvalue(sem, &sem_val); 
				if(operation == OPERATION::QUIT) {
					quit = true;
					break;
				}

				// PRINT serializes operations
				if(operation == OPERATION::PRINT) {
					do_print = true;
					break;
				}

				// If operation is not QUIT or PRINT, spawn a new thread
				threads.emplace_back(&Server::execute_operation, this, operation, value);
			}

			// Wait for threads to finish executing
			for(std::thread& t : threads) {
				t.join();
			}
			threads.clear();
			if(do_print) {
				hashtable.print();
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
	const int max_nb_threads;
	std::vector<std::thread> threads;
	int shm_index;

	// Very simple hash function, not optimized to minimize collisions
	static uint simple_hash(int32 value) {
		return value > 0 ? value : -value;
	}

	std::pair<OPERATION, int32> read_message() {
		// check if we are at the end of the shared memory,
		// if so, wrap around
		if((shm_index + 8) > shm_size)
			shm_index = 0;
		// read the operation
		int32 operation = read_int32_from_buff();
		int32 value = read_int32_from_buff();
		return std::make_pair(static_cast<OPERATION>(operation), value);
	}

	int32 read_int32_from_buff()
	{
		bytes_to_int32_t value;
		for(int i = 0; i < 4; i++) {
			value.bytes[i] = shm_ptr[shm_index + i];
		}
		shm_index += 4;
		return value.integer;
	}

	void execute_operation(OPERATION operation, int32 value) 
	{
		std::cout << "[Thread " << std::this_thread::get_id() << "] started" << std::endl;
		int n = 0;
		int limit = rand();
		while(n++ < limit) {}
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
					default: break; // should not be reached
			}	
		std::cout << "[Thread " << std::this_thread::get_id() << "] finished" << std::endl;
	}
};

int main()
{
	try {
	Server server(SHM_NAME, SHM_SIZE, SEM_NAME, 7, 8);
	server.run();
	} catch (std::exception& e) {
		std::cout << "Exception encountered: " << e.what() << std::endl;
	}
}
