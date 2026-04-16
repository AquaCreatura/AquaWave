#pragma once

#include <vector>
#include <atomic>
#include <optional>
#include <cstddef>

template<typename T>
class AquaObjectQueue
{
public:
	AquaObjectQueue();

	void Resize(size_t capacity);
	void Reset();

	bool push(const T& value);
	bool push_swap(T& value);

	T pop();

	bool empty() const;
	bool IsReadAvailable() const;
	bool IsWriteAvailable() const;

	size_t capacity() const;
	size_t size();
private:
	size_t m_capacity;
	std::vector<T> m_buffer;

	std::atomic<size_t> m_head;
	std::atomic<size_t> m_tail;

private:
	size_t next(size_t index) const;
};


