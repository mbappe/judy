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

// @(#) $Revision: 4.43 $ $Source: /judy/src/JudyCommon/JudyGet.c $
//
// Judy1Test() and JudyLGet() functions for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

#ifdef TRACEJPG                 // different macro name, for "retrieval" only.
#include "JudyPrintJP.c"
#endif

//void(ju_BranchPop0());            // Not used in Get

#ifdef  noDCD
#undef ju_DcdNonMatchKey
#define ju_DcdNonMatchKey(INDEX,PJP,POP0BYTES) (0)
#endif  // DCD

#include <immintrin.h> // _mm_movemask_epi8, __m128i

#ifdef  REFER
// _mm_loadu_si128 is SSE2
// _mm_lddqu_si128 is SSE3
// _mm_lddqu_si128 "may perform better than _mm_loadu_si128 when the data
// crosses a cache line boundary".

// v_t is a vector of 16 chars. __m128i is a vector of 2 long longs.
// We need the char variant so we can compare with a char using '==' or '>='.
#ifdef __clang__
// clang has some support for gcc attribute "vector_size" but it doesn't work
// as well as its own ext_vector_type.
// For example, it won't promote a scalar to a vector for compare.
typedef char __attribute__((ext_vector_type(16))) v_t;

// gcc has no support for clang attribute "ext_vector_type".
typedef char __attribute__((vector_size(16))) v_t;
// for mis-aligned moves  typedef char __attribute__((vector_size(16), aligned(4))) 
#endif // __clang__

// HasKey returns (1 << matching slot number) if sorted full Bucket
// has Key or zero if Bucket does not have Key.
// Keys are sorted with lowest key at vector index zero.
static int 
HasKey(v_t Bucket, uint8_t Key)
{
    v_t xEq = (Bucket == Key); // compare Key with all
    return _mm_movemask_epi8((__m128i)xEq); // (1 << matching slot) or 0
}

// HasKeyInPop returns (1 << matching slot number) if sorted partial Bucket
// has Key or zero if partial Bucket does not have Key.
// Keys are packed and sorted with lowest key at vector index zero.
static int
HasKeyInPop(v_t Bucket, char Key, int nPopCnt)
{
    return HasKey(Bucket, Key) & ((1 << nPopCnt) - 1);
}

// LocateKey returns the matching slot number if sorted full Bucket
// has Key or -1 if Bucket does not have Key.
// Keys are sorted with lowest key at vector index zero.
static int
LocateKey(v_t Bucket, char Key)
{
    // get (matching byte num + 1) or 0 if no match
    return __builtin_ffsll(HasKey(Bucket, Key)) - 1;
}

// LocateKeyInPop returns the matching slot number if sorted parital Bucket
// has Key or -1 if partial Bucket does not have Key.
// Keys are packed and sorted with lowest key at vector index zero.
static int
LocateKeyInPop(v_t Bucket, char Key, int nPopCnt)
{
    // get (matching byte num + 1) or 0 if no match
    return __builtin_ffsll(HasKeyInPop(Bucket, Key, nPopCnt)) - 1;
}

// HasGeKey returns (-1 << matching slot number) & 0xffff)
// if sorted full Bucket has a key that is greater than or equal to Key
// or zero if Bucket does not have such a key.
// Keys are sorted with lowest key at vector index zero.
static int
HasGeKey(v_t Bucket, char Key)
{
    v_t xGe = (Bucket >= Key); // compare Key with all
    return _mm_movemask_epi8((__m128i)xGe); // (1 << matching slot) or 0
}

// HasGeKeyInPop returns (-1 << matching slot number) & 0xffff)
// if sorted partial Bucket has a key that is greater than or equal to Key
// or zero if partial Bucket does not have such a key.
// Keys are packed and sorted with lowest key at vector index zero.
static int
HasGeKeyInPop(v_t Bucket, char Key, int nPopCnt)
{
    int n = HasGeKey(Bucket, Key);
    n &= (1 << nPopCnt) - 1;
    return n;
}

// Use LocateSlot if we know the bucket does not have the key
// but we need to know where the key belongs.
static int
LocateSlot(v_t Bucket, char Key)
{
    return (__builtin_ffsll(HasGeKey(Bucket, Key)) + 16) % 17;
}

