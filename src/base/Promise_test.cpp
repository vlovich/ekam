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

#include "Promise.h"
#include <stdio.h>
#include <stdlib.h>
#include <kj/compat/gtest.h>

namespace ekam {
namespace {

class MockExecutor final: public Executor {
private:
  class PendingRunnableImpl : public PendingRunnable {
  public:
    PendingRunnableImpl(MockExecutor* executor, OwnedPtr<Runnable> runnable)
        : executor(executor), runnable(runnable.release()) {}

    ~PendingRunnableImpl() {
      if (runnable != nullptr) {
        for (auto iter = executor->queue.begin(); iter != executor->queue.end(); ++iter) {
          if (*iter == this) {
            executor->queue.erase(iter);
            break;
          }
        }
      }
    }

    void run() {
      runnable.release()->run();
    }

  private:
    MockExecutor* executor;
    OwnedPtr<Runnable> runnable;
  };

public:
  MockExecutor() {}
  ~MockExecutor() {
    EXPECT_TRUE(queue.empty());
  }

  void runNext() {
    ASSERT_FALSE(queue.empty());
    auto front = queue.front();
    queue.pop_front();
    front->run();
  }

  bool empty() {
    return queue.empty();
  }

  // implements Executor -----------------------------------------------------------------
  OwnedPtr<PendingRunnable> runLater(OwnedPtr<Runnable> runnable) override {
    auto result = newOwned<PendingRunnableImpl>(this, runnable.release());
    queue.push_back(result.get());
    return result.release();
  }

private:
  std::deque<PendingRunnableImpl*> queue;
};

template <typename T>
class MockPromiseFulfiller: public PromiseFulfiller<T> {
public:
  typedef typename PromiseFulfiller<T>::Callback Callback;
  MockPromiseFulfiller(Callback* callback, Callback** callbackPtr)
      : callbackPtr(callbackPtr) {
    *callbackPtr = callback;
  }
  ~MockPromiseFulfiller() {
    *callbackPtr = nullptr;
  }

private:
  Callback** callbackPtr;
};

TEST(PromiseTest, Basic) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller;
  auto promise = newPromise<MockPromiseFulfiller<int>>(&fulfiller);

  bool triggered = false;

  Promise<int> promise2 = mockExecutor.when(promise)(
    [&triggered](int i) -> int {
      EXPECT_EQ(5, i);
      triggered = true;
      return 123;
    });

  EXPECT_FALSE(triggered);

  fulfiller->fulfill(5);

  EXPECT_FALSE(triggered);

  mockExecutor.runNext();

  EXPECT_TRUE(triggered);

  // Fulfiller deleted because promise has been consumed.
  EXPECT_TRUE(fulfiller == nullptr);
}

TEST(PromiseTest, PreFulfilled) {
  MockExecutor mockExecutor;

  auto promise = newFulfilledPromise(5);

  bool triggered = false;

  Promise<int> promise2 = mockExecutor.when(promise)(
    [&triggered](int i) -> int {
      EXPECT_EQ(5, i);
      triggered = true;
      return 123;
    });

  EXPECT_FALSE(triggered);

  mockExecutor.runNext();

  EXPECT_TRUE(triggered);
}

TEST(PromiseTest, Dependent) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<int>>(&fulfiller1);
  PromiseFulfiller<int>::Callback* fulfiller2;
  auto promise2 = newPromise<MockPromiseFulfiller<int>>(&fulfiller2);

  Promise<int> promise3 = mockExecutor.when(promise1, promise2)(
    [](int a, int b) -> int {
      return a + b;
    });

  int result = 0;

