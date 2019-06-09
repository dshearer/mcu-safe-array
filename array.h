#ifndef __MCU_SAFE_ARRAY_H__
#define __MCU_SAFE_ARRAY_H__

#include <stddef.h>
#include <string.h>

#define SLICE_METH_ASSERTS() \
    do { \
        static_assert(Start <= L, "Bad start index"); \
        static_assert(End <= L, "Bad end index"); \
        static_assert(End >= Start, "Bad end index"); \
    } while (false);

#define DATA_METH_ASSERTS() \
    do { \
        static_assert(Offset < L, "Bad offset"); \
    } while (false);

namespace safearray {

/**
 * \brief A pointer to an array with a size.
 * 
 * The size is known only at runtime.  An instance is just a way to pass an
 * array and its size in one object.
 */
template<typename T>
class CArrayPtr
{
public:
    CArrayPtr(const T *data, size_t size): _data(data), _size(size) {}

    size_t size() const {
        return this->_size;
    }

    const T *data() const {
        return this->_data;
    }

private:
    const T *_data;
    size_t _size;
};

/** 
 * \brief A const pointer to a section of an array.
 * 
 * The slice itself has a fixed length known at compile-time, which is
 * hopefully less than or equal to the size of the array.
 * 
 * NOTE: Since a slice is a pointer, it's possible to give it a value
 * of NULL, which is what the default constructor does.
 */
template<typename T, size_t L>
class CSlice
{
public:
    /**
     * Make a slice pointing to nothing (NULL).
     * 
     * WARNING: Most of the other methods will have undefined behavior 
     * (and thus memory safety is not guaranteed) until this object is
     * given a valid data pointer.
     */
    CSlice() : _data(NULL) {};

    /**
     * Make a slice pointing to L instances of T at the given location.
     * 
     * WARNING: This class provides memory-safety only if the given pointer
     * points to L contiguous instances of T.
     */
    explicit CSlice(const T *data) : _data(data) {}

    /**
     * \brief Get a pointer to the slice's data.
     * 
     * \tparam Offset An offset (in number of instances of T) to apply to the
     * pointer before returning it.  The value is statically checked to ensure
     * memory-safety.  Default: 0.
     * 
     * \return P + Offset, where P is a pointer to the beginning of the slice's
     * data.
     */
    template<size_t Offset = 0>
    const T *cdata() const {
        DATA_METH_ASSERTS();
        return this->_data + Offset;
    }

    /**
     * WARNING: This method does no static or runtime bounds-checking.
     * 
     * \param i An index.  If i >= L, the return value is undefined.
     * 
     * \return A reference to the element at index i.
     */
    const T& operator[](size_t i) const {
        return this->_data[i];
    }

    /**
     * \return A CArrayPtr to the slice's data.
     */
    CArrayPtr<T> operator&() const {
        return CArrayPtr<T>(this->_data, L);
    }

    /**
     * Make a smaller slice.
     * 
     * The bounds given as template params are statically checked to ensure
     * memory-safety.
     * 
     * \tparam Start The index of the first element in the slice.  Default: 0.
     * \tparam End The next index after the index of the last element in the
     * slice.  Default: L.
     * \return A slice pointing from the element at index Start (inclusive) to
     * the element at index End (exclusive).
     */
    template<size_t Start = 0, size_t End = L>
    CSlice<T, End - Start> cslice() const {
        SLICE_METH_ASSERTS();
        return CSlice<T, End - Start>(this->cdata<Start>());
    }

    /**
     * \return The number of instances of T in the slice.
     */
    constexpr static size_t size() {
        return L;
    }

    /**
     * \return The number of bytes used by the slice's data.
     */
    constexpr static size_t sizeBytes() {
        return L * sizeof(T);
    }

protected:
    const T *_data;
};

/** 
 * \brief A pointer to a section of an array.
 * 
 * The slice itself has a fixed length known at compile-time, which is
 * hopefully less than or equal to the size of the array.
 * 
 * NOTE: Since a slice is a pointer, it's possible to give it a value
 * of NULL, which is what the default constructor does.
 */
template<typename T, size_t L>
class Slice : public CSlice<T, L>
{
public:
    /**
     * Make a slice pointing to nothing (NULL).
     * 
     * WARNING: Most of the other methods will have undefined behavior 
     * (and thus memory safety is not guaranteed) until this object is
     * given a valid data pointer.
     */
    Slice() : CSlice<T, L>() {};