// Use LocateSlotInPop if we know the bucket does not have the key
// but we need to know where the key belongs.
static int
LocateSlotInPop(v_t Bucket, char Key, int nPopCnt)
{
    return (__builtin_ffsll(HasGeKeyInPop(Bucket, Key, nPopCnt)) + 16) % 17;
}

// Search returns the matching slot number if sorted full Bucket
// has Key or ~(slot number of first slot with key greater than Key)
// if Bucket has such a key or ~16 if Bucket has no such key.
// Keys are sorted with lowest key at vector index zero.
static int
Search(v_t Bucket, char Key)
{
    // get (matching byte num + 1) or 0 if no match
    int n = __builtin_ffsll(HasGeKey(Bucket, Key));
    // 0 => ~16
    // 1 => 0 if there and ~0 if not there
    // 2 => 1 if there and ~1 if not there
    // 4 => 2 if there and ~2 if not there
    // 0x8000 => 15 if there and ~15 if not there
#ifdef METHOD_TEST // Has je with gcc, but short branch forward.
    int z = (n == 0);
    --n; n = z ? ~(int)sizeof(v_t) : n ^ ((Bucket[n] == Key) - 1);
#elif defined(METHOD_LS)
    int eq = __builtin_ffsll(HasKey(Bucket, Key));
    return eq ? eq - 1 : ~LocateSlot(Bucket, Key);
#elif defined(METHOD_DECR)
    int z = (n == 0); // key is greater than all others in the bucket
    n = (z - 1) & (n - 1); // slot or 0
    n ^= ((Bucket[n] == Key) - 1); // slot or ~slot or 0
    n ^= ~(z - 1) & nPopCnt; // slot or ~slot or ~nPopCnt
#else // METHOD_MASK // One instruction less than METHOD_DECR with gcc.
    int nz = (n != 0); // bucket has a greater or equal key
    --n; n ^= (Bucket[n & (sizeof(v_t) - 1)] == Key) - 1;
    n |= (nz - 1) & ~sizeof(v_t);
#endif
    return n;
}

// SearchInPop returns the matching slot number if sorted parital Bucket
// has Key or ~(slot number of first slot with key greater than Key)
// if partial Bucket has such a key or ~16 if partial Bucket has no such key.
// Keys are packed and sorted with lowest key at vector index zero.
static int
SearchInPop(v_t Bucket, char Key, int nPopCnt)
{
    // get (matching byte num + 1) or 0 if no match
    int n = __builtin_ffsll(HasGeKeyInPop(Bucket, Key, nPopCnt));
#ifdef METHOD_TEST // has je with gcc, but short branch forward
    int z = (n == 0);
    --n; n = z ? ~nPopCnt : n ^ ((Bucket[n] == Key) - 1);
#elif defined(METHOD_LS)
    int eq = __builtin_ffsll(HasKeyInPop(Bucket, Key, nPopCnt));
    return eq ? eq - 1 : ~LocateSlotInPop(Bucket, Key, nPopCnt);
#elif !defined(METHOD_DECR)
    int z = (n == 0); // key is greater than all others in the bucket
    n = (z - 1) & (n - 1); // slot or 0
    n ^= ((Bucket[n] == Key) - 1); // slot or ~slot or 0
    n ^= ~(z - 1) & nPopCnt; // slot or ~slot or ~nPopCnt
#else // METHOD_MASK // Doesn't work for LocateSlotInPop.
    int nz = (n != 0); // partial bucket has a greater or equal key
    --n; n ^= (Bucket[n & (sizeof(v_t) - 1)] == Key) - 1;
    n |= (nz - 1) & ~nPopCnt;
#endif

    return n;
}
#endif  // REFER


#ifdef __clang__
// clang has some support for gcc attribute "vector_size" but it doesn't work
// as well as its own ext_vector_type.
// For example, it won't promote a scalar to a vector for compare.
typedef char __attribute__((ext_vector_type(16))) v16qi_t;
typedef uint16_t __attribute__((ext_vector_type(8))) v8qi_t;
#else // __clang__
// gcc has no support for clang attribute "ext_vector_type".

