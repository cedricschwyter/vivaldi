// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_ASH_COMPONENTS_DEVICE_ACTIVITY_CHURN_ACTIVE_STATUS_H_
#define CHROMEOS_ASH_COMPONENTS_DEVICE_ACTIVITY_CHURN_ACTIVE_STATUS_H_

#include <bitset>

#include "base/component_export.h"
#include "base/time/time.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ash {

namespace system {
class StatisticsProvider;
}  // namespace system

namespace device_activity {

// The Churn use case maintains an instance of this class to represent
// which of the past 18 months the device was active.
class COMPONENT_EXPORT(CHROMEOS_ASH_COMPONENTS_DEVICE_ACTIVITY)
    ChurnActiveStatus {
 public:
  // First 10 bits represent number months from 01/01/2000 to the current month.
  // Remaining 18 bits represents past 18 months when device was active from
  // current month.
  static constexpr int kMonthsSinceInceptionSize = 10;
  static constexpr int kActiveMonthsBitSize = 18;
  static constexpr int kChurnBitSize =
      kMonthsSinceInceptionSize + kActiveMonthsBitSize;
  static constexpr int kInceptionYear = 2000;
  static constexpr int kMonthsInYear = 12;
  static constexpr int kMonthsSinceInceptionBitOffset = 0;
  static constexpr int kActiveMonthsBitOffset = 10;

  ChurnActiveStatus();
  explicit ChurnActiveStatus(int value);
  ChurnActiveStatus(const ChurnActiveStatus&) = delete;
  ChurnActiveStatus& operator=(const ChurnActiveStatus&) = delete;
  ~ChurnActiveStatus();

  int GetValueAsInt() const;

  // Updates the |value_| to reflect current month is active.
  absl::optional<std::bitset<kChurnBitSize>> UpdateValue(base::Time ts);

  // Initialize the underlying |value_| field.
  // This method should be called if the device loses the |value_| over restarts
  // and powerwash. Value can be initialized after being recovered from the
  // local_state or preserved file active status value.
  void InitializeValue(int value);

  // Returns the base::Time object representing the defined inception date.
  const base::Time GetInceptionMonth() const;

  // Returns the int representation of the known months since inception.
  int GetMonthsSinceInception() const;

  // Uses the inception month and months since inception in order to return
  // a new timestamp representing the current active month.
  // TODO(hirthanan): Compare against UpdateValue parameter ts month and year to
  // see accuracy and correctness of this method.
  const base::Time GetCurrentActiveMonth() const;

  // Returns the int representation of the known active months in |value_|.
  int GetActiveMonthBits();

  // Set the value for testing.
  void SetValueForTesting(std::bitset<kChurnBitSize> val);

  const base::Time GetFirstActiveWeek() const;

  // Method is used to test the ActivateDate VPD field is able to get translated
  // to the correct base::Time object.
  base::Time GetFirstActiveWeekForTesting(const std::string& year,
                                          const std::string& weeks);

 private:
  // Set |first_active_week_|, which is at the week granularity.
  // This field is set by retrieving the ActivateDate vpd field stored in
  // machine statistics, which is a string.
  // This string is converted to a base::Time object for easier comparison.
  void SetFirstActiveWeek();

  // Value storing the 28 bits for churn active status.
  std::bitset<kChurnBitSize> value_;

  // Singleton lives throughout class lifetime.
  system::StatisticsProvider* const statistics_provider_;

  // This class is constructed after the machine statistics are loaded. This
  // callback logic exists in the device activity controller.
  // The |first_active_week_| stores the UTC based ActivateDate VPD field, which
  // specifies the date (week granularity) when the device was first activated.
  // Note: The exact first active month cannot be determined because of the
  // week granularity, but the overall calculation for first active
  // should be accurate since most weeks fall within the month.
  base::Time first_active_week_;
};

}  // namespace device_activity

}  // namespace ash

#endif  // CHROMEOS_ASH_COMPONENTS_DEVICE_ACTIVITY_CHURN_ACTIVE_STATUS_H_
