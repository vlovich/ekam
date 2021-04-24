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

#ifndef KENTONSCODE_OS_DISKFILE_H_
#define KENTONSCODE_OS_DISKFILE_H_

#include "File.h"
#include <string>

namespace ekam {

class DiskFile final: public File {
public:
  DiskFile(const std::string& path, File* parent);
  ~DiskFile();

  // implements File ---------------------------------------------------------------------
  std::string basename() override;
  std::string canonicalName() override;
  OwnedPtr<File> clone() override;
  bool hasParent() override;
  OwnedPtr<File> parent() override;

  bool equals(File* other) override;
  size_t identityHash() override;

  OwnedPtr<DiskRef> getOnDisk(Usage usage) override;

  bool exists() override;
  bool isFile() override;
  bool isDirectory() override;

  // File only.
  Hash contentHash() override;
  std::string readAll() override;
  void writeAll(const std::string& content) override;
  void writeAll(const void* data, int size) override;

  // Directory only.
  void list(OwnedPtrVector<File>::Appender output) override;
  OwnedPtr<File> relative(const std::string& path) override;

  // Methods that create or delete objects.
  void createDirectory() override;
  void link(File* target) override;
  void unlink() override;

private:
  class DiskRefImpl;

  std::string path;
  OwnedPtr<File> parentRef;
};

}  // namespace ekam

#endif  // KENTONSCODE_OS_DISKFILE_H_
