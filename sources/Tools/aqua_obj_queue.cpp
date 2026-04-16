#include "aqua_obj_queue.h"

#include <utility>

template<typename T>
AquaObjectQueue<T>::AquaObjectQueue()
	: m_capacity(0),
	m_head(0),
	m_tail(0)
{
}

template<typename T>
void AquaObjectQueue<T>::Resize(size_t capacity)
{
	m_capacity = capacity + 1;
	m_buffer.clear();
	m_buffer.resize(m_capacity);

	m_head.store(0, std::memory_order_relaxed);
	m_tail.store(0, std::memory_order_relaxed);
}

template<typename T>
void AquaObjectQueue<T>::Reset()
{
	m_head.store(0, std::memory_order_relaxed);
	m_tail.store(0, std::memory_order_relaxed);
}

template<typename T>
size_t AquaObjectQueue<T>::next(size_t index) const
{
	return (index + 1) % m_capacity;
}

template<typename T>
bool AquaObjectQueue<T>::IsWriteAvailable() const
{
	size_t tail = m_tail.load(std::memory_order_acquire);
	size_t next_tail = next(tail);

	return next_tail != m_head.load(std::memory_order_acquire);
}

template<typename T>
bool AquaObjectQueue<T>::IsReadAvailable() const
{
	return m_head.load(std::memory_order_acquire) !=
		m_tail.load(std::memory_order_acquire);
}

template<typename T>
bool AquaObjectQueue<T>::push(const T& value)
{
	if (!IsWriteAvailable())
	{
		return false;
	}

	size_t tail = m_tail.load(std::memory_order_relaxed);
	m_buffer[tail] = value;

	m_tail.store(next(tail), std::memory_order_release);
	return true;
}

template<typename T>
bool AquaObjectQueue<T>::push_swap(T& value)
{
	if (!IsWriteAvailable())
	{
		return false;
	}
	
	size_t tail = m_tail.load(std::memory_order_relaxed);
	std::swap(m_buffer[tail], value);

	m_tail.store(next(tail), std::memory_order_release);
	return true;
}

template<typename T>
T AquaObjectQueue<T>::pop()
{
	if (!IsReadAvailable())
	{
		return std::nullopt;
	}

	size_t head = m_head.load(std::memory_order_relaxed);
	T value = m_buffer[head];

	m_head.store(next(head), std::memory_order_release);
	return value;
}

template<typename T>
bool AquaObjectQueue<T>::empty() const
{
	return !IsReadAvailable();
}

template<typename T>
size_t AquaObjectQueue<T>::capacity() const
{
	return (m_capacity == 0) ? 0 : (m_capacity - 1);
}

template<typename T>
size_t AquaObjectQueue<T>::size()
{
	size_t head = m_head.load(std::memory_order_acquire);
	size_t tail = m_tail.load(std::memory_order_acquire);

	if (tail >= head)
	{
		return tail - head;
	}
	else
	{
		return m_capacity - (head - tail);
	}

}

// при необходимости:
// template class AquaObjectQueue<int>;