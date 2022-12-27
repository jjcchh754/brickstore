/* Copyright (C) 2004-2022 Robert Griebl. All rights reserved.
**
** This file is part of BrickStore.
**
** This file may be distributed and/or modified under the terms of the GNU
** General Public License version 2 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.
*/

// The parallel merge sort algorithm was derived from Markus Weissmann's
// BSD licensed "pmsort" (originally implemented in C and pthreads):

/*_
 * Copyright (c) 2006, 2007, Markus W. Weissmann <mail@mweissmann.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of OpenDarwin nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: $
 *
 */
#pragma once

#include <QtConcurrentRun>


static constexpr qsizetype MinParallelSpan = 500;

template <typename RandomAccessIterator, typename T, typename LessThan>
void qParallelSortThread(RandomAccessIterator begin, RandomAccessIterator end, LessThan lessThan,
                         T *tmp, int threadCount)
{
    qsizetype span = end - begin;

    // stop if there are no threads left OR if size of array is at most MinParallelSpan (to avoid overhead)
    if ((threadCount == 1) || (span <= MinParallelSpan)) {
        // let's just sort this in serial
        std::sort(begin, end, lessThan);
    } else {
        // divide & conquer
        qsizetype a_span = (span + 1) / 2; // bigger half of array if array-size is uneven
        qsizetype b_span = span - a_span;  // smaller half of array if array-size is uneven

        QFuture<void> future = QtConcurrent::run(qParallelSortThread<RandomAccessIterator, T, LessThan>,
                                                 begin + a_span, end, lessThan, tmp + a_span, threadCount / 2);
        qParallelSortThread(begin, begin + a_span, lessThan, tmp, (threadCount + 1) / 2);
        future.waitForFinished();

        RandomAccessIterator a = begin;
        RandomAccessIterator b = begin + a_span;

        Q_ASSERT(span == (end - begin));
        Q_ASSERT((b + b_span) == end);

        for (qsizetype i = 0; i < span; ++i) {
            // 'a' if no b left or a < b
            // 'b' if no a left or a >= b
            if (!b_span || (a_span && b_span && lessThan(*a, *b))) {
                *(tmp + i) = *a;
                a++;
                a_span--;
            } else {
                *(tmp + i) = *b;
                b++;
                b_span--;
            }
        }
        std::copy(begin, end, tmp);
    }
}

template <typename RandomAccessIterator, typename LessThan>
inline void qParallelSortImpl(RandomAccessIterator begin, RandomAccessIterator end, LessThan lessThan)
{
    const qsizetype span = end - begin;
    if (span >= 2) {
        int threadCount = QThread::idealThreadCount();
        if ((threadCount == 1) || (span <= MinParallelSpan)) {
            std::sort(begin, end, lessThan);
        } else {
            auto *tmp = new typename std::iterator_traits<RandomAccessIterator>::value_type[span];
            qParallelSortThread(begin, end, lessThan, tmp, threadCount);
            delete [] tmp;
        }
    }
}


#if defined(BS_HAS_PARALLEL_STL) && __has_include(<execution>)
#  if defined(emit)
#    undef emit // libtbb uses a function named emit()
#  endif
#  include <execution>
#  if (__cpp_lib_execution >= 201603) && (__cpp_lib_parallel_algorithm >= 201603)
#    define BS_HAS_PARALLEL_STL_EXECUTION
#  endif
#  if !defined(QT_NO_EMIT)
#    define emit
#  endif
#endif

#ifdef BS_HAS_PARALLEL_STL_EXECUTION
#include <algorithm>

template <class IT, class LT>
inline constexpr void qParallelSort(const IT begin, const IT end, LT lt)
{
    std::sort(std::execution::par_unseq, begin, end, lt);
}

#else

template <class IT, class LT>
inline constexpr void qParallelSort(const IT begin, const IT end, LT lt)
{
    qParallelSortImpl(begin, end, lt);
}

#endif // BS_HAS_PARALLEL_STL_EXECUTION


#ifdef QPARALLELSORT_TESTING // benchmarking
#include "stopwatch.h"

static void test_par_sort()
{
    for (int k = 0; k < 3; ++k) {
        const char *which = (k == 0 ? "std::sort           " :
                            (k == 1 ? "std::sort par_unseq ":
                                      "Parallel Sort       "));
        int maxloop = 24;
        int *src = new int[1 << maxloop];
        for (int j = 0; j < (1 << maxloop); ++j)
            src[j] = rand();

        for (int i = 0; i <= maxloop; ++i) {
            size_t count = 1 << i;
            QByteArray msg = u"%3 test run %1 ... %2 values"_qs.arg(i).arg(count)
                    .arg(QString::fromLatin1(which)).toLatin1();
            int *array = new int[count];
            memcpy(array, src, sizeof(int) * count);

            {
                stopwatch sw(msg.constData());
                if (k == 0)
                    std::sort(array, array + count);
                else if (k == 1)
                    std::sort(std::execution::par_unseq, array, array + count);
                else
                    qParallelSortImpl(array, array + count, std::less<int>());
            }
            delete [] array;
        }
        delete [] src;
    }
}
#  endif

