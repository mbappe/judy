// Copyright (C) 2000 - 2002 Hewlett-Packard Company
//
// This program is free software; you can redistribute it and/or modify it
// under the term of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// _________________

// @(#) $Revision: 4.45 $ $Source: /judy/src/JudyCommon/JudyMallocIF.c $
//
// Judy malloc/free interface functions for Judy1 and JudyL.
//
// Compile with one of -DJUDY1 or -DJUDYL.
//
// Compile with -DTRACEMI (Malloc Interface) to turn on tracing of malloc/free
// calls at the interface level.  (See also TRACEMF in lower-level code.)
// Use -DTRACEMI2 for a terser format suitable for trace analysis.
//
// There can be malloc namespace bits in the LSBs of "raw" addresses from most,
// but not all, of the j__udy*Alloc*() functions; see also JudyPrivate.h.  To
// test the Judy code, compile this file with -DMALLOCBITS and use debug flavor
// only (for assertions).  This test ensures that (a) all callers properly mask
// the namespace bits out before dereferencing a pointer (or else a core dump
// occurs), and (b) all callers send "raw" (unmasked) addresses to
// j__udy*Free*() calls.
//
// Note:  Currently -DDEBUG turns on MALLOCBITS automatically.

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

// Set "hidden" global j__uMaxWords to the maximum number of words to allocate
// to any one array (large enough to have a JPM, otherwise j__uMaxWords is
// ignored), to trigger a fake malloc error when the number is exceeded.  Note,
// this code is always executed, not #ifdefd, because its virtually free.
//
// Note:  To keep the MALLOC macro faster and simpler, set j__uMaxWords to
// MAXINT, not zero, by default.

Word_t j__uMaxWords = ~(Word_t)0;

// This macro hides the faking of a malloc failure:
//
// Note:  To keep this fast, just compare WordsPrev to j__uMaxWords without the
// complexity of first adding WordsNow, meaning the trigger point is not
// exactly where you might assume, but it shouldnt matter.

#define MALLOC(MallocFunc,WordsPrev,WordsNow) \
        (((WordsPrev) > j__uMaxWords) ? 0 : MallocFunc(WordsNow))

// Clear words starting at address:
//
// Note:  Only use this for objects that care; in other cases, it doesnt
// matter if the objects memory is pre-zeroed.

#define ZEROWORDS(Addr,Words)                   \
        {                                       \
            Word_t  Words__ = (Words);          \
            PWord_t Addr__  = (PWord_t) (Addr); \
            while (Words__--) *Addr__++ = 0;  \
        }

#ifdef TRACEMI

// TRACING SUPPORT:
//
// Note:  For TRACEMI, use a format for address printing compatible with other
// tracing facilities; in particular, %x not %lx, to truncate the "noisy" high
// part on 64-bit systems.
//
// TBD: The trace macros need fixing for alternate address types.
//
// Note:  TRACEMI2 supports trace analysis no matter the underlying malloc/free
// engine used.

#include <stdio.h>

static Word_t j__udyMemSequence = 0;   // event sequence number.

#define TRACE_ALLOC4(a,b,c,d)     (void) printf((a), (b), (c), (d))
#define TRACE_ALLOC5(a,b,c,d,e)   (void) printf((a), (b), (c), (d), (e))
#define TRACE_FREE5( a,b,c,d,e)   (void) printf((a), (b), (c), (d), (e))
#define TRACE_ALLOC6(a,b,c,d,e,f) (void) printf((a), (b), (c), (d), (e), (f))
#define TRACE_FREE6( a,b,c,d,e,f) (void) printf((a), (b), (c), (d), (e), (f))

#else

#ifdef TRACEMI2


#include <stdio.h>

// not implemented yet
#define b_pw cJU_BYTESPERWORD

#define TRACE_ALLOC5(a,b,c,d,e)   \
            (void) printf("a %lx %lx %lx\n", (b), (d) * b_pw, e)
#define TRACE_FREE5( a,b,c,d,e)   \
            (void) printf("f %lx %lx %lx\n", (b), (d) * b_pw, e)
#define TRACE_ALLOC6(a,b,c,d,e,f)         \
            (void) printf("a %lx %lx %lx\n", (b), (e) * b_pw, f)
#define TRACE_FREE6( a,b,c,d,e,f)         \
            (void) printf("f %lx %lx %lx\n", (b), (e) * b_pw, f)

static Word_t j__udyMemSequence = 0;   // event sequence number.

