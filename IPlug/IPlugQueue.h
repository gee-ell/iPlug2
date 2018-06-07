/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

// This is based on MLQueue.h by Randy Jones, based on https://kjellkod.wordpress.com/2012/11/28/c-debt-paid-in-full-wait-free-lock-free-queue/

#pragma once

#include <atomic>
#include <cstddef>

template<typename T>
class IPlugQueue final
{
public:
  IPlugQueue(int size)
  {
    mData.Resize(size + 1);
  }

  ~IPlugQueue(){}

  bool Push(const T& item)
  {
    const auto currentWriteIndex = mWriteIndex.load(std::memory_order_relaxed);
    const auto nextWriteIndex = Increment(currentWriteIndex);
    if(nextWriteIndex != mReadIndex.load(std::memory_order_acquire))
    {
      mData.Get()[currentWriteIndex] = item;
      mWriteIndex.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }

  bool Pop(T& item)
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    if(currentReadIndex == mWriteIndex.load(std::memory_order_acquire))
    {
      return false; // empty the queue
    }
    item = mData.Get()[currentReadIndex];
    mReadIndex.store(Increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  size_t ElementsAvailable() const
  {
    return (mWriteIndex.load(std::memory_order_acquire) - mReadIndex.load(std::memory_order_relaxed))%mData.GetSize();
  }

  // useful for reading elements while a criteria is met. Can be used like
  // while IPlugQueue.ElementsAvailable() && q.peek().mTime < 100 { elem = q.pop() ... }
  const T& Peek()
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    return mData[currentReadIndex];
  }

  bool WasEmpty() const
  {
    return (mWriteIndex.load() == mReadIndex.load());
  }

  bool WasFull() const
  {
    const auto nextWriteIndex = increment(mWriteIndex.load());
    return (nextWriteIndex == mReadIndex.load());
  }

private:
  size_t Increment(size_t idx) const
  {
    return (idx + 1) % (mData.GetSize());
  }

  WDL_TypedBuf<T> mData;
  std::atomic<size_t> mWriteIndex{0};
  std::atomic<size_t> mReadIndex{0};
};