#ifdef  USEMISSALIGNED
typedef char     __attribute__((vector_size(16), aligned(4))) v16qi_t;
typedef uint16_t __attribute__((vector_size(16), aligned(4)))  v8qi_t;
#else   // aligned method
typedef char     __attribute__((vector_size(16)))            v16qi_t;
typedef uint16_t __attribute__((vector_size(16)))             v8qi_t;
#endif  // aligned method

#endif // GCC ! __clang__

// ****************************************************************************
// J U D Y   1   T E S T
// J U D Y   L   G E T
//
// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.

// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.
#ifdef JUDY1
#ifdef JUDYGETINLINE
FUNCTION int j__udy1Test (Pcvoid_t PArray, // from which to retrieve.
                       Word_t Index,    // to retrieve.
                       PJError_t PJError        // optional, for returning error info.
        )
#else  // ! JUDYGETINLINE 
FUNCTION int Judy1Test (Pcvoid_t PArray, // from which to retrieve.
                         Word_t Index,    // to retrieve.
                         PJError_t PJError        // optional, for returning error info.
        )
#endif // ! JUDYGETINLINE 
#else /* JUDYL */
#ifdef JUDYGETINLINE
FUNCTION PPvoid_t j__udyLGet (Pcvoid_t PArray,     // from which to retrieve.
                Word_t Index,        // to retrieve.
                PJError_t PJError    // optional, for returning error info.
    )
#else  // ! JUDYGETINLINE 
FUNCTION PPvoid_t JudyLGet (Pcvoid_t PArray,     // from which to retrieve.
                Word_t Index,        // to retrieve.
                PJError_t PJError    // optional, for returning error info.
    )
#endif // ! JUDYGETINLINE 
#endif /* JUDYL */
{
    Pjp_t     Pjp;                      // current JP while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    Word_t    Pop1;                     // leaf population (number of indexes).
    Pjll_t    Pjll;                     // pointer to LeafL.

#ifdef  JUDYL
    Pjv_t     Pjv;
#endif  // JUDYL

    int       posidx;
    uint8_t   Digit = 0;                // byte just decoded from Index.

    (void) PJError;                     // no longer used

    if (PArray == (Pcvoid_t)NULL)  // empty array.
        goto NotFoundExit;

#ifdef  TRACEJPG
#ifdef JUDY1
//    printf("\nJudy1Test, Key = 0x%lx, Array Pop1 = %lu\n", 
//            (size_t)Index, (size_t)(JU_LEAFW_POP0(PArray) + 1));
#else /* JUDYL */
//    printf("\nJudyGet, Key = 0x%lx, Array Pop1 = %lu\n", 
//        (size_t)Index, (size_t)(JU_LEAFW_POP0(PArray) + 1));
#endif /* JUDYL */
#endif  // TRACEJPG

// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAFW_POP0(PArray) < cJU_LEAFW_MAXPOP1) // must be a LEAFW
        {
            Pjlw_t Pjlw = P_JLW(PArray);        // first word of leaf.

            Pop1   = Pjlw[0] + 1;

            if ((posidx = j__udySearchLeafW(Pjlw + 1, Pop1, Index)) < 0)
                goto NotFoundExit;              // no jump to middle of switch

#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (JL_LEAFWVALUEAREA(Pjlw, Pop1) + posidx));
#endif  // JUDYL

        }
        Pjpm = P_JPM(PArray);
        Pjp = &(Pjpm->jpm_JP);  // top branch is below JPM.

#ifdef  TRACEJPG
        j__udyIndex = Index;
        j__udyPopulation = Pjpm->jpm_Pop0;
#endif  // TRACEJPG

// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:

ContinueWalk:           // for going down one level; come here with Pjp set.

#ifdef TRACEJPG
        JudyPrintJP(Pjp, "g", __LINE__);
#endif  // TRACEJPG

        switch (ju_Type(Pjp))
        {

// Ensure the switch table starts at 0 for speed; otherwise more code is
// executed:

        case 0: goto ReturnCorrupt;     // save a little code.

// ****************************************************************************
// JPNULL*:
//
// Note:  These are legitimate in a BranchU (only) and do not constitute a
// fault.

        case cJU_JPNULL1:
        case cJU_JPNULL2:
        case cJU_JPNULL3:
        case cJU_JPNULL4:
        case cJU_JPNULL5:
        case cJU_JPNULL6:
        case cJU_JPNULL7:
        {
            break;
        }
// ****************************************************************************
// JPBRANCH_L*:
//
        case cJU_JPBRANCH_L2:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L3:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 3)) break;

            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L4:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L5:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L6:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L7:
        {
            // ju_DcdNonMatchKey() would be a no-op.
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchL;
        }
        case cJU_JPBRANCH_L:
        {
            Pjbl_t Pjbl;

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchLs; come here with Digit set:

JudyBranchL:
//            Pjbl = P_JBL(Pjp->jp_Addr0);
            Pjbl = P_JBL(ju_BaLPntr(Pjp));

            posidx = j__udySearchBranchL(Pjbl->jbl_Expanse, Pjbl->jbl_NumJPs, Digit);

            if (posidx < 0)
                break;

            Pjp = Pjbl->jbl_jp + posidx;
            goto ContinueWalk;
        }