#else

#define TRACE_ALLOC4(a,b,c,d)     // null.
#define TRACE_ALLOC5(a,b,c,d,e)   // null.
#define TRACE_FREE5( a,b,c,d,e)   // null.
#define TRACE_ALLOC6(a,b,c,d,e,f) // null.
#define TRACE_FREE6( a,b,c,d,e,f) // null.

#endif // ! TRACEMI2
#endif // ! TRACEMI


// MALLOC NAMESPACE SUPPORT:

#ifdef MALLOCBITS
#define MALLOCBITS_VALUE 0x3    // bit pattern to use.
#define MALLOCBITS_MASK  0x7    // note: matches mask__ in JudyPrivate.h.

#define MALLOCBITS_SET(Addr)    ((Addr) |= MALLOCBITS_VALUE)
#define MALLOCBITS_TEST(Addr)                                   \
        assert(((Addr) & MALLOCBITS_MASK) == MALLOCBITS_VALUE); \
        ((Addr) &= ~MALLOCBITS_VALUE)
#else
#define MALLOCBITS_SET(Addr)    // null.
#define MALLOCBITS_TEST(Addr)   // null.
#endif


// SAVE ERROR INFORMATION IN A Pjpm:
//
// "Small" (invalid) Addr values are used to distinguish overrun and no-mem
// errors.  (TBD, non-zero invalid values are no longer returned from
// lower-level functions, that is, JU_ERRNO_OVERRUN is no longer detected.)

#define J__UDYSETALLOCERROR(Addr)                                       \
        {                                                               \
            JU_ERRID(Pjpm) = __LINE__;                                  \
            if ((Word_t) (Addr) > 0) JU_ERRNO(Pjpm) = JU_ERRNO_OVERRUN; \
            else                     JU_ERRNO(Pjpm) = JU_ERRNO_NOMEM;   \
            return(0);                                                  \
        }

// These globals are defined in JudyMalloc.c so we don't define them
// in more than one place, i.e. Judy1MallocIFc. and JudyLMallocIF.c.
#ifdef RAMMETRICS
extern Word_t    j__AllocWordsJBB;
extern Word_t    j__AllocWordsJBU;
extern Word_t    j__AllocWordsJBL;
extern Word_t    j__AllocWordsJLB1;
// extern Word_t    j__AllocWordsJLB2;
extern Word_t    j__AllocWordsJLL[8];
#define   j__AllocWordsJLLW  j__AllocWordsJLL[0]
#define   j__AllocWordsJLL7  j__AllocWordsJLL[7]
#define   j__AllocWordsJLL6  j__AllocWordsJLL[6]
#define   j__AllocWordsJLL5  j__AllocWordsJLL[5]
#define   j__AllocWordsJLL4  j__AllocWordsJLL[4]
#define   j__AllocWordsJLL3  j__AllocWordsJLL[3]
#define   j__AllocWordsJLL2  j__AllocWordsJLL[2]
#define   j__AllocWordsJLL1  j__AllocWordsJLL[1]
extern Word_t    j__AllocWordsJV;
extern Word_t    j__NumbJV;
#endif // RAMMETRICS

// ****************************************************************************
// ALLOCATION FUNCTIONS:
//
// To help the compiler catch coding errors, each function returns a specific
// object type.
//
// Note:  Only j__udyAllocJPM() and j__udyAllocJLLW() return multiple values <=
// sizeof(Word_t) to indicate the type of memory allocation failure.  Other
// allocation functions convert this failure to a JU_ERRNO.


// Note:  Unlike other j__udyAlloc*() functions, Pjpms are returned non-raw,
// that is, without malloc namespace or root pointer type bits:

typedef unsigned long U_t;      // because unitptr_t is defined wrong

FUNCTION Pjpm_t j__udyAllocJPM(void)
{
        Word_t Words = (sizeof(jpm_t) + cJU_BYTESPERWORD - 1) / cJU_BYTESPERWORD;
        Pjpm_t Pjpm  = (Pjpm_t) MALLOC(JudyMalloc, Words, Words);

        assert((Words * cJU_BYTESPERWORD) == sizeof(jpm_t));

        if ((Word_t) Pjpm > sizeof(Word_t))
        {
            ZEROWORDS(Pjpm, Words);
            Pjpm->jpm_TotalMemWords = Words;

#ifdef RAMMETRICS
//            j__AllocWordsJLLW += Words;
#endif // RAMMETRICS

        }

        TRACE_ALLOC4("\n%p %8zd = j__udyAllocJPM(%d)\n", 
                (void *)Pjpm, j__udyMemSequence++, (int)Words);
        // MALLOCBITS_SET(Pjpm_t, Pjpm);  // see above.
        return(Pjpm);

} // j__udyAllocJPM()


