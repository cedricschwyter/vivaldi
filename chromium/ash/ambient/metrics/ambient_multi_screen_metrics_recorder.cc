// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ambient/metrics/ambient_multi_screen_metrics_recorder.h"

#include <algorithm>

#include "base/check.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/strcat.h"

namespace ash {

AmbientMultiScreenMetricsRecorder::AmbientMultiScreenMetricsRecorder(
    AmbientTheme theme)
    : theme_(theme) {}

AmbientMultiScreenMetricsRecorder::~AmbientMultiScreenMetricsRecorder() {
  base::UmaHistogramCounts100(
      base::StrCat({"Ash.AmbientMode.ScreenCount.", ToString(theme_)}),
      num_registered_screens_);
}

void AmbientMultiScreenMetricsRecorder::RegisterScreen(
    lottie::Animation* animation) {
  ++num_registered_screens_;
  if (!animation)
    return;

  DCHECK(!animation_observations_.IsObservingSource(animation));
  registered_animations_.insert(animation);
  animation_observations_.AddObservation(animation);
}

void AmbientMultiScreenMetricsRecorder::AnimationFramePainted(
    const lottie::Animation* animation,
    float t) {
  if (registered_animations_.size() <= 1u) {
    DVLOG(4) << "Not computing mean timestamp offset for single screen";
    return;
  }

  // Out of the N animations, find the pair with the largest timestamp offset.
  // The below does it brute force in N^2 time because it's simplest and N
  // (the number of screens present) is realistically going to be very small.
  // It's not worth optimizing.
  absl::optional<base::TimeDelta> largest_timestamp_offset;
  for (auto animation_l = registered_animations_.begin();
       animation_l != registered_animations_.end(); ++animation_l) {
    for (auto animation_r = animation_l + 1;
         animation_r != registered_animations_.end(); ++animation_r) {
      absl::optional<base::TimeDelta> offset =
          GetOffsetBetweenAnimations(**animation_l, **animation_r);
      if (!offset) {
        DVLOG(4)
            << "One or both animations are inactive. Cannot compute offset";
        continue;
      }

      if (!largest_timestamp_offset || *offset > *largest_timestamp_offset) {
        largest_timestamp_offset = offset;
      }
    }
  }

  if (!largest_timestamp_offset) {
    DVLOG(4) << "At least 2 animations need to be active to compute an offset";
    return;
  }

  // Since this metric is recorded on every single animation frame, the UMA
  // histogram macros are used for performance reasons. They require a static
  // compile-time metric name as an argument though.
#define MUTLISCREEN_OFFSET_NAME(theme) \
  "Ash.AmbientMode.MultiScreenOffset." theme
  switch (theme_) {
    case AmbientTheme::kFeelTheBreeze:
      UMA_HISTOGRAM_TIMES(MUTLISCREEN_OFFSET_NAME("FeelTheBreeze"),
                          *largest_timestamp_offset);
      break;
    case AmbientTheme::kFloatOnBy:
      UMA_HISTOGRAM_TIMES(MUTLISCREEN_OFFSET_NAME("FloatOnBy"),
                          *largest_timestamp_offset);
      break;
    case AmbientTheme::kSlideshow:
    case AmbientTheme::kVideoNewMexico:
    case AmbientTheme::kVideoClouds:
      LOG(DFATAL) << "Should not be recording animation metrics for "
                  << ToString(theme_);
      break;
  }
#undef MUTLISCREEN_OFFSET_NAME
}

void AmbientMultiScreenMetricsRecorder::AnimationIsDeleting(
    const lottie::Animation* animation) {
  animation_observations_.RemoveObservation(
      const_cast<lottie::Animation*>(animation));
  // Remove from |registered_animations_| here to prevent any possibility of
  // use-after-free if AnimationFramePainted() happens to be called for a
  // remaining animation.
  registered_animations_.erase(animation);
}

absl::optional<base::TimeDelta>
AmbientMultiScreenMetricsRecorder::GetOffsetBetweenAnimations(
    const lottie::Animation& animation_l,
    const lottie::Animation& animation_r) const {
  absl::optional<float> current_progress_l = animation_l.GetCurrentProgress();
  absl::optional<float> current_progress_r = animation_r.GetCurrentProgress();
  if (!current_progress_l || !current_progress_r) {
    DVLOG(4) << "Both animations must be active (playing and painted at least "
                "1 frame) to compute an offset";
    return absl::nullopt;
  }

  const lottie::Animation* animation_with_smaller_t = nullptr;
  const lottie::Animation* animation_with_larger_t = nullptr;
  base::TimeDelta smaller_timestamp;
  base::TimeDelta larger_timestamp;
  if (*current_progress_l < *current_progress_r) {
    animation_with_smaller_t = &animation_l;
    animation_with_larger_t = &animation_r;
    smaller_timestamp =
        *current_progress_l * animation_l.GetAnimationDuration();
    larger_timestamp = *current_progress_r * animation_r.GetAnimationDuration();
  } else {
    animation_with_smaller_t = &animation_r;
    animation_with_larger_t = &animation_l;
    smaller_timestamp =
        *current_progress_r * animation_r.GetAnimationDuration();
    larger_timestamp = *current_progress_l * animation_l.GetAnimationDuration();
  }
  // Take the smaller of incremental (a normal forward animation step) and
  // loopback progress. Ex:
  // * Incremental: .5 -> .52 = .02
  // * Loopback: .98 -> .02 = .04 (whereas the incremental would be .96, which
  //   does not make sense).
  base::TimeDelta incremental_progress = larger_timestamp - smaller_timestamp;
  DCHECK(IsPlaybackConfigValid(animation_with_larger_t->GetPlaybackConfig()));
  DCHECK(IsPlaybackConfigValid(animation_with_smaller_t->GetPlaybackConfig()));
  absl::optional<lottie::Animation::CycleBoundaries> larger_t_cycle =
      animation_with_larger_t->GetCurrentCycleBoundaries();
  absl::optional<lottie::Animation::CycleBoundaries> smaller_t_cycle =
      animation_with_smaller_t->GetCurrentCycleBoundaries();
  DCHECK(larger_t_cycle);
  DCHECK(smaller_t_cycle);

  // Note the animations may not loop from [0, 1]. They may start and end their
  // loops at arbitrary points in the middle. For example, if the start/end
  // points are [.25, .75], and the 2 timestamps are 0.73 and 0.26, the
  // offset would be (.75 - .73) + (.26 - .25) = .03.
  base::TimeDelta looped_progress =
      (larger_t_cycle->end_offset - larger_timestamp) +
      (smaller_timestamp - smaller_t_cycle->start_offset);
  return std::min(incremental_progress, looped_progress);
}

// static
bool AmbientMultiScreenMetricsRecorder::IsPlaybackConfigValid(
    const absl::optional<lottie::Animation::PlaybackConfig>& playback_config) {
  return playback_config &&
         // The logic in GetOffsetBetweenAnimations() assumes animation time
         // always ticks forward.
         playback_config->style != lottie::Animation::Style::kThrobbing;
}

}  // namespace ash
