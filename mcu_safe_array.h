#ifndef __MCU_SAFE_ARRAY_H__
#define __MCU_SAFE_ARRAY_H__

/**
 * \mainpage
 * 
 * This library contains class templates that help with writing memory-safe
 * programs that manipulate arrays of data.  It uses C++11 features to do
 * compile-time bounds checking and thus adds no runtime overhead in time or space.
 * 
 * Here are the classes
 * <ul>
 * <li>\c safearray::Array: An array with a fixed length known at compile-time.
 * Equivalent to a C array.</li>
 * 
 * <li>\c safearray::Slice: A pointer to a section of a C array.  The length of the slice
 * is known at compile-time.</li>
 * 
 * <li>\c safearray::CSlice: Like \c %safearray::Slice, but it points to a const C array.</li>
 * 
 * <li>\c safearray::CArrayPtr: A pointer to a const C array with a size known at runtime.
 * This is really just a way to pass a C array and its size in one object.</li>
 * </ul>
 * 
 * Here's an example of using \c %safearray::Array to parse messages received over a network.
 * First, we define structs for the message types:
 * 
 * \code
 * typedef enum { HELLO, BYE } msg_type_t;
 * 
 * struct HelloMsg {
 *     msg_type_t                      type; // will be HELLO
 *     safearray::Array<char, 20>      my_name;
 *     uint32_t                        my_id;
 *     safearray::Array<uint8_t, 32>   hmac;
 * };
 * 
 * // sizeof(HelloMsg) == sizeof(msg_type_t) + sizeof(char) * 20 + 
 * // sizeof(uint32_t) + sizeof(uint8_t) bytes * 32.
 * 
 * struct ByeMsg {
 *     msg_type_t                    type; // will be BYE
 *     safearray::Array<uint8_t, 32> hmac;
 * };
 * 
 * // sizeof(ByteMsg) == sizeof(msg_type_t) + sizeof(uint8_t) * 32 bytes.
 * \endcode
 * 
 * Note that \c %safearray::Array can be used as a field type, since it has
 * the same size as a C array.
 * 
 * Now we define a global buffer for storing bytes received over the network:
 * 
 * \code
 * #define MAX_MSG_SIZE (sizeof(HelloMsg) > sizeof(ByeMsg) ? \
 *      sizeof(HelloMsg) : sizeof(ByeMsg))
 * 
 * static safearray::Array<uint8_t, MAX_MSG_SIZE> g_buff;
 * \endcode
 * 
 * Assume that these functions are defined somewhere:
 * 
 * \code
 * void lowlevel_recv(void *b, size_t len);
 * void process_hello(const HelloMsg* msg);
 * void process_bye(const ByeMsg* msg);
 * \endcode
 * 
 * Finally, we define a function that parses messages:
 * 
 * \code
 * void radio_recv() {
 *      // copy data from low-level API into g_buff
 *      lowlevel_recv(g_buff.data(), g_buff.size());
 * 
 *      // temporarily cast to msg_type_t to read msg type (which is at
 *      // the beginning of the buffer)
 *      const msg_type_t *type = safearray::cast<msg_type_t>(g_buff);
 *      switch (*type) {
 *          case HELLO: {
 *              const HelloMsg *msg = safearray::cast<HelloMsg>(g_buff);
 *              process_hello(msg);
 *              break;
 *          }
 * 
 *          case BYE: {
 *              const ByeMsg *msg = safearray::cast<ByeMsg>(g_buff);
 *              process_bye(msg);
 *              break;
 *          }
 *      }
 * }
 * \endcode
 * 
 * \c radio_recv uses \c safearray::cast to interpret the bytes stored
 * in the static array \c g_buff.  \c safearray::cast statically checks
 * that the buffer is large enough to store instances of the requested
 * type.
 */

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
 * \brief A pointer to a const C array with a size known at runtime.
 * 
 * \tparam T The type of the elements of the array.
 * 
 * The size is known only at runtime, so no compile-time bounds-checking is
 * done.  This is really just a way to pass a C array and its size in one
 * object.
 */
template<typename T>
class CArrayPtr
{
public:
    /**
     * \brief Make a pointer to a const C array.
     * 
     * \param data A const C array
     * \param size The number of instances of \c T in \c data
     */
    CArrayPtr(const T *data, size_t size): _data(data), _size(size) {}

    /**
     * \brief Get the size of the C array.
     * 
     * \return The number of instances of \c T in the array
     */
    size_t size() const {
        return this->_size;
    }

    /**
     * \brief Get the C array.
     * 
     * \return The C array
     */
    const T *data() const {
        return this->_data;
    }

private:
    const T *_data;
    size_t _size;
};