FUNCTION Word_t j__udyAllocJBL(Pjpm_t Pjpm)
{
        Word_t Words   = sizeof(jbl_t) / cJU_BYTESPERWORD;
        Word_t PjblRaw = MALLOC(JudyMallocVirtual, Pjpm->jpm_TotalMemWords, Words);

        assert((Words * cJU_BYTESPERWORD) == sizeof(jbl_t));

        if (PjblRaw > sizeof(Word_t))
        {
            ZEROWORDS(P_JBL(PjblRaw), Words);
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJBL += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjblRaw); }

        TRACE_ALLOC5("\n%p %8zd = j__udyAllocJBL(), Words = %d, TotPop1 = %zd\n", 
                (void *)PjblRaw, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjblRaw);
        return(PjblRaw);

} // j__udyAllocJBL()


FUNCTION Word_t j__udyAllocJBB(Pjpm_t Pjpm)
{
        Word_t Words   = sizeof(jbb_t) / cJU_BYTESPERWORD;
        Word_t PjbbRaw = MALLOC(JudyMallocVirtual,
                                         Pjpm->jpm_TotalMemWords, Words);

        assert((Words * cJU_BYTESPERWORD) == sizeof(jbb_t));

        if (PjbbRaw > sizeof(Word_t))
        {
            ZEROWORDS(P_JBB(PjbbRaw), Words);
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJBB += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjbbRaw); }

        TRACE_ALLOC5("\n%p %8zd = j__udyAllocJBB(), (int)Words = %d, TotPop1 = %zd\n", 
                (void *)PjbbRaw, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjbbRaw);
        return(PjbbRaw);

} // j__udyAllocJBB()


FUNCTION Word_t j__udyAllocJBBJP(int NumJPs, Pjpm_t Pjpm)
{
        Word_t Words = JU_BRANCHJP_NUMJPSTOWORDS(NumJPs);
            Word_t  PjpRaw;

        PjpRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjpRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJBB += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjpRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJBBJP(%d), Words = %d, TotPop1 = %zd\n",
                (void *)PjpRaw, j__udyMemSequence++, (int)NumJPs, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjpRaw);
        return(PjpRaw);

} // j__udyAllocJBBJP()


FUNCTION Word_t j__udyAllocJBU(Pjpm_t Pjpm)
{
        Word_t Words   = sizeof(jbu_t) / cJU_BYTESPERWORD;
        Word_t PjbuRaw = MALLOC(JudyMallocVirtual,
                                         Pjpm->jpm_TotalMemWords, Words);

        assert((Words * cJU_BYTESPERWORD) == sizeof(jbu_t));

        if (PjbuRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJBU += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjbuRaw); }

        TRACE_ALLOC5("\n%p %8zd = j__udyAllocJBU(), Words = %d, TotPop1 = %zd\n", 
                (void *)PjbuRaw, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjbuRaw);
        return(PjbuRaw);

} // j__udyAllocJBU()