    /**
     * Make a slice pointing to L instances of T at the given location.
     * 
     * WARNING: This class provides memory-safety only if the given pointer
     * points to L contiguous instances of T.
     */
    explicit Slice(T *data) : CSlice<T, L>(data) {}

    /**
     * Get a pointer to the slice's data.
     * 
     * @tparam Offset An offset (in number of instances of T) to apply to the
     * pointer before returning it.  The value is statically checked to ensure
     * memory-safety.  Default: 0.
     */
    template<size_t Offset = 0>
    T *data() {
        return (T *) this->template cdata<Offset>();
    }

    /**
     * Fill the slice with the given value.
     */
    void fill(T val) {
        for (size_t i = 0; i < L; ++i) {
            (*this)[i] = val;
        }
    }

    /**
     * Copy data from a slice into this one.
     * 
     * \param data A slice from which to copy data.  Its size is statically
     * checked to ensure memory-safety.
     */
    template<size_t L2>
    void assign(CSlice<T, L2> data) {
        static_assert(L2 <= L, "Bad slice length");
        memcpy(this->data(), data.cdata(), data.sizeBytes());
    }

    /**
     * Get the element at the given index.
     * 
     * WARNING: This method does no static or runtime bounds-checking.
     * 
     * \param i An index.  If i >= L, the return value is undefined.
     * 
     * \return The element at index i.
     */
    T& operator[](size_t i) {
        return this->data()[i];
    }

    /**
     * Make a smaller slice.
     * 
     * The bounds given as template params are statically checked to ensure
     * memory-safety.
     * 
     * \tparam Start The index of the first element in the slice.  Default: 0.
     * \tparam End The next index after the index of the last element in the
     * slice.  Default: L.
     * \return A slice pointing from the element at index Start (inclusive) to
     * the element at index End (exclusive).
     */
    template<size_t Start = 0, size_t End = L>
    Slice<T, End - Start> slice() {
        SLICE_METH_ASSERTS();
        return Slice<T, End - Start>(this->data() + Start);
    }
};

/**
 * \brief An array with a fixed length known at compile-time.
 * 
 * Instances take up the same amount of space as a regular C array.  Their
 * size in bytes can be retrieved with the "sizeof" operator.
 */
template<typename T, size_t L>
class Array
{
public:
    Array(const Array& other) = delete;
    Array(Array& other) = delete;
    Array& operator=(const Array& other) = delete;
    Array& operator=(Array& other) = delete;

    /**
     * Get a pointer to the array's data.
     * 
     * @tparam Offset An offset (in number of instances of T) to apply to the
     * pointer before returning it.  The value is statically checked to ensure
     * memory-safety.  Default: 0.
     */
    template<size_t Offset = 0>
    const T *cdata() const {
        DATA_METH_ASSERTS();
        return this->_data + Offset;
    }

    /**
     * Get a pointer to the slice's data.
     * 
     * @tparam Offset An offset (in number of instances of T) to apply to the
     * pointer before returning it.  The value is statically checked to ensure
     * memory-safety.  Default: 0.
     */
    template<size_t Offset = 0>
    T *data() {
        return (T *) this->template cdata<Offset>();
    }

    /**
     * WARNING: This method does no static or runtime bounds-checking.
     * 
     * \param i An index.  If i >= L, the return value is undefined.
     * 
     * \return The element at index i.
     */
    const T& operator[](size_t i) const {
        return this->_data[i];
    }

    /**
     * Get the element at the given index.
     * 
     * WARNING: This method does no static or runtime bounds-checking.
     * 
     * \param i An index.  If i >= L, the return value is undefined.
     * 
     * \return The element at index i.
     */
    T& operator[](size_t i) {
        return this->_data[i];
    }

