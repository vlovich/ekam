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

#ifndef KENTONSCODE_EKAM_ACTION_H_
#define KENTONSCODE_EKAM_ACTION_H_

#include <string>
#include <vector>
#include <sys/types.h>

#include "base/OwnedPtr.h"
#include "os/File.h"
#include "Tag.h"
#include "os/EventManager.h"

namespace ekam {

class ActionFactory;

class ProcessExitCallback {
public:
  virtual ~ProcessExitCallback();

  // Negative = signal number.
  virtual void done(int exit_status) = 0;
};

enum class Priority: uint8_t {
  // Lower number indicates it should be evaluated first.
  // So we want to learn all the rules first before we do anything else. Then we want to perform
  // all the code gen actions (to avoid needlessly compiling files before the code gen has even
  // taken place).
  // NOTE: Since rules are run in parallel it's entirely possible we may end up evaluating
  // out-of-order. That should be OK since the rest of Ekam is built to handle this without a
  // problem. This priority should only reduce the overhead of Ekam, not impact correctness in any
  // way. Only "CodeGen" and "Compilation" are available for ekam rules to specify dynamically.
  // Everything else is marked automatically within Ekam.
  // This is a *very* coarse heuristic intended to improve the 0-knowledge situation (first launch
  // of Ekam). A more optimal optimization would be to dump the DAG that Ekam discovers during
  // runtime so that subsequent invocations can make even more optimal use (e.g. it's not hard to
  // conceive a scenario where a host compilation is needed to generate code that's used in
  // another host compilation, ad infinitum, making this heuristic not as beneficial in such
  // use-cases).
  Rules = 0,
  HostCompilation, // This is needed to build tools needed for codegen.
  HostLink,
  // As with Link below, this ensures we only try to link host binaries after everything is
  // compiled.
  CodeGen, // This is needed to generate code needed for compilation.
  Compilation,
  Link,
  // No sense trying to link anything that we might be missing object files for. This does have an
  // unfortunate side-effect that
  EverythingElse, // Basically tests. Anything else?
};
static constexpr uint8_t NumPriorities = static_cast<uint8_t>(Priority::EverythingElse) + 1;

class BuildContext {
public:
  virtual ~BuildContext() noexcept(false);

  virtual File* findProvider(Tag id) = 0;
  virtual File* findInput(const std::string& path) = 0;

  enum InstallLocation {
    BIN,
    LIB,
    NODE_MODULES
  };
  static const int INSTALL_LOCATION_COUNT = 3;

  static const char* const INSTALL_LOCATION_NAMES[INSTALL_LOCATION_COUNT];

  virtual void provide(File* file, const std::vector<Tag>& tags) = 0;
  virtual void install(File* file, InstallLocation location, const std::string& name) = 0;
  virtual void log(const std::string& text) = 0;

  virtual OwnedPtr<File> newOutput(const std::string& path) = 0;

  virtual void addActionType(OwnedPtr<ActionFactory> factory) = 0;

  virtual void passed() = 0;
  virtual void failed() = 0;
};

class Action {
public:
  virtual ~Action();

  virtual bool isSilent() { return false; }
  virtual std::string getVerb() = 0;
  virtual Promise<void> start(EventManager* eventManager, BuildContext* context) = 0;
};

class ActionFactory {
public:
  virtual ~ActionFactory();

  virtual void enumerateTriggerTags(std::back_insert_iterator<std::vector<Tag> > iter) = 0;
  virtual OwnedPtr<Action> tryMakeAction(const Tag& id, File* file) = 0;
  virtual Priority getPriority() = 0;
};

}  // namespace ekam

#endif  // KENTONSCODE_EKAM_ACTION_H_
