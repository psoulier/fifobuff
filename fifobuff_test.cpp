#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include "gtest/gtest.h"
#include "fifobuff.hpp"

#define CAP 10

/*
 * Test adding to FIFOBuff.
 */
TEST(FIFOBuffTest, add) {
    FIFOBuff<int>   fb(CAP);
    int             tmp;

    for (int i = 0; i < CAP; i++) {
        ASSERT_TRUE(fb.add(i));
    }

    ASSERT_FALSE(fb.add(13));
}

/*
 * Test removing from FIFOBuff.
 */
TEST(FIFOBuffTest, remove) {
    FIFOBuff<int>   fb(CAP);
    int             tmp;

    ASSERT_FALSE(fb.remove(nullptr));
    ASSERT_FALSE(fb.remove(&tmp));

    for (int i = 0; i < CAP; i++) {
        fb.add(i);
    }

    for (int i = 0; i < CAP; i++) {
        fb.remove(&tmp);
        ASSERT_EQ(i, tmp);
    }

    ASSERT_FALSE(fb.remove(nullptr));
}

/*
 * Test wrap-around case for FIFO.
 */
TEST(FIFOBuffTest, wrap) {
    FIFOBuff<int>   fb(CAP);
    int             tmp;

    // Add [0, CAP)
    for (int i = 0; i < CAP; i++) {
        fb.add(i);
    }

    // Remove [0, CAP/2)
    for (int i = 0; i < CAP/2; i++) {
        fb.remove(nullptr);
    }

    // Add [CAP/2, CAP + CAP/2)
    for (int i = 0; i < CAP/2; i++) {
        fb.add(CAP + i);
    }

    // Make sure fifo has [CAP/2, CAP+CAP/2)
    for (int i = 0; i < CAP; i++) {
        fb.remove(&tmp);
        ASSERT_EQ(i+CAP/2, tmp);
    }
}

class Dummy {
    public:

        static int  count;
        int         inst;

        Dummy() : inst(count) {
            count++;
        }

        Dummy(const Dummy &dum) : inst(dum.inst) {
            count++;
        }

        ~Dummy() {
            count--;
        }
};

int Dummy::count = 0;

/*
 * The FIFOBuff template class uses a placement new to invoke the constructor
 * of an added object.  Because of this, we also need to explicityly call the
 * destructor to ensure consistent C++ semantics.
 */
TEST(FIFOBuffTest, cleanup) {
    FIFOBuff<Dummy> *pfb;
    int             tmp;

    pfb = new FIFOBuff<Dummy>(CAP);

    for (int i = 0; i < CAP; i++) {
        pfb->add(Dummy());
    }

    // Remove half the objects in the FIFO.
    for (int i = 0; i < CAP/2; i++) {
        pfb->remove(nullptr);
    }

    // The FIFOBuff destructor should ensure the remaining elements in the FIFO are
    // correctly destroyed.
    delete pfb;

    // The static "count" field of the "Dummy" class is incremented in every
    // constructor and decremented in the destructor.  When the "pfb" is deleted,
    // the could should be zero.
    ASSERT_EQ(0, Dummy::count);
}

#define POISON          -13
#define NUM_CONSUMERS   10
#define PRODUCTS        100000

int     check[PRODUCTS] = {0, };

void* consumer(void *arg) {
    FIFOBuff_TS<int>    *pfb = (FIFOBuff_TS<int>*)arg;
    int                 i;

    // Keep consuming until "poison" value is read.
    do {
        pfb->remove_wait(&i);


        /*
         * For a basic test, increment the check array for this integer.
         * Each array element should be incremented exactly once.
         */
        if (i != POISON) {
            check[i]++;
        }
    } while (i != POISON);

    return nullptr;
}

/*
 * Sanity check FIFOBuff_TS using multiple threads.  Certainly not comprehensive,
 * but checks some basic functionality.
 */
TEST(FIFOBuffTest, threaded) {
    pthread_t           threads[NUM_CONSUMERS];
    FIFOBuff_TS<int>    *pfb;
    int                 tmp;

    pfb = new FIFOBuff_TS<int>(CAP);

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&threads[i], nullptr, consumer, (void*)pfb);
    }

    for (int i = 0; i < PRODUCTS; i++) {
        pfb->add_wait(i);
    }

    /*
     * Done "producing" stuff; send poison value to get threads to stop.
     */
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pfb->add_wait(POISON);
    }

    // Wait for all threads to complete...
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(threads[i], nullptr);
    }

    // FIFO better be empty here.
    ASSERT_FALSE(pfb->peek(&tmp));

    /*
     * Make sure each 'check' element is equal to 1.  Won't catch many
     * race-like bugs, but it's better than nothing...
     */
    for (int i = 0; i < PRODUCTS; i++) {
        ASSERT_EQ(1, check[i]);
    }

    delete pfb;
}




