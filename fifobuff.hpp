/*
 * File: fifobuff.hpp
 * Author: Paul Soulier
 *
 * Provides implementation for single-threaded and multi-threaded (thread-safe)
 * fixed-sized FIFO buffer.
 *
 */
#ifndef __FIFOBUFF_HPP__
#define __FIFOBUFF_HPP__

#include <stdint.h>
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>

/*
 * FIFO Buffer Class
 *
 * Implements a non-thread-safe, fixed-sized, FIFO buffer.
 * param T: type of element to store in buffer.
 */
template <typename T>
class FIFOBuff {
    typedef uint8_t item_mem_t[sizeof(T)];

    item_mem_t  *buffer;
    size_t      head;
    size_t      tail;
    size_t      fifo_size;
    size_t      fifo_cap;
    bool        free_mem;

public:

    FIFOBuff() = delete;

    /*
     * Construct a FIFO buffer with a dynamically allocated buffer that can
     * contain a max of 'fifo_size' elements.
     *
     * param fifo_size: Max number of elements FIFO can hold.
     */
    FIFOBuff(size_t max_cap) : head(0), tail(0), fifo_size(0), fifo_cap(max_cap), free_mem(true) {
        buffer = new item_mem_t[fifo_cap];
    }

    /*
     * Construct a FIFO buffer using memory provided by caller.
     *
     * param buf: Pointer to buffer memory of at least "sizeof(T) * fifo_size" bytes.
     * param fifo_size: max number of elements FIFO can hold.
     */
    FIFOBuff(void *buf, size_t max_cap) : head(0), tail(0), fifo_size(0), fifo_cap(max_cap), free_mem(false) {
        buffer = reinterpret_cast<item_mem_t*>(buf);
    }

    ~FIFOBuff() {
        /*
         * When queue is deallocated, cleanup any leftover elements.
         */
        while (fifo_size > 0) {
            reinterpret_cast<T*>(buffer + head)->~T();
            head = (head + 1) % fifo_cap;
            fifo_size--;
        }

        if (free_mem) {
            delete [] buffer;
        }
    }

    /*
     * Returns current number of elements in FIFO.
     */
    size_t size() const {
        return fifo_size;
    }

    /*
     * Returns max number of elements FIFO can hold.
     */
    size_t capacity() const {
        return fifo_cap;
    }

    /*
     * Adds element to the back of the FIFO by copying data referenced by 'item' into
     * FIFO buffer.
     *
     * @return Returns true if 'item' was added, false if FIFO was full.
     */
    bool add(const T &item) {
        if (fifo_size < fifo_cap) {
            new (buffer + tail) T(item);
            tail = (tail + 1) % fifo_cap;
            fifo_size++;

            return true;
        }
        else {
            return false;
        }
    }

    /**
     * Removes the item at the head of the FIFO.
     *
     * @param pitem If not null, the element being removed is copied to 'pitem' before
     *        removal.
     * @return returns true if an element was removed, false if FIFO was empty.
     */
    bool remove(T *pitem) {
        if (fifo_size == 0) {
            return false;
        }
        else {
            if (pitem != nullptr) {
                *pitem = *reinterpret_cast<T*>(buffer + head);
            }

            /*
             * Call destructor for 'T'.  If 'T' is a POD or built-in type, this will be
             * a NOP.
             */
            reinterpret_cast<T*>(buffer + tail)->~T();

            head = (head + 1) % fifo_cap;
            fifo_size--;

            return true;
        }
    }

    /**
     * Returns a pointer to the element at the front of FIFO without removing
     * it from the buffer.
     *
     * return: Returns pointer to element or null if FIFO is empty.
     */
    bool peek(T *pitem) const {
        if (fifo_size > 0) {
            *pitem = *reinterpret_cast<T*>(buffer + tail);
            return true;
        }
        else {
            return false;
        }
    }
};

/*
 * Implements a thread-safe FIFO buffer.
 *
 * Provides a thread-safe wrapper for the FIFOBuff class.  The interface for
 * FIFOBuff_TS is almost identical to FIFOBuff, but adds two methods: 'add_wait()' and
 * 'remove_wait()'.  Both block until the necessary resource is available.  The 'size()'
 * method is removed since the result is only approximate for multi-threaded environments.
 *
 * NOTE: for the sake of simplicity, no error checking is done on OS semaphore/mutex
 * calls.
 */
template <typename T>
class FIFOBuff_TS {
    FIFOBuff<T>         fifo;
    pthread_mutex_t     mutex;
    sem_t               *add_sem;
    sem_t               *rem_sem;

    /*
     * Initialize Posix/pthread semaphores and mutex.
     */
    void init() {
        add_sem = sem_open("TSQueue_AddSem", O_CREAT, 0600, fifo.capacity());
        rem_sem = sem_open("TSQueue_RemSem", O_CREAT, 0600, 0);
        sem_unlink("TSQueue_AddSem");
        sem_unlink("TSQueue_RemSem");
        pthread_mutex_init(&mutex, nullptr);
    }

public:

    FIFOBuff_TS() = delete;

    FIFOBuff_TS(size_t max_cap) : fifo(max_cap) {
        init();
    }

    FIFOBuff_TS(void *buf, size_t max_cap) : fifo(buf, max_cap) {
        init();
    }


    ~FIFOBuff_TS() {
        /*
         * Cleanup semaphores and mutex.
         */
        sem_close(add_sem);
        sem_close(rem_sem);
        pthread_mutex_destroy(&mutex);
    }

    /*
     * Same as FIFOBuff except thread-safe.
     */
    bool add(const T &item) {
        if (sem_trywait(add_sem) == 0) {
            pthread_mutex_lock(&mutex);
            fifo.add(item);
            pthread_mutex_unlock(&mutex);

            sem_post(rem_sem);

            return true;
        }
        else {
            return false;
        }
    }

    /*
     * Adds an element to FIFO.  If FIFO is full, the call blocks until room
     * becomes available to add the item.
     */
    void add_wait(const T &item) {
        sem_wait(add_sem);

        pthread_mutex_lock(&mutex);
        fifo.add(item);
        pthread_mutex_unlock(&mutex);

        sem_post(rem_sem);
    }

    /*
     * Same as FIFOBuff except thread-safe.
     */
    bool peek(T *item) {
        bool    peek_ok = false;

        pthread_mutex_lock(&mutex);

        if (fifo.size() > 0) {
            peek_ok = true;
            fifo.peek(item);
        }

        pthread_mutex_unlock(&mutex);

        return peek_ok;
    }

    /*
     * Same as FIFOBuff except thread-safe.
     */
    bool remove(T *pitem) {
        if (sem_trywait(rem_sem) == 0) {
            pthread_mutex_lock(&mutex);
            fifo.remove(pitem);
            pthread_mutex_unlock(&mutex);

            sem_post(add_sem);

            return true;
        }
        else {
            return false;
        }
    }

    /*
     * Removes an item from the FIFO and, if not null, copies data to 'pitem'.
     * If the FIFO is empty, call blocks until an element becomes available.
     */
    void remove_wait(T *pitem) {
        sem_wait(rem_sem);

        pthread_mutex_lock(&mutex);
        fifo.remove(pitem);
        pthread_mutex_unlock(&mutex);

        sem_post(add_sem);
    }
};

#endif
