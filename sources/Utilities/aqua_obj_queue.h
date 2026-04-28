#pragma once

#include <vector>
#include <atomic>
#include <optional>
#include <cstddef>
namespace aqua_queue {

template<typename T>
class ObjQue
{
public:

	ObjQue()
		: m_capacity(0),
		m_head(0),
		m_tail(0)
	{
	}


	void Resize(size_t capacity)
	{
		m_capacity = capacity + 1;
		m_buffer.clear();
		m_buffer.resize(m_capacity);

		m_head.store(0, std::memory_order_relaxed);
		m_tail.store(0, std::memory_order_relaxed);
	}


	void Reset()
	{
		m_head.store(0, std::memory_order_relaxed);
		m_tail.store(0, std::memory_order_relaxed);
	}





	bool IsWriteAvailable() const
	{
		size_t tail = m_tail.load(std::memory_order_acquire);
		size_t next_tail = next(tail);

		return next_tail != m_head.load(std::memory_order_acquire);
	}


	bool IsReadAvailable() const
	{
		return m_head.load(std::memory_order_acquire) !=
			m_tail.load(std::memory_order_acquire);
	}


	bool push(const T& value)
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


	bool push_swap(T& value)
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


	T pop()
	{
		if (!IsReadAvailable())
		{
			return T();
		}

		size_t head = m_head.load(std::memory_order_relaxed);
		T value = m_buffer[head];

		m_head.store(next(head), std::memory_order_release);
		return value;
	}

	bool pop_swap(T& passed)
	{
		if (!IsReadAvailable())
		{
			return false;
		}

		size_t head = m_head.load(std::memory_order_relaxed);
		std::swap(passed, m_buffer[head]);
		m_head.store(next(head), std::memory_order_release);
		return true;
	}


	bool empty() const
	{
		return !IsReadAvailable();
	}


	size_t capacity() const
	{
		return (m_capacity == 0) ? 0 : (m_capacity - 1);
	}


	size_t size()
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
private:
	size_t m_capacity;
	std::vector<T> m_buffer;

	std::atomic<size_t> m_head;
	std::atomic<size_t> m_tail;

private:
	size_t next(size_t index) const
	{
		return (index + 1) % m_capacity;
	}
};


}