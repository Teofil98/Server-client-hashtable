#pragma once

#include <functional>
#include <shared_mutex>
#include <vector>
#include "linked_list.h"

template <typename T>
class HashTable
{
public:

	HashTable(const uint size, std::function<uint(T)> hash_function) 
		: m_size{size}, m_hash_function{hash_function}
	{
		m_buckets = new LinkedList<T>[m_size]; 
		m_bucket_locks = new std::shared_mutex[m_size];
	};

	~HashTable()
	{
		delete[] m_buckets;
		delete[] m_bucket_locks;
	}

	void insert(T value)
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_buckets[index].add(value);
	}

	void remove(T value)
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_buckets[index].remove(value);
	}

	bool find(T value)
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		return m_buckets[index].find(value);
	}

	void print()
	{
		for(uint i = 0; i < m_size; i++) {
			std::cout << "[" << i << "]: ";
			m_buckets[i].print();
		}
		std::cout << std::endl;
	}

	void lock_bucket_for_write(T value) 
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_bucket_locks[index].lock();
	}

	void unlock_bucket_for_write(T value) 
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_bucket_locks[index].unlock();
	}

	void lock_bucket_for_read(T value) 
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_bucket_locks[index].lock_shared();
	}

	void unlock_bucket_for_read(T value) 
	{
		uint hash = m_hash_function(value);
		uint index = hash % m_size;
		m_bucket_locks[index].unlock_shared();
	}

private:
	const uint m_size;
	LinkedList<T>* m_buckets;
	std::shared_mutex* m_bucket_locks;
	std::function<int(T)> m_hash_function;
};