#ifndef noB
// ****************************************************************************
// JPBRANCH_B*:
//  Note: Only one (1) JPBRANCH_B* can be any path in the walk down the tree,
//  therefore the next level must be a Leaf or Immed.

        case cJU_JPBRANCH_B2:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B3:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 3)) break;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchB;
        }
        case cJU_JPBRANCH_B4:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchB;
        }
        case cJU_JPBRANCH_B5:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchB;
        }
        case cJU_JPBRANCH_B6:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchB;
        }
        case cJU_JPBRANCH_B7:
        {
//            ju_DcdNonMatchKey() would be a no-op.
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchB;
        }
        case cJU_JPBRANCH_B:
        {
            Pjbb_t    Pjbb;
            Word_t    subexp;   // in bitmap, 0..7[3]
            BITMAPB_t BitMap;   // for one subexpanse.
            BITMAPB_t BitMsk;   // bit in BitMap for Indexs Digit.

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchBs; come here with Digit set:

JudyBranchB:
            Pjbb   = P_JBB(ju_BaLPntr(Pjp));
            subexp = Digit / cJU_BITSPERSUBEXPB;

            BitMap = JU_JBB_BITMAP(Pjbb, subexp);
            BitMsk = JU_BITPOSMASKB(Digit);

//          No JP in subexpanse for Index, implies not found:
            if ((BitMap & BitMsk) == 0) 
                break;

//          start address of Pjp_t array
            Pjp    = P_JP(JU_JBB_PJP(Pjbb, subexp));

            Pjp += j__udyCountBitsB(BitMap & (BitMsk - 1));
            goto ContinueWalk;

        } // case cJU_JPBRANCH_B*
#endif  // noB
// ****************************************************************************
// JPBRANCH_U*:

        case cJU_JPBRANCH_U:
        {
            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U7:
        {
//          ju_DcdNonMatchKey() would be a no-op.

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 7);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U6:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 6)) break;

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 6);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U5:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 5)) break;

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 5);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U4:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 4)) break;

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 4);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U3:
        {

            if (ju_DcdNonMatchKey(Index, Pjp, 3)) break;

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 3);
            goto ContinueWalk;
        }
        case cJU_JPBRANCH_U2:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 2)) break;

            Pjp =  P_JBU(ju_BaLPntr(Pjp))->jbu_jp + JU_DIGITATSTATE(Index, 2);
            goto ContinueWalk;
        }
// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of ju_DcdNonMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.

       // Judy1 does not have Leaf1
#ifdef  JUDYL     
        case cJU_JPLEAF1:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 1)) break;

            Pjll = P_JLL(ju_BaLPntr(Pjp));
            Pop1   = ju_LeafPop0(Pjp) + 1;

            Pjv  = JL_LEAF1VALUEAREA(Pjll, Pop1);

#ifdef  PARALLEL
            v16qi_t     m128reg;
            v16qi_t    *Pv16qi;

            DIRECTHITS;

//          This seems a little hokey
            Pv16qi = (v16qi_t *)ju_BaLPntr(Pjp);

            uint8_t  key8 = (uint8_t)Index;
            Word_t   charmask = 0;

// for performance testing only!!!!!!!, plus xxxns on i7-6800K
#ifdef  EXPPARALLEL
//          This supports a Leaf1 Pop1 up to 64
            int   bucket = 0;
            for (int pop0 = 0; pop0 < Pop1; pop0 += 16, bucket++)
            {
                m128reg   = Pv16qi[bucket] == key8;      // compare 16 more Keys with key8
                charmask |=  (Word_t)_mm_movemask_epi8((__m128i)m128reg) << pop0;
            }