FUNCTION Word_t j__udyAllocJLL1(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF1POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL1 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL1(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL1()


FUNCTION Word_t j__udyAllocJLL2(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF2POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL2 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL2(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL2()


FUNCTION Word_t j__udyAllocJLL3(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF3POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL3 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL3(%d), Words = %d, TotPop1 = %zd\n",
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL3()



FUNCTION Word_t j__udyAllocJLL4(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF4POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL4 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL4(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL4()


FUNCTION Word_t j__udyAllocJLL5(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF5POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL5 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL5(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL5()


FUNCTION Word_t j__udyAllocJLL6(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF6POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL6 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL6(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL6()


FUNCTION Word_t j__udyAllocJLL7(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF7POPTOWORDS(Pop1);
        Word_t PjllRaw;

        PjllRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjllRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLL7 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjllRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLL7(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjllRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjllRaw);
        return(PjllRaw);

} // j__udyAllocJLL7()



// Note:  Root-level leaf addresses are always whole words (Pjllw_t), and unlike
// other j__udyAlloc*() functions, they are returned non-raw, that is, without
// malloc namespace or root pointer type bits (the latter are added later by
// the caller):

FUNCTION Pjllw_t j__udyAllocJLLW(int Pop1)
{
        Word_t Words = JU_LEAFWPOPTOWORDS(Pop1);
        Pjllw_t Pjllw  = (Pjllw_t)MALLOC(JudyMalloc, Words, Words);

#ifdef RAMMETRICS
            j__AllocWordsJLLW += Words;
#endif // RAMMETRICS

        TRACE_ALLOC6("\n%p %8zd = j__udyAllocJLLW(%d), Words = %d, TotPop1 = %d\n", 
                (void *)Pjllw, j__udyMemSequence++, Pop1, (int)Words, Pop1);
        // MALLOCBITS_SET(Pjllw_t, Pjllw);  // see above.
        return(Pjllw);

} // j__udyAllocJLLW()


FUNCTION Word_t j__udyAllocJLB1(Pjpm_t Pjpm)
{
        Word_t Words = sizeof(jlb_t) / cJU_BYTESPERWORD;
        Word_t PjlbRaw;

        PjlbRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        assert((Words * cJU_BYTESPERWORD) == sizeof(jlb_t));

        if (PjlbRaw > sizeof(Word_t))
        {
            ZEROWORDS(P_JLB(PjlbRaw), Words);
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJLB1 += Words;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjlbRaw); }

        TRACE_ALLOC5("\n%p %8zd = j__udyAllocJLB1(), Words = %d, TotPop1 = %zd\n", 
                (void *)PjlbRaw, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjlbRaw);
        return(PjlbRaw);

} // j__udyAllocJLB1()


#ifdef JUDYL

FUNCTION Word_t j__udyLAllocJV(int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JL_LEAFVPOPTOWORDS(Pop1);
        Word_t  PjvRaw;

        PjvRaw = MALLOC(JudyMalloc, Pjpm->jpm_TotalMemWords, Words);

        if (PjvRaw > sizeof(Word_t))
        {
            Pjpm->jpm_TotalMemWords += Words;

#ifdef RAMMETRICS
            j__AllocWordsJV += Words;
            j__NumbJV++;
#endif // RAMMETRICS

        }
        else { J__UDYSETALLOCERROR(PjvRaw); }

        TRACE_ALLOC6("\n%p %8zd = j__udyLAllocJV(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)PjvRaw, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0 + 2);
        MALLOCBITS_SET(PjvRaw);
        return(PjvRaw);

} // j__udyLAllocJV()

#endif // JUDYL


// ****************************************************************************
// FREE FUNCTIONS:
//
// To help the compiler catch coding errors, each function takes a specific
// object type to free.


// Note:  j__udyFreeJPM() receives a root pointer with NO root pointer type
// bits present, that is, they must be stripped by the caller using P_JPM():

FUNCTION void j__udyFreeJPM(Pjpm_t PjpmFree, Pjpm_t PjpmStats)
{
        Word_t Words = (sizeof(jpm_t) + cJU_BYTESPERWORD - 1) / cJU_BYTESPERWORD;

#ifdef RAMMETRICS
//        j__AllocWordsJLLW -= Words;
#endif // RAMMETRICS

        if (PjpmStats != (Pjpm_t) NULL) PjpmStats->jpm_TotalMemWords -= Words;

// Note:  Log PjpmFree->jpm_Pop0, similar to other j__udyFree*() functions, not
// an assumed value of cJU_LEAFW_MAXPOP1, for when the caller is
// Judy*FreeArray(), jpm_Pop0 is set to 0, and the population after the free
// really will be 0, not cJU_LEAFW_MAXPOP1.

        TRACE_FREE5("\n%p %8zd =  j__udyFreeJPM(), Words = %d, TotPop1 = %zd\n", 
                (void *)PjpmFree, j__udyMemSequence++, (int)Words, PjpmFree->jpm_Pop0);

        // MALLOCBITS_TEST(Pjpm_t, PjpmFree);   // see above.
        JudyFree((Word_t) PjpmFree, Words);

} // j__udyFreeJPM()


FUNCTION void j__udyFreeJBL(Word_t Pjbl, Pjpm_t Pjpm)
{
        Word_t Words = sizeof(jbl_t) / cJU_BYTESPERWORD;

        MALLOCBITS_TEST(Pjbl);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJBL -= Words;
#endif // RAMMETRICS

        TRACE_FREE5("\n%p %8zd =  j__udyFreeJBL(), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjbl, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0);

        JudyFreeVirtual(Pjbl, Words);

} // j__udyFreeJBL()


FUNCTION void j__udyFreeJBB(Word_t Pjbb, Pjpm_t Pjpm)
{
        int Words = sizeof(jbb_t) / cJU_BYTESPERWORD;

        MALLOCBITS_TEST(Pjbb);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJBB -= Words;
#endif // RAMMETRICS

        TRACE_FREE5("\n%p %8zd =  j__udyFreeJBB(), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjbb, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0);

        JudyFreeVirtual(Pjbb, Words);

} // j__udyFreeJBB()


FUNCTION void j__udyFreeJBBJP(Word_t Pjp, int NumJPs, Pjpm_t Pjpm)
{
        Word_t Words = JU_BRANCHJP_NUMJPSTOWORDS(NumJPs);

        MALLOCBITS_TEST(Pjp);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJBB -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJBBJP(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjp, j__udyMemSequence++, NumJPs, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjp, Words);

} // j__udyFreeJBBJP()


FUNCTION void j__udyFreeJBU(Word_t Pjbu, Pjpm_t Pjpm)
{
        int Words = sizeof(jbu_t) / cJU_BYTESPERWORD;

        MALLOCBITS_TEST(Pjbu);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJBU -= Words;
#endif // RAMMETRICS

        TRACE_FREE5("\n%p %8zd =  j__udyFreeJBU(), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjbu, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0);

        JudyFreeVirtual(Pjbu, Words);

} // j__udyFreeJBU()


FUNCTION void j__udyFreeJLL1(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF1POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL1 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL1(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL1()


FUNCTION void j__udyFreeJLL2(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF2POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL2 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL2(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL2()


FUNCTION void j__udyFreeJLL3(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF3POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL3 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL3(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL3()



FUNCTION void j__udyFreeJLL4(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF4POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL4 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL4(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);


} // j__udyFreeJLL4()


FUNCTION void j__udyFreeJLL5(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF5POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL5 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL5(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL5()


FUNCTION void j__udyFreeJLL6(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF6POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL6 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL6(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL6()


FUNCTION void j__udyFreeJLL7(Word_t Pjll, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JU_LEAF7POPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjll);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLL7 -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLL7(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjll, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjll, Words);

} // j__udyFreeJLL7()



// Note:  j__udyFreeJLLW() receives a root pointer with NO root pointer type
// bits present, that is, they are stripped by P_JLLW():

FUNCTION void j__udyFreeJLLW(Pjllw_t Pjllw, int Pop1, Pjpm_t Pjpm)
{
        int Words = JU_LEAFWPOPTOWORDS(Pop1);

        // MALLOCBITS_TEST(Pjllw_t, Pjllw);       // see above.

        if (Pjpm) Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLLW -= Words;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd =  j__udyFreeJLLW(%d), Words = %d, TotPop1 = %d\n", 
                (void *)Pjllw, j__udyMemSequence++, Pop1, (int)Words, Pop1 - 1);

        JudyFree((Word_t)Pjllw, Words);

} // j__udyFreeJLLW()


FUNCTION void j__udyFreeJLB1(Word_t Pjlb, Pjpm_t Pjpm)
{
        Word_t Words = sizeof(jlb_t) / cJU_BYTESPERWORD;

        MALLOCBITS_TEST(Pjlb);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJLB1 -= Words;
#endif // RAMMETRICS

        TRACE_FREE5("\n%p %8zd =  j__udyFreeJLB1(), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjlb, j__udyMemSequence++, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjlb, Words);

} // j__udyFreeJLB1()


#ifdef JUDYL

FUNCTION void j__udyLFreeJV(Word_t Pjv, int Pop1, Pjpm_t Pjpm)
{
        Word_t Words = JL_LEAFVPOPTOWORDS(Pop1);

        MALLOCBITS_TEST(Pjv);

        Pjpm->jpm_TotalMemWords -= Words;

#ifdef RAMMETRICS
        j__AllocWordsJV -= Words;
        j__NumbJV--;
#endif // RAMMETRICS

        TRACE_FREE6("\n%p %8zd = j__udyLFreeJV(%d), Words = %d, TotPop1 = %zd\n", 
                (void *)Pjv, j__udyMemSequence++, Pop1, (int)Words, Pjpm->jpm_Pop0);

        JudyFree(Pjv, Words);

} // j__udyLFreeJV()

#endif // JUDYL
