#include "ReversePlayer.h"

float ReversePlayer::getReadIncrement (float pitchRatio, bool reversed) noexcept
{
    // Positive = forward, negative = backward
    return reversed ? -pitchRatio : pitchRatio;
}
