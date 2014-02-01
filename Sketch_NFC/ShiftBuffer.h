/*
 * ShiftBuffer.h
 *
 *  Created on: 2013. 12. 14.
 *      Author: innocentevil
 */

#ifndef SHIFTBUFFER_H_
#define SHIFTBUFFER_H_

#include <inttypes.h>
namespace Util {

template<class T>
class ShiftBuffer {
public:
	ShiftBuffer(uint32_t size) {
		this->size = size;
		array = new T[size];
		for (uint32_t i = 0; i < size; i++) {
			array[i] = 0;
		}

	}
	virtual ~ShiftBuffer() {
                delete[] array;
	}
	T shift(T item) {
		T val = array[0];
		for (uint32_t i = 0; i + 1 < this->size; i++) {
			array[i] = array[i + 1];
		}
		array[size - 1] = item;
		return val;
	}

	bool match(const T* pattern, uint32_t offset, uint32_t len) {
		for (uint32_t i = 0; i < len; i++) {
			if (array[i + offset] != pattern[i])
				return false;
		}
		return true;
	}
	void copy(T* tg, uint8_t offset, uint8_t len) {
		for (uint8_t i = 0; i < len; i++) {
			tg[i] = array[offset + i];
		}
	}

	uint8_t find(const T* pattern, uint8_t len) {
		for (uint8_t i = 0; i < size - len; i++) {
			if (pattern[0] == array[i]) {
				if (match(pattern, i, len)) {
					return i;
				}
			}
		}
		return -1;
	}

	T* getBuffer(){
		return array;
	}

private:
	T* array;
	uint32_t size;
};

}

#endif /* SHIFTBUFFER_H_ */

