/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "osquery/dispatcher/dispatcher.h"
#include "osquery/events/kernel/circular_queue_user.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <benchmark/benchmark.h>

#include <boost/make_shared.hpp>

namespace osquery {

#ifdef KERNEL_TEST

static void CommunicationBenchmark(benchmark::State &state) {
  if (state.thread_index == 0) {
    CQueue queue(8 * (1 << 20));

    osquery_event_t event;
    osquery::CQueue::event *event_buf = nullptr;
    int drops = 0;
    size_t reads0 = 0;
    size_t reads1 = 0;
    size_t syncs = 0;
    int max_before_sync = 0;
    while (state.KeepRunning()) {
      drops += queue.kernelSync(OSQUERY_NO_BLOCK);
      syncs++;
      max_before_sync = 2000;
      while (max_before_sync > 0 && (event = queue.dequeue(&event_buf))) {
        switch (event) {
        case OSQUERY_TEST_EVENT_0:
          reads0++;
          max_before_sync--;
          break;
        case OSQUERY_TEST_EVENT_1:
          reads1++;
          max_before_sync--;
          break;
        default:
          break;
        }
      }
    }

    state.SetBytesProcessed(reads0 * sizeof(test_event_0_data_t) +
                            reads1 * sizeof(test_event_1_data_t) +
                            (reads0 + reads1) * sizeof(osquery_event_time_t));
    state.SetItemsProcessed(reads0 + reads1);
    std::string s = std::string("dropped: ") + std::to_string(drops) +
                    "  syncs: " + std::to_string(syncs);
    state.SetLabel(s);
  } else {
    const char *filename = "/dev/osquery";
    int fd = open(filename, O_RDWR);
    int type = state.thread_index % 2;
    while (state.KeepRunning()) {
      ioctl(fd, OSQUERY_IOCTL_TEST, &type);
    }
    close(fd);
  }
}

BENCHMARK(CommunicationBenchmark)->UseRealTime()->ThreadRange(2, 32);

#endif // KERNEL_TEST
}
