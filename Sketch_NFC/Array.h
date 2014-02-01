/*
 * Array.h
 *
 *  Created on: 2013. 12. 21.
 *      Author: innocentevil
 */

#ifndef ARRAY_H_
#define ARRAY_H_

template<class T>
class Array {
public:
	Array(const int size) :
			length(size) {
		array = new T[size]();
		pos = 0;
	}
	virtual ~Array() {
		delete[] array;
	}
	T& operator[](int i) {
		return array[i];
	}

	int write(T* src, int len) {
		for (int i = 0; (i < len) && (pos < length); i++) {
			array[pos++] = src[i];
		}
		return pos;
	}

	int write(T& val) {
		array[pos++] = val;
		return pos;
	}

	int write(Array<T>& src) {
		return write(src.array, src.length);
	}

	int read(T* dest, int len) {
		for (int i = 0; (i < len) && (pos < length); i++) {
			dest[i] = array[pos++];
		}
		return pos;
	}

	int read(Array<T>& dest) {
		return read(dest.array,dest.length);
	}

	T& read() {
		return array[pos++];
	}

	T& seek(int idx) {
		pos = idx;
		return array[pos];
	}

	int position() {
		return pos;
	}

	bool isAvaliable(){
		return pos < length;
	}

	int length;
private:
	int pos;
	T* array;
};

#endif /* ARRAY_H_ */