  Promise<void> promise4 = mockExecutor.when(promise3)(
    [&result](int a) {
      result = a;
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller1->fulfill(12);
  EXPECT_TRUE(mockExecutor.empty());
  fulfiller2->fulfill(34);
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_EQ(result, 46);
}

TEST(PromiseTest, Chained) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<int>>(&fulfiller1);
  PromiseFulfiller<int>::Callback* fulfiller2;
  auto promise2 = newPromise<MockPromiseFulfiller<int>>(&fulfiller2);

  int result = 0;

  Promise<void> promise3 = mockExecutor.when(promise2)(
    [&result](int a) {
      result = a;
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller2->fulfill(promise1.release());
  EXPECT_TRUE(mockExecutor.empty());
  EXPECT_EQ(0, result);
  fulfiller1->fulfill(123);
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_EQ(result, 123);
}

TEST(PromiseTest, ChainedVoid) {
  MockExecutor mockExecutor;

  PromiseFulfiller<void>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<void>>(&fulfiller1);
  PromiseFulfiller<void>::Callback* fulfiller2;
  auto promise2 = newPromise<MockPromiseFulfiller<void>>(&fulfiller2);

  bool triggered = false;

  Promise<void> promise3 = mockExecutor.when(promise2)(
    [&triggered](Void) {
      triggered = true;
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller2->fulfill(promise1.release());
  EXPECT_TRUE(mockExecutor.empty());
  EXPECT_FALSE(triggered);
  fulfiller1->fulfill();
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_TRUE(triggered);
}

TEST(PromiseTest, ChainedVoidWhen) {
  MockExecutor mockExecutor;

  PromiseFulfiller<void>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<void>>(&fulfiller1);
  PromiseFulfiller<void>::Callback* fulfiller2 = nullptr;

  Promise<void> promise3 = mockExecutor.when(promise1)(
    [&fulfiller2](Void) -> Promise<void> {
      DEBUG_ERROR << "promise3";
      return newPromise<MockPromiseFulfiller<void>>(&fulfiller2);
    });

  bool triggered = false;

  Promise<void> promise4 = mockExecutor.when(promise3)(
    [&triggered](Void) {
      DEBUG_ERROR << "promise4";
      triggered = true;
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller1->fulfill();
  EXPECT_FALSE(mockExecutor.empty());
  EXPECT_TRUE(fulfiller2 == nullptr);
  mockExecutor.runNext();
  EXPECT_FALSE(fulfiller2 == nullptr);
  EXPECT_TRUE(mockExecutor.empty());
  EXPECT_FALSE(triggered);

  ASSERT_TRUE(fulfiller2 != nullptr);
  fulfiller2->fulfill();
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_TRUE(triggered);
}

TEST(PromiseTest, ChainedPreFulfilled) {
  MockExecutor mockExecutor;

  auto promise1 = newFulfilledPromise(123);
  PromiseFulfiller<int>::Callback* fulfiller2;
  auto promise2 = newPromise<MockPromiseFulfiller<int>>(&fulfiller2);

  int result = 0;

  Promise<void> promise3 = mockExecutor.when(promise2)(
    [&result](int a) {
      result = a;
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller2->fulfill(promise1.release());
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  ASSERT_EQ(result, 123);
}

TEST(PromiseTest, MoveSemantics) {
  MockExecutor mockExecutor;

  PromiseFulfiller<OwnedPtr<int>>::Callback* fulfiller;
  auto promise = newPromise<MockPromiseFulfiller<OwnedPtr<int>>>(&fulfiller);

  OwnedPtr<int> ptr = newOwned<int>(12);

  int result = 0;

  Promise<void> promise2 = mockExecutor.when(promise, ptr)(
    [&result](OwnedPtr<int> i, OwnedPtr<int> j) {
      result = *i + *j;
    });

  fulfiller->fulfill(newOwned<int>(34));
  mockExecutor.runNext();
  ASSERT_EQ(result, 46);
}

TEST(PromiseTest, Cancel) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller;
  auto promise = newPromise<MockPromiseFulfiller<int>>(&fulfiller);

  Promise<void> promise2 = mockExecutor.when(promise)(
    [](int i) {
      ADD_FAILURE() << "Can't get here.";
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller->fulfill(5);
  EXPECT_FALSE(mockExecutor.empty());
  promise2.release();
  EXPECT_TRUE(mockExecutor.empty());
}

TEST(PromiseTest, VoidPromise) {
  MockExecutor mockExecutor;

  PromiseFulfiller<void>::Callback* fulfiller;
  auto promise = newPromise<MockPromiseFulfiller<void>>(&fulfiller);

  bool triggered = false;

  Promise<void> promise2 = mockExecutor.when(promise)(
    [&triggered](Void) {
      triggered = true;
    });

  EXPECT_FALSE(triggered);
  fulfiller->fulfill();
  EXPECT_FALSE(triggered);
  mockExecutor.runNext();
  EXPECT_TRUE(triggered);
}

TEST(PromiseTest, Exception) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<int>>(&fulfiller1);
  PromiseFulfiller<int>::Callback* fulfiller2;
  auto promise2 = newPromise<MockPromiseFulfiller<int>>(&fulfiller2);

  bool triggered = false;

  Promise<void> promise3 = mockExecutor.when(promise1, promise2, 123)(
    [](int i, int j, int k) {
      ADD_FAILURE() << "Can't get here.";
    }, [&triggered](MaybeException<int> i, MaybeException<int> j, int k) {
      triggered = true;

      ASSERT_TRUE(i.isException());
      ASSERT_FALSE(j.isException());
      ASSERT_EQ(123, k);
      ASSERT_EQ(456, j.get());

      try {
        i.get();
        ADD_FAILURE() << "Expected exception.";
      } catch (const std::logic_error& e) {
        ASSERT_STREQ("test", e.what());
      }
    });

  try {
    throw std::logic_error("test");
  } catch (...) {
    fulfiller1->propagateCurrentException();
  }
  fulfiller2->fulfill(456);

  EXPECT_FALSE(triggered);

  mockExecutor.runNext();

  EXPECT_TRUE(triggered);
}

TEST(PromiseTest, ExceptionInCallback) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<int>>(&fulfiller1);

  Promise<int> promise2 = mockExecutor.when(promise1)(
    [](int a) -> int {
      throw std::logic_error("test");
    });

  bool triggered = false;

  Promise<void> promise3 = mockExecutor.when(promise2)(
    [](int i) {
      ADD_FAILURE() << "Can't get here.";
    }, [&triggered](MaybeException<int> i) {
      triggered = true;

      ASSERT_TRUE(i.isException());
      try {
        i.get();
        ADD_FAILURE() << "Expected exception.";
      } catch (const std::logic_error& e) {
        ASSERT_STREQ("test", e.what());
      }
    });

  EXPECT_TRUE(mockExecutor.empty());
  fulfiller1->fulfill(12);
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  ASSERT_TRUE(triggered);
}

TEST(PromiseTest, ExceptionPropagation) {
  MockExecutor mockExecutor;

  PromiseFulfiller<int>::Callback* fulfiller1;
  auto promise1 = newPromise<MockPromiseFulfiller<int>>(&fulfiller1);

  Promise<void> promise2 = mockExecutor.when(promise1)(
    [](int a) {
      ADD_FAILURE() << "Can't get here.";
    });

  bool triggered = false;

  Promise<void> promise3 = mockExecutor.when(promise2)(
    [](Void) {
      ADD_FAILURE() << "Can't get here.";
    }, [&triggered](MaybeException<void> i) {
      triggered = true;

      ASSERT_TRUE(i.isException());
      try {
        i.get();
        ADD_FAILURE() << "Expected exception.";
      } catch (const std::logic_error& e) {
        ASSERT_STREQ("test", e.what());
      }
    });

  EXPECT_TRUE(mockExecutor.empty());
  try {
    throw std::logic_error("test");
  } catch (...) {
    fulfiller1->propagateCurrentException();
  }
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  EXPECT_FALSE(mockExecutor.empty());
  mockExecutor.runNext();
  ASSERT_TRUE(triggered);
}

}  // namespace
}  // namespace ekam
