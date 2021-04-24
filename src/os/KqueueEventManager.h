// Ekam Build System
// Author: Kenton Varda (kenton@sandstorm.io)
// Copyright (c) 2010-2015 Kenton Varda, Google Inc., and contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef KENTONSCODE_OS_KQUEUEEVENTMANAGER_H_
#define KENTONSCODE_OS_KQUEUEEVENTMANAGER_H_

#include <sys/types.h>
#include <unordered_set>
#include <deque>

#include "EventManager.h"
#include "base/OwnedPtr.h"

typedef struct kevent KEvent;

namespace ekam {

class KqueueEventManager final: public RunnableEventManager {
public:
  KqueueEventManager();
  ~KqueueEventManager();

  // implements RunnableEventManager -----------------------------------------------------
  void loop() override;

  // implements EventManager -------------------------------------------------------------
  OwnedPtr<AsyncOperation> runAsynchronously(Callback* callback) override;
  OwnedPtr<AsyncOperation> onProcessExit(pid_t pid, ProcessExitCallback* callback) override;
  OwnedPtr<AsyncOperation> onReadable(int fd, IoCallback* callback) override;
  OwnedPtr<AsyncOperation> onWritable(int fd, IoCallback* callback) override;
  OwnedPtr<AsyncOperation> onFileChange(const std::string& filename, FileChangeCallback* callback) override;

private:
  class KEventHandler;

  class AsyncCallbackHandler;
  class ProcessExitHandler;
  class ReadHandler;
  class WriteHandler;
  class FileChangeHandler;

  struct IntptrShortPairHash {
    inline bool operator()(const std::pair<intptr_t, short>& p) const {
      return p.first * 65537 + p.second;
    }
  };

  int kqueueFd;

  std::deque<AsyncCallbackHandler*> asyncCallbacks;
  std::deque<KEvent> fakeEvents;
  int handlerCount;

  bool handleEvent();

  void updateKqueue(const KEvent& event);
  void updateKqueue(uintptr_t ident, short filter, u_short flags,
                    KEventHandler* handler = NULL, u_int fflags = 0, intptr_t data = 0);
};

}  // namespace ekam

#endif  // KENTONSCODE_OS_KQUEUEEVENTMANAGER_H_
