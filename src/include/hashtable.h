#pragma once

#include <functional>
#include "linked_list.h"

template <typename T>
class HashTable
{
public:
	HashTable(const uint size, std::function<uint(T)> hash_function) 
		: m_size{size}, m_hash_function{hash_function}
	{
		m_buckets = new LinkedList<T>[m_size]; 
	};
	~HashTable()
	{
		delete[] m_buckets;
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
private:
	const uint m_size;
	LinkedList<T>* m_buckets;
	std::function<int(T)> m_hash_function;
};