//          because shift by 64 is undefined -- 2 steps
            charmask &= (((Word_t)1 << (Pop1 - 1)) << 1) - 1;
            if (charmask == 0)
                break;
#else  // EXPPARALLEL

//          This supports a Leaf1 Pop1 up to 32
            m128reg  =  Pv16qi[0] == key8;      // compare 16 Keys with key8
            charmask =  (Word_t)_mm_movemask_epi8((__m128i)m128reg);

//            if (Pop1 > 16) a little slower (dlb - Oct2017)
//            {
            m128reg  =  Pv16qi[1] == key8;      // compare 16 Keys with key8
            charmask |= (Word_t)_mm_movemask_epi8((__m128i)m128reg) << 16;
//            }

//          mask off bits beyond population
            charmask &= ((Word_t)1 << Pop1) - 1;
//          need 2 step shift when Pop1 == 64 charmask &= (((Word_t)1 << (Pop1 - 1)) << 1) - 1;
//            charmask &= (((Word_t)1 << (Pop1 - 1)) << 1) - 1;
            if (charmask == 0)          // no matching Keys
                break;
            posidx = __builtin_ctzll(charmask);
            return((PPvoid_t) (Pjv + posidx));

#endif // EXPPARALLEL

            posidx = __builtin_ctzll(charmask); 

//            if (posidx   != __builtin_ffsll(charmask) - 1)
//                printf(" Value = 0x%lx, Index = 0x%lx\n", *(Pjv + posidx), Index);

            return((PPvoid_t) (Pjv + posidx));

#else   // ! PARALLEL

//          entry Pjll = Leaf1, Pjv = PValue, Pop1 = population, Key = Index
            posidx = j__udySearchLeaf1(Pjll, Pop1, Index);
            goto CommonLeafExit;
#endif  // ! PARALLEL

        }
#endif  // JUDYL

        case cJU_JPLEAF2:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 2)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));

// entry Pjll = Leaf2, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf2(Pjll, Pop1, Index);
            if (posidx < 0) 
                break;

#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll, Pop1);
            return((PPvoid_t) (Pjv + posidx));
#else   // JUDY1
            return(1);
#endif  // JUDY1

        }
        case cJU_JPLEAF3:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 3)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf3Exit;

Leaf3Exit:     // entry Pjll = Leaf3, Pjv = Value, Pop1 = population

            posidx = j__udySearchLeaf3(Pjll, Pop1, Index);
            goto CommonLeafExit;
CommonLeafExit:
            if (posidx < 0) 
                break;
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL

        }
        case cJU_JPLEAF4:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 4)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));

#ifdef  JUDYL
            Pjv = JL_LEAF4VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf4Exit;
            
Leaf4Exit:
            posidx = j__udySearchLeaf4(Pjll, Pop1, Index);
            goto CommonLeafExit;
        }
        case cJU_JPLEAF5:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 5)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));

#ifdef  JUDYL
            Pjv = JL_LEAF5VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf5Exit;
            
Leaf5Exit:
            posidx = j__udySearchLeaf5(Pjll, Pop1, Index);
            goto CommonLeafExit;
        }
        case cJU_JPLEAF6:
        {
            if (ju_DcdNonMatchKey(Index, Pjp, 6)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));

#ifdef  JUDYL
            Pjv = JL_LEAF6VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf6Exit;
            
Leaf6Exit:
            posidx = j__udySearchLeaf6(Pjll, Pop1, Index);
            goto CommonLeafExit;
        }
        case cJU_JPLEAF7:
        {
            // ju_DcdNonMatchKey() would be a no-op.

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(ju_BaLPntr(Pjp));

#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf7Exit;
            
Leaf7Exit:
            posidx = j__udySearchLeaf7(Pjll, Pop1, Index);
            goto CommonLeafExit;
        }


