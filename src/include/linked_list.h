#include <iostream>

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
		// TODO: Free list elements
		
		while(head != end) {
			remove((head->next)->value);
		}

		delete head;
	}

	void add (T value) 
	{
		Node<T>* node = new Node<T>;
		node->value = value;
		node->next = nullptr;

		// add after end
		end->next = node;
		end = node;
	}
	
	void remove(T value)
	{
		if(head == end) { 
			return;
		}

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
				return;
			}
			prev = current;
		}
	}

	bool find(T value)
	{
		Node<T>* prev = head;
		while(prev->next != nullptr) {
			Node<T>* current = prev->next;
			if(current->value == value) {
				return true;
			}
			prev = current;
		}
		return false;
	}

	void print()
	{
		Node<T>* prev = head;
		while(prev->next != nullptr) {
			Node<T>* current = prev->next;
			std::cout << current->value << " -> ";
			prev = current;
		}
		std::cout << std::endl;
	}

private:
	Node<T>* head;
	Node<T>* end;
};