/** 
 * \brief A const pointer to a section of a C array.
 * 
 * \tparam T The type of the elements of the slice.
 * \tparam L The size (i.e., number of instances of \c T) of the slice.
 * 
 * The slice itself has a fixed length known at compile-time, which is
 * (hopefully) less than or equal to the size of the array.  Several methods do
 * compile-time bounds-checking to ensure memory-safety.
 * 
 * NOTE: Since a slice is a pointer, it's possible to give it a value
 * of \c NULL, which is what the default constructor does.
 */
template<typename T, size_t L>
class CSlice
{
public:
    /**
     * \brief Make a slice pointing to nothing (\c NULL).
     * 
     * WARNING: Most of the other methods will have undefined behavior 
     * (and thus memory safety is not guaranteed) until this object is
     * given a valid data pointer.
     */
    CSlice() : _data(NULL) {};

    /**
     * \brief Make a slice pointing to \c L instances of \c T at the given
     * location.
     * 
     * WARNING: This class provides memory-safety only if the given pointer
     * points to \c L contiguous instances of \c T.
     */
    explicit CSlice(const T *data) : _data(data) {}

    /**
     * \brief Get a pointer to the data.
     * 
     * \tparam Offset An offset (in number of instances of \c T) to apply to the
     * pointer before returning it.  The value is statically checked to ensure
     * memory-safety.  Default: \c 0.
     * 
     * \return \c P + \c Offset, where \c P is a pointer to the beginning of
     * the data.
     */
    template<size_t Offset = 0>
    const T *cdata() const {
        DATA_METH_ASSERTS();
        return this->_data + Offset;
    }

    /**
     * \brief Get the element at a particular index.
     * 
     * WARNING: This method does no static or runtime bounds-checking.
     * 
     * \param i An index.  If \c i \c >= \c L, the return value is undefined.
     * 
     * \return A reference to the element at index \c i.
     */
    const T& operator[](size_t i) const {
        return this->_data[i];
    }

    /**
     * Make a pointer to the data.
     * 
     * \return A \c CArrayPtr to the data.
     */
    CArrayPtr<T> operator&() const {
        return CArrayPtr<T>(this->_data, L);
    }

    /**
     * \brief Make a slice pointing to a section of the data.
     * 
     * The bounds given as template params are statically checked to ensure
     * memory-safety.
     * 
     * \tparam Start The index of the first element in the slice.  Default: \c 0.
     * \tparam End The next index after the index of the last element in the
     * slice.  Default: \c L.
     * 
     * \return A slice pointing from the element at index \c Start (inclusive) to
     * the element at index \c End (exclusive).
     */
    template<size_t Start = 0, size_t End = L>
    CSlice<T, End - Start> cslice() const {
        SLICE_METH_ASSERTS();
        return CSlice<T, End - Start>(this->cdata<Start>());
    }

    /**
     * \brief Get the number of instances of \c T in the slice.
     * 
     * \return The number of instances of \c T in the slice.
     */
    constexpr static size_t size() {
        return L;
    }

    /**
     * \brief Get the number of bytes used by the slice's data.
     * 
     * \return The number of bytes used by the slice's data.
     */
    constexpr static size_t sizeBytes() {
        return L * sizeof(T);
    }

protected:
    const T *_data;
};

/** 
 * \brief A pointer to a section of a C array.
 * 
 * \tparam T The type of the elements of the slice.
 * \tparam L The size (i.e., number of instances of \c T) of the slice.
 * 
 * The slice itself has a fixed length known at compile-time, which is
 * (hopefully) less than or equal to the size of the array.  Several methods do
 * compile-time bounds-checking to ensure memory-safety.
 * 
 * NOTE: Since a slice is a pointer, it's possible to give it a value
 * of \c NULL, which is what the default constructor does.
 */
template<typename T, size_t L>
class Slice : public CSlice<T, L>
{
public:
    /**
     * \brief Make a slice pointing to nothing (\c NULL).
     * 
     * WARNING: Most of the other methods will have undefined behavior 
     * (and thus memory safety is not guaranteed) until this object is
     * given a valid data pointer.
     */
    Slice() : CSlice<T, L>() {};

    /**
     * \brief Make a slice pointing to \c L instances of \c T at the given location.
     * 
     * WARNING: This class provides memory-safety only if the given pointer
     * points to \c L contiguous instances of \c T.
     * 
     * \param data A pointer to the beginning of the slice's data.
     */
    explicit Slice(T *data) : CSlice<T, L>(data) {}