// ****************************************************************************
// JPLEAF_B1:

        case cJU_JPLEAF_B1:
        {
            Pjlb_t    Pjlb;
            Word_t    subexp;   // in bitmap, 0..7.
            BITMAPL_t BitMap;   // for one subexpanse.
            BITMAPL_t BitMsk;   // bit in BitMap for Indexs Digit.

#ifdef JUDYL
            Word_t      PjvRaw;
#endif  // JUDYL

            if (ju_DcdNonMatchKey(Index, Pjp, 1)) break;

            Pjlb   = P_JLB(ju_BaLPntr(Pjp));
            Digit  = JU_DIGITATSTATE(Index, 1);
            subexp = Digit / cJU_BITSPERSUBEXPL;

#ifdef EXP1BIT
            if (subexp == 0) return(1); // 7% faster
#endif  // EXP1BIT

            BitMap = JU_JLB_BITMAP(Pjlb, subexp);
            BitMsk = JU_BITPOSMASKL(Digit);

// No value in subexpanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;

#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
// Count value areas in the subexpanse below the one for Index:
// JudyL is much more complicated because of Value area subarrays:

#ifdef BMVALUE
//          if population of subexpanse == 1, return ^ to Value^
            if (BitMap == BitMsk)
            {
//////            if (ju_DcdNonMatchKey(Index, Pjp, 1)) break;
                return((PPvoid_t)(&Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw));
            }
#endif /* BMVALUE */

//          Get raw pointer to Value area
            PjvRaw = Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw;

#ifdef  UNCOMPVALUE
            if (PjvRaw & 1)     // check if uncompressed Value area
            {
                posidx = Digit % cJU_BITSPERSUBEXPL; // offset from Index
// done by P_JV()  PjvRaw -= 1;       // mask off least bit
            }
            else                                    // Compressed, calc offset
#endif  // UNCOMPVALUE
            {
                posidx = j__udyCountBitsL(BitMap & (BitMsk - 1));
            }
            return((PPvoid_t) (P_JV(PjvRaw) + posidx));
#endif // JUDYL

        } // case cJU_JPLEAF_B1

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
            return(1);
        }
#endif // JUDY1

// ****************************************************************************
// JPIMMED*:
//
// Note that the contents of jp_DcdPopO is different for cJU_JPIMMED_*_01:

        case cJU_JPIMMED_1_01:          // 1 byte decode
        case cJU_JPIMMED_2_01:          // 2 byte decode
        case cJU_JPIMMED_3_01:          // 3 byte decode
        case cJU_JPIMMED_4_01:          // 4 byte decode
        case cJU_JPIMMED_5_01:          // 5 byte decode
        case cJU_JPIMMED_6_01:          // 6 byte decode
        case cJU_JPIMMED_7_01:          // 7 byte decode

//            if (JU_JPDCDPOP0(Pjp) != JU_TRIMTODCDSIZE(Index)) 
//            if (JU_TRIMTODCDSIZE(Index ^ ju_DcdPop0(Pjp)) != 0) 
//            if (ju_DcdPop0(Pjp) != JU_TRIMTODCDSIZE(Index)) 

//          This version does not have an conditional branch
            if ((ju_DcdPop0(Pjp) ^ Index) << 8) 
                break;

#ifdef  JUDY1
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
//        return((PPvoid_t) &(Pjp->jp_ValueI));  // ^ immediate value
        return((PPvoid_t) ju_PImmVal_01(Pjp));  // ^ to immediate Value
#endif  // JUDYL

#ifdef  JUDY1
        case cJ1_JPIMMED_1_15:
        case cJ1_JPIMMED_1_14:
        case cJ1_JPIMMED_1_13:
        case cJ1_JPIMMED_1_12:
        case cJ1_JPIMMED_1_11:
        case cJ1_JPIMMED_1_10:
        case cJ1_JPIMMED_1_09:
        case cJ1_JPIMMED_1_08:
#endif  // JUDY1

        case cJU_JPIMMED_1_07:
        case cJU_JPIMMED_1_06:
        case cJU_JPIMMED_1_05:
        case cJU_JPIMMED_1_04:
        case cJU_JPIMMED_1_03:
        case cJU_JPIMMED_1_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;

#ifdef  PARALLEL
            DIRECTHITS;

//          posidx = 16 bits where each char match is a bit set
//            v_t m128 = (*(v_t *)Pjp == (uint8_t)Index); // compare Key with all
//    return _mm_movemask_epi8((__m128i)m128); // (1 << matching slot) or 0
//
//            posidx = HasKey(*(v_t *)Pjp, (char)Index); // check equal 16 bytes

