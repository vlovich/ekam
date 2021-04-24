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

#ifndef KENTONSCODE_EKAM_SIMPLEDASHBOARD_H_
#define KENTONSCODE_EKAM_SIMPLEDASHBOARD_H_

#include <stdio.h>

#include "Dashboard.h"

namespace ekam {

class SimpleDashboard final : public Dashboard {
public:
  SimpleDashboard(FILE* outputStream);
  ~SimpleDashboard();

  // implements Dashboard ----------------------------------------------------------------
  OwnedPtr<Task> beginTask(const std::string& verb, const std::string& noun, Silence silence) override;

private:
  class TaskImpl;

  FILE* outputStream;
};

}  // namespace ekam

#endif  // KENTONSCODE_EKAM_SIMPLEDASHBOARD_H_