    /**
     * \brief Get a pointer to the data.
     * 
     * \tparam Offset An offset (in number of instances of \c T) to apply to
     * the pointer before returning it.  The value is statically checked to
     * ensure memory-safety.  Default: \c 0.
     * 
     * \return \c P \c + \c Offset, where \c P is a pointer to the beginning
     * of the slice's data.
     */
    template<size_t Offset = 0>
    T *data() {
        return (T *) this->template cdata<Offset>();
    }

    /**
     * \brief Fill with the given value.
     */
    void fill(T val) {
        for (size_t i = 0; i < L; ++i) {
            (*this)[i] = val;
        }
    }

    /**
     * \brief Copy data from a slice.
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
     * \copydoc CSlice::operator[]
     */
    T& operator[](size_t i) {
        return this->data()[i];
    }

    /**
     * \copydoc CSlice::cslice
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
 * \tparam T The type of the elements of the array.
 * \tparam L The size (i.e., number of instances of \c T) of the array.
 * 
 * Several methods do compile-time bounds-checking to ensure memory-safety.
 * 
 * Instances take up the same amount of space as a regular C array.  Their
 * size in bytes can be retrieved with the "sizeof" operator.
 */
template<typename T, size_t L>
class Array
{
public:
    /**
     * \brief This constructor is deleted to prevent accidental copies.
     */
    Array(const Array& other) = delete;

    /**
     * \brief This constructor is deleted to prevent accidental copies.
     */
    Array(Array& other) = delete;

    /**
     * \brief This method is deleted to prevent accidental copies.
     */
    Array& operator=(const Array& other) = delete;

    /**
     * \brief This method is deleted to prevent accidental copies.
     */
    Array& operator=(Array& other) = delete;

    /**
     * \copydoc CSlice::cdata
     */
    template<size_t Offset = 0>
    const T *cdata() const {
        DATA_METH_ASSERTS();
        return this->_data + Offset;
    }

    /**
     * \copydoc Slice::data
     */
    template<size_t Offset = 0>
    T *data() {
        return (T *) this->template cdata<Offset>();
    }

    /**
     * \copydoc CSlice::operator[]
     */
    const T& operator[](size_t i) const {
        return this->_data[i];
    }

    /**
     * \copydoc Slice::operator[]
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
     * \copydoc CSlice::size
     */
    constexpr static size_t size() {
        return L;
    }

    /**
     * \copydoc Slice::fill
     */
    void fill(T val) {
        this->slice().fill(val);
    }

    /**
     * \copydoc Slice::assign
     */
    template<size_t L2>
    void assign(CSlice<T, L2> data) {
        static_assert(L2 <= L, "Bad slice length");
        this->slice().assign(data);
    }

    /**
     * \copydoc CSlice::cslice
     */
    template<size_t Start = 0, size_t End = L>
    CSlice<T, End - Start> cslice() const {
        SLICE_METH_ASSERTS();
        return CSlice<T, End - Start>(this->_data + Start);
    }

    /**
     * \copydoc Slice::slice
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
 * \copydoc safearray::operator<<(Slice<T, L1>, CSlice<T, L2>)
 */
template<typename T, size_t L1, size_t L2>
Slice<T, L1 - L2> operator<<(Array<T, L1>& dest, CSlice<T, L2> data) {
    return dest.slice() << data;
}

/**
 * A pointer to a byte array.
 */
using CByteArrayPtr = CArrayPtr<unsigned char>;

/**
 * A constant slice of a byte array.
 */
template<size_t L>
using CByteSlice = CSlice<unsigned char, L>;

/**
 * A slice of a byte array.
 */
template<size_t L>
using ByteSlice = Slice<unsigned char, L>;

/**
 * A byte array.
 */
template<size_t L>
using ByteArray = Array<unsigned char, L>;

/**
 * Cast the bytes in a byte array to another datatype.
 * 
 * The size of the array and the target datatype are statically compared to
 * ensure memory-safety.
 * 
 * \tparam T The datatype to cast the bytes to.
 * 
 * \param array The array containing the byte that will be cast.
 * 
 * \return A pointer to be beginning of the array but cast to the specified
 * datatype.
 */
template<typename T, size_t L>
inline const T *cast(const ByteArray<L>& array) {
    static_assert(sizeof(T) <= sizeof(array), "Unsafe cast");
    return (const T *) array.cdata();
}

/**
 * \copydoc safearray::cast(const ByteArray<L>&)
 */
template<typename T, size_t L>
inline T *cast(ByteArray<L>& array) {
    const T *p = cast<T>((const ByteArray<L>&) array);
    return (T *) p;
}

} // namespace safearray

#endif
