#pragma once

#include <iostream>
#include <shared_mutex>

template <typename T>
struct Node
{
	T value;
	Node* next;
};

template <typename T>
class LinkedList 
{
public:
	LinkedList() 
	{
		// initialize empty list
		head = new Node<T>;
		head->next = nullptr;
		end = head;
	}
	~LinkedList() 
	{ 
		while(head != end) {
			remove((head->next)->value);
		}

		delete head;
	}

	void add (T value) 
	{
		rw_lock.lock();
		Node<T>* node = new Node<T>;
		node->value = value;
		node->next = nullptr;

		// add after end
		end->next = node;
		end = node;
		rw_lock.unlock();
	}
	
	void remove(T value)
	{
		rw_lock.lock();
		Node<T>* prev = head;
		
		while(prev->next != nullptr) {
			Node<T>* current = prev->next;
			if(current->value == value) {
				// remove node
				prev->next = current->next;
				if(current == end) {
					end = prev;
				}
				delete current;
				rw_lock.unlock();
				return;
			}
			prev = current;
		}
		rw_lock.unlock();
	}

	bool find(T value)
	{
		rw_lock.lock_shared();
		Node<T>* prev = head;
		while(prev->next != nullptr) {
			Node<T>* current = prev->next;
			if(current->value == value) {
				rw_lock.unlock_shared();
				return true;
			}
			prev = current;
		}
		rw_lock.unlock_shared();
		return false;
	}

	void print()
	{
		rw_lock.lock_shared();
		Node<T>* prev = head;
		while(prev->next != nullptr) {
			Node<T>* current = prev->next;
			std::cout << current->value << " -> ";
			prev = current;
		}
		std::cout << std::endl;
		rw_lock.unlock_shared();
	}

private:
	Node<T>* head;
	Node<T>* end;
	std::shared_mutex rw_lock;
};
