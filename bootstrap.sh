#! /bin/sh
#
# ekam -- http://code.google.com/p/ekam
# Copyright (c) 2010 Kenton Varda and contributors.  All rights reserved.
# Portions copyright Google, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of the ekam project nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

echo "This script builds a basic Ekam binary using a single massive compiler"
echo "invocation, then rebuilds Ekam using Ekam itself."

SOURCES=$(ls src/*.cpp src/*/*.cpp |
    grep -v KqueueEventManager | grep -v PollEventManager |
    grep -v ProtoDashboard | grep -v ekam-client | grep -v _test)

set -e

echo "*************************************************"
echo "Building using one massive compile..."
echo "*************************************************"

echo \$ g++ -Isrc -std=gnu++0x $SOURCES -o bootstrap-ekam
g++ -Isrc -std=gnu++0x $SOURCES -o bootstrap-ekam

echo "*************************************************"
echo "Building again using Ekam..."
echo "*************************************************"

if test -e tmp/ekam; then
  rm -f tmp/ekam
fi

echo \$ ./bootstrap-ekam -j4
./bootstrap-ekam -j4

echo "*************************************************"
if test -e bin/ekam; then
  echo "SUCCESS: output is at: bin/ekam"
  echo "IGNORE ALL ERRORS ABOVE.  Some errors are expected depending"
  echo "on your OS and whether or not you have the protobuf source code"
  echo "in your source tree."
	rm bootstrap-ekam
else
  echo "FAILED"
  exit 1
fi
echo "*************************************************"
