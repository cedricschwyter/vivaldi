// Copyright 2022 Vivaldi Technologies. All rights reserved.

#ifndef IOS_CHROME_BROWSER_UI_VIVALDI_SPEED_DIAL_SORTING_MODE_H_
#define IOS_CHROME_BROWSER_UI_VIVALDI_SPEED_DIAL_SORTING_MODE_H_

// Page enumerates the modes of the speed dial sorting.
typedef NS_ENUM(NSUInteger, SpeedDialSortingMode) {
  SpeedDialSortingManual = 0,
  SpeedDialSortingByTitle = 1,
  SpeedDialSortingByAddress = 2,
  SpeedDialSortingByNickname = 3,
  SpeedDialSortingByDescription = 4,
  SpeedDialSortingByDate = 5,
};

#endif  // IOS_CHROME_BROWSER_UI_VIVALDI_SPEED_DIAL_SORTING_MODE_H_