    /**
     * \return A pointer to the array's data.
     */
    CArrayPtr<T> operator&() const {
        return CArrayPtr<T>(this->_data, L);
    }

    /**
     * \return The number of instances of T in the array.
     */
    constexpr static size_t size() {
        return L;
    }

    /**
     * Fill the array with the given value.
     */
    void fill(T val) {
        this->slice().fill(val);
    }

    /**
     * Copy data from a slice into this array.
     * 
     * \param data A slice from which to copy data.  Its size is statically
     * checked to ensure memory-safety.
     */
    template<size_t L2>
    void assign(CSlice<T, L2> data) {
        static_assert(L2 <= L, "Bad slice length");
        this->slice().assign(data);
    }

    /**
     * Make a slice of this array.
     * 
     * The bounds given as template params are statically checked to ensure
     * memory-safety.
     * 
     * \tparam Start The index of the first element in the slice.  Default: 0.
     * \tparam End The next index after the index of the last element in the
     * slice.  Default: L.
     * \return A slice pointing from the element at index Start (inclusive) to
     * the element at index End (exclusive).
     */
    template<size_t Start = 0, size_t End = L>
    CSlice<T, End - Start> cslice() const {
        SLICE_METH_ASSERTS();
        return CSlice<T, End - Start>(this->_data + Start);
    }

    /**
     * Make a slice of this array.
     * 
     * The bounds given as template params are statically checked to ensure
     * memory-safety.
     * 
     * \tparam Start The index of the first element in the slice.  Default: 0.
     * \tparam End The next index after the index of the last element in the
     * slice.  Default: L.
     * \return A slice pointing from the element at index Start (inclusive) to
     * the element at index End (exclusive).
     */
    template<size_t Start = 0, size_t End = L>
    Slice<T, End - Start> slice() {
        SLICE_METH_ASSERTS();
        return Slice<T, End - Start>(this->_data + Start);
    }

    T _data[L];
} __attribute__((packed));

static_assert(sizeof(Array<char, 10>) == 10, "Bad definition of Array");

template<typename T, size_t L>
Array<T, L>& operator^=(Array<T, L>& array, unsigned char v) {
    for (size_t i; i < L; ++i) {
        array[i] ^= v;
    }
    return array;
}

/**
 * Copy data from one slice into another.
 * 
 * NOTE: Sizes are statically checked to ensure memory-safety.
 * 
 * \param dest The slice into which to copy data.
 * \param data The slice from which to copy data.
 * 
 * \return A slice pointing to the section of the destination slice that wasn't
 * written to.
 */
template<typename T, size_t L1, size_t L2>
Slice<T, L1 - L2> operator<<(Slice<T, L1> dest, CSlice<T, L2> data) {
    dest.assign(data);
    return dest.template slice<L2>();
}

/**
 * Copy data from a slice into an array.
 * 
 * NOTE: Sizes are statically checked to ensure memory-safety.
 * 
 * \param dest The array into which to copy data.
 * \param data The slice from which to copy data.
 * 
 * \return A slice pointing to the section of the array that wasn't
 * written to.
 */
template<typename T, size_t L1, size_t L2>
Slice<T, L1 - L2> operator<<(Array<T, L1>& dest, CSlice<T, L2> data) {
    return dest.slice() << data;
}

using CByteArrayPtr = CArrayPtr<unsigned char>;

template<size_t L>
using CByteSlice = CSlice<unsigned char, L>;

template<size_t L>
using ByteSlice = Slice<unsigned char, L>;

template<size_t L>
using ByteArray = Array<unsigned char, L>;

template<typename T, size_t L>
inline const T *cast(const ByteArray<L>& array) {
    static_assert(sizeof(T) <= sizeof(array), "Unsafe cast");
    return (const T *) array.cdata();
}

template<typename T, size_t L>
inline T *cast(ByteArray<L>& array) {
    const T *p = cast<T>((const ByteArray<L>&) array);
    return (T *) p;
}

} // namespace safearray

#endif