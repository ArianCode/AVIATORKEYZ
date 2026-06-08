#pragma once

#include <juce_core/juce_core.h>

#if defined (AVIATORKEYZ_DEBUG) && AVIATORKEYZ_DEBUG
  #define AK_ASSERT(x) jassert (x)
  #define AK_LOG(msg)  DBG (msg)
#else
  #define AK_ASSERT(x)
  #define AK_LOG(msg)
#endif