//typedef    char __attribute__((vector_size(16))) v_t;
//typedef uint8_t __attribute__((vector_size(16))) m128_t;
//            m128_t m128reg;
//
//typedef unsigned char __attribute__((vector_size(16))) __v16qu;
//typedef   uint8_t __attribute__((__vector_size__ (16))) __v16qu;
//(v_t)_mm_loadu_si128((__m128i *)pUaBucket)

            v16qi_t m128reg;
            Word_t  charmask = 0;
            uint8_t key8 = (uint8_t)Index;

//          Do a parallel search of 16 chars
            m128reg = (*(v16qi_t *)Pjp) == key8; 
            charmask = _mm_movemask_epi8((__m128i)m128reg);

#ifdef  JUDYL
            charmask >>= 8;                     // skip 1st word in jp_t
            Pjv = P_JV(ju_PImmVals(Pjp));       // ^ immediate values area
#endif  // JUDYL

//          Mask off bits that are beyond population (assume stats with 1st word)
            charmask &= ((Word_t)1 << Pop1) - 1; 

            if (charmask == 0)                  // not found
                break;
#ifdef  JUDYL
            posidx = __builtin_ctzll(charmask);
            return((PPvoid_t) (Pjv + posidx));
#else   // JUDY1
            return(1);
#endif  // JUDY1

#else   // ! PARALLEL

            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            posidx = j__udySearchLeaf1(Pjll, Pop1, Index);
            if (posidx < 0) 
                break;
#ifdef  JUDYL
            Pjv = P_JV(ju_PImmVals(Pjp));  // ^ immediate values area
            return((PPvoid_t) (Pjv + posidx));
#else   // JUDY1
            return(1);
#endif  // JUDY1

#endif  // ! PARALLEL

        }
#ifdef  JUDY1
        case cJ1_JPIMMED_2_07:
        case cJ1_JPIMMED_2_06:
        case cJ1_JPIMMED_2_05:
        case cJ1_JPIMMED_2_04:
#endif  // JUDY1

        case cJU_JPIMMED_2_03:
        case cJU_JPIMMED_2_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;

            Pjll = (Pjll_t)ju_PImmed2(Pjp);

#ifdef  JUDYL
            Pjv = P_JV(ju_PImmVals(Pjp));  // ^ immediate values area
#endif  // JUDYL

            posidx = j__udySearchLeaf2(Pjll, Pop1, Index);
            goto CommonLeafExit;
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_3_05: 
        case cJ1_JPIMMED_3_04:
        case cJ1_JPIMMED_3_03:
#endif  // JUDY1

        case cJU_JPIMMED_3_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);

#ifdef  JUDYL
//            Pjv = P_JV(Pjp->jp_PValue);
            Pjv = P_JV(ju_PImmVals(Pjp));  // ^ immediate values area
#endif  // JUDYL

            goto Leaf3Exit;
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_4_03:
        case cJ1_JPIMMED_4_02:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
//            Pjll = (Pjll_t)Pjp->jp_1Index4;
            Pjll = (Pjll_t)ju_PImmed4(Pjp);
            goto Leaf4Exit;
        }
        case cJ1_JPIMMED_5_03:
        case cJ1_JPIMMED_5_02:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
//            Pjll = (Pjll_t)Pjp->jp_1Index1;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf5Exit;
        }
        case cJ1_JPIMMED_6_02:
        {
            Pop1 = 2;
//            Pjll = (Pjll_t)Pjp->jp_1Index1;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf6Exit;
        }
        case cJ1_JPIMMED_7_02:
        {
            Pop1 = 2;
//            Pjll = (Pjll_t)Pjp->jp_1Index1;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf7Exit;
        }
#endif  // JUDY1


// ****************************************************************************
// INVALID JP TYPE:

        default:

ReturnCorrupt:
            break;

//            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
//            JUDY1CODE(return(JERRI );)
//            JUDYLCODE(return(PPJERR);)

        } // switch on JP type

NotFoundExit:

#ifdef  JUDY1
            return(0);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) NULL);
#endif  // JUDYL

} // Judy1Test() / JudyLGet()
