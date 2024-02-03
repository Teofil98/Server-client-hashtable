#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <stdexcept>
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
			const int htable_size, const unsigned long max_threads = std::thread::hardware_concurrency()) 
		: shm_size{mem_size}, shm_name{mem_name}, sem_name{semaphore_name},
		  hashtable_size{htable_size}, hashtable(htable_size, Server::simple_hash),
		  max_nb_threads{max_threads}, shm_index{0}
	{
		if(shm_size < MESSAGE_SIZE) {
			throw std::invalid_argument("ERROR: Shared memory size is smaller than message size");
		}
		// open shared memory
		shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
		if(shm_fd == -1) {
			throw std::runtime_error("ERROR: Could not create shared memory");
		}
	
		ftruncate(shm_fd, shm_size);	
		shm_ptr = (char*) mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);

		// create named semaphore
		sem = sem_open(sem_name, O_CREAT, 0666, 0);
		if(sem == SEM_FAILED) {
			throw std::runtime_error("ERROR: Could not create named semaphore");
		}

		threads.reserve(max_nb_threads);

		srand(1234);
	}

	~Server() 
	{
		sem_close(sem);
		munmap((void*)shm_ptr, shm_size);
		shm_unlink(shm_name);
	}

	void run() 
	{
		std::cout << "Server waiting for client input." << std::endl;
		int sem_val;
		bool quit = false; 
		bool do_print = false;
		while(!quit) {
			do_print = false;
			sem_getvalue(sem, &sem_val); 
			// Read messages and spawn threads
			while(sem_val > 0 && threads.size() < max_nb_threads) {
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
				spawn_thread(operation, value);
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
	const unsigned long max_nb_threads;
	std::vector<std::thread> threads;
	int shm_index;
	std::mutex thread_mtx;
	std::condition_variable cv;
	int thread_ready = false;

	// Very simple hash function, not optimized to minimize collisions
	static uint simple_hash(int32 value) {
		return value > 0 ? value : -value;
	}

	std::pair<OPERATION, int32> read_message() {
		// check if we are at the end of the shared memory,
		// if so, wrap around
		if((shm_index + MESSAGE_SIZE) > shm_size)
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

	void execute_insert(int32 value) 
	{
		std::cout << "[Thread " << std::this_thread::get_id() << "] started" << std::endl;

		{
		std::unique_lock<std::mutex> lck(thread_mtx);	
		hashtable.lock_bucket_for_write(value);
		thread_ready = true;
		cv.notify_all();
		}
		
		std::cout << "[Thread " << std::this_thread::get_id() << "] inserting " << value << std::endl;
		rand_delay();
		hashtable.insert(value);
		hashtable.unlock_bucket_for_write(value);
		std::cout << "[Thread " << std::this_thread::get_id() << "] finished" << std::endl;
	}

	void execute_delete(int32 value)
	{
		std::cout << "[Thread " << std::this_thread::get_id() << "] started" << std::endl;
		{
		std::unique_lock<std::mutex> lck(thread_mtx);	
		hashtable.lock_bucket_for_write(value);
		thread_ready = true;
		cv.notify_all();
		}

		std::cout << "[Thread " << std::this_thread::get_id() << "] removing " << value << std::endl;
		rand_delay();
		hashtable.remove(value);
		hashtable.unlock_bucket_for_write(value);
		std::cout << "[Thread " << std::this_thread::get_id() << "] finished" << std::endl;
	}

	void execute_find(int32 value)
	{
		std::cout << "[Thread " << std::this_thread::get_id() << "] started" << std::endl;
		{
		std::unique_lock<std::mutex> lck(thread_mtx);	
		hashtable.lock_bucket_for_read(value);
		thread_ready = true;
		cv.notify_all();
		}

		rand_delay();
		bool result = hashtable.find(value);
		hashtable.unlock_bucket_for_read(value);
		if(result) std::cout << "[Thread " << std::this_thread::get_id() << "] Element " << value << " found" << std::endl; 
		else  std::cout << "[Thread " << std::this_thread::get_id() << "] Element " << value << " not found" << std::endl;
		std::cout << "[Thread " << std::this_thread::get_id() << "] finished" << std::endl;

	}

	void rand_delay() 
	{
		int n = 0;
		int limit = rand() % 1000000;
		while(n++ < limit) {}

	}

	void spawn_thread(OPERATION operation, int32 value) 
	{
			switch (operation) {
			case OPERATION::INSERT: {
				threads.emplace_back(&Server::execute_insert, this, value);
			} break;
			case OPERATION::DELETE: {
				threads.emplace_back(&Server::execute_delete, this, value);
			} break;
			case OPERATION::FIND: {
				threads.emplace_back(&Server::execute_find, this, value);
			} break;
				default: break; // should not be reached
			}	
			// wait for thread to grab read/write lock before continuing
			// this ensures that the order of enqueued operations from the client
			// is maintained
			{
				std::unique_lock<std::mutex> lck(thread_mtx);
				while(!thread_ready) {
					cv.wait(lck);
				}
				thread_ready = false;
			}
	}
};

int main(int argc, char* argv[])
{
	if(argc != 2) {
		std::cout << "Please specify hashtable size as command line argument." << std::endl;
		return 0;
	}

	int hash_size = atoi(argv[1]);
	if(hash_size <= 0) {
		std::cout << "Hashtable size not valid." << std::endl;
		return 0;
	}

	try {
	Server server(SHM_NAME, SHM_SIZE, SEM_NAME, hash_size, MAX_NUM_THREADS);
	server.run();
	} catch (std::exception& e) {
		std::cout << "Exception encountered: " << e.what() << std::endl;
	}
	return 0;
}
