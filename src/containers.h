#include <stdlib.h>
#include "gb_emu.h"


template <typename T>
struct DynamicArray {
	static constexpr uint32 initialAllocSize = 32;
	T * data = nullptr;
	uint32 capacity = 0;
	uint32 count = 0;

	DynamicArray() = default;
	DynamicArray( const DynamicArray<T> &src ) {
		*this = src;
	}

	DynamicArray<T>& operator=(const DynamicArray<T> & rhs) {
		if ( rhs.capacity ) {
			Resize( rhs.capacity );
			for ( uint32 i = 0; i < rhs.capacity; i++ ) {
				data[i] = rhs.data[i];
			}
			count = rhs.count;
		} else {
			capacity = 0;
			count = 0;
			delete [] data;
			data = nullptr;
		}
		return *this;
	}

	~DynamicArray() {
		delete [] data;
	}

	bool Resize( uint32 newCapacity ) {
		if ( newCapacity == 0 ) {
			newCapacity = initialAllocSize;
		}
		T * temp = new T[ newCapacity ];

		for ( uint32 i = 0; i < MIN(capacity, newCapacity); i++ ) {
			temp[i] = data[i];
		}

		delete [] data;
		data = temp;

		capacity = newCapacity;
		if ( count > newCapacity ) {
			count = newCapacity;
		}
		return true;
	}

	void Clear() {
		delete [] data;
		data = nullptr;
		capacity = 0;
		count = 0;
	}

	T & AllocateOne() {
		if ( count == capacity ) {
			Resize( capacity * 2 );
		}
		return *(data + count++);
	}

	T & PushBack( const T & newElem ) {
		if ( count == capacity ) {
			Resize( capacity * 2 );
		}
		data[ count ] = newElem;
		return *(data + count++);
	}

	T & At( uint32 index ) {
		return *(data + index);
	}

	T & Last() {
		return *(data + count - 1);
	}

	T * begin() { return data == nullptr ? nullptr : &data[0]; }
	T * end() { return data == nullptr ? nullptr : &data[count - 1]; }
	
	const T * begin() const { return data == nullptr ? nullptr : &data[0]; }
	const T * end() const { return data == nullptr ? nullptr : &data[count - 1]; }
};
