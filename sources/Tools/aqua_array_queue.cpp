#include "aqua_array_queue.h"
#include <ipps.h>
#include <algorithm>

template<typename T>
AquaArrayQueue<T>::AquaArrayQueue()
	: m_capacity(0), m_head(0), m_tail(0)
{
}

template<typename T>
void AquaArrayQueue<T>::Resize(size_t capacity)
{
	// +1 позволяет отличить full от empty без доп. флага
	m_capacity = capacity + 1;
	m_buffer.resize(m_capacity);
	Reset();
}

template<typename T>
void AquaArrayQueue<T>::Reset()
{
	// relaxed достаточно — нет межпоточной зависимости при reset
	m_head.store(0, std::memory_order_relaxed);
	m_tail.store(0, std::memory_order_relaxed);
}

template<typename T>
size_t AquaArrayQueue<T>::write(const T* data, size_t count)
{
	size_t head = m_head.load(std::memory_order_relaxed);
	size_t tail = m_tail.load(std::memory_order_acquire);
	// acquire нужен чтобы видеть данные, прочитанные consumer-ом

	size_t free = WriteAvailableSizeInternal(head, tail);
	size_t to_write = std::min(count, free);

	if (to_write == 0)
		return 0;

	// разбиваем на 2 копирования чтобы избежать ветки wrap-around внутри цикла
	size_t first_part = std::min(to_write, m_capacity - head);

	ippsCopy_8u(
		reinterpret_cast<const Ipp8u*>(data),
		reinterpret_cast<Ipp8u*>(&m_buffer[head]),
		first_part * sizeof(T) // ipps работает в байтах
	);

	size_t second_part = to_write - first_part;

	if (second_part > 0)
	{
		ippsCopy_8u(
			reinterpret_cast<const Ipp8u*>(data + first_part),
			reinterpret_cast<Ipp8u*>(&m_buffer[0]),
			second_part * sizeof(T)
		);
	}

	// release гарантирует, что данные записаны до обновления head
	m_head.store(next(head, to_write), std::memory_order_release);
	return to_write;
}

template<typename T>
size_t AquaArrayQueue<T>::Read(T* dst, size_t count)
{
	size_t tail = m_tail.load(std::memory_order_relaxed);
	size_t head = m_head.load(std::memory_order_acquire);
	// acquire нужен чтобы видеть данные, записанные producer-ом

	size_t available = ReadAvailableSizeInternal(head, tail);
	size_t to_read = std::min(count, available);

	if (to_read == 0)
		return 0;

	// аналогично write — избегаем условной логики внутри копирования
	size_t first_part = std::min(to_read, m_capacity - tail);

	ippsCopy_8u(
		reinterpret_cast<const Ipp8u*>(&m_buffer[tail]),
		reinterpret_cast<Ipp8u*>(dst),
		first_part * sizeof(T)
	);

	size_t second_part = to_read - first_part;

	if (second_part > 0)
	{
		ippsCopy_8u(
			reinterpret_cast<const Ipp8u*>(&m_buffer[0]),
			reinterpret_cast<Ipp8u*>(dst + first_part),
			second_part * sizeof(T)
		);
	}

	// release — consumer "освобождает" данные после чтения
	m_tail.store(next(tail, to_read), std::memory_order_release);
	return to_read;
}

template<typename T>
bool AquaArrayQueue<T>::empty() const
{
	return m_head.load(std::memory_order_acquire) ==
		m_tail.load(std::memory_order_acquire);
}

template<typename T>
size_t AquaArrayQueue<T>::ReadAvailableSize() const
{
	size_t head = m_head.load(std::memory_order_acquire);
	size_t tail = m_tail.load(std::memory_order_acquire);
	return ReadAvailableSizeInternal(head, tail);
}

template<typename T>
size_t AquaArrayQueue<T>::WriteAvailableSize() const
{
	size_t head = m_head.load(std::memory_order_acquire);
	size_t tail = m_tail.load(std::memory_order_acquire);
	return WriteAvailableSizeInternal(head, tail);
}

template<typename T>
size_t AquaArrayQueue<T>::capacity() const
{
	return m_capacity - 1;
}

template<typename T>
size_t AquaArrayQueue<T>::next(size_t index, size_t step) const
{
	// модуль даёт цикличность без ветвлений
	return (index + step) % m_capacity;
}

template<typename T>
size_t AquaArrayQueue<T>::ReadAvailableSizeInternal(size_t head, size_t tail) const
{
	// разница зависит от того, "перешли ли через ноль"
	if (head >= tail)
		return head - tail;

	return m_capacity - (tail - head);
}

template<typename T>
size_t AquaArrayQueue<T>::WriteAvailableSizeInternal(size_t head, size_t tail) const
{
	// -1 чтобы не допустить состояния head == tail при full
	return m_capacity - 1 - ReadAvailableSizeInternal(head, tail);
}