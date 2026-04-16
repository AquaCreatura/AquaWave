#pragma once

#include <vector>
#include <atomic>
#include <cstddef>

template<typename T>
class AquaArrayQueue
{
public:
	AquaArrayQueue();

	void Resize(size_t capacity);
	void Reset();

	size_t write(const T* data, size_t count);
	size_t Read(T* dst, size_t count);

	bool empty() const;

	size_t ReadAvailableSize() const;
	size_t WriteAvailableSize() const;

	size_t capacity() const;

private:
	size_t m_capacity;
	std::vector<T> m_buffer;

	std::atomic<size_t> m_head;
	std::atomic<size_t> m_tail;

private:
	size_t next(size_t index, size_t step = 1) const;

	size_t ReadAvailableSizeInternal(size_t head, size_t tail) const;
	size_t WriteAvailableSizeInternal(size_t head, size_t tail) const;
};
