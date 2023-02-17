// Copyright 2022 Vivaldi Technologies. All rights reserved.

#import "ios/ui/custom_views/vivaldi_text_field_view.h"

#import "UIKit/UIKit.h"

#import "ios/chrome/browser/ui/bookmarks/vivaldi_bookmarks_constants.h"
#import "ios/chrome/browser/ui/ntp/vivaldi_ntp_constants.h"
#import "ios/ui/custom_views/custom_view_constants.h"
#import "ios/ui/helpers/vivaldi_colors_helper.h"
#import "ios/ui/helpers/vivaldi_uiview_layout_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Padding for the textfield
// In order - Top, Leading, Bottom, Trailing
const UIEdgeInsets textFieldPadding = UIEdgeInsetsMake(0.0, 12.0, 0.0, 0.0);
// Padding for the delete button
// In order - Top, Leading, Bottom, Trailing
const UIEdgeInsets deleteTextButtonPadding = UIEdgeInsetsMake(0.0, 8.0, 0.0, 12.0);
// Padding for the underline view on the textfield
// In order - Top, Leading, Bottom, Trailing
const UIEdgeInsets underlinePadding = UIEdgeInsetsMake(2.0, 12.0, 0.0, 0.0);
// Size for the underline
// In order - Width, Height
const CGSize underlineSize = CGSizeMake(0.0, 1.0);
}

@interface VivaldiTextFieldView()<UITextFieldDelegate>
// The textfield that backs the view
@property(nonatomic,weak) UITextField* textField;
// Textfield text clear button
@property(nonatomic,weak) UIButton* deleteTextButton;
@end

@implementation VivaldiTextFieldView

@synthesize textField = _textField;
@synthesize deleteTextButton = _deleteTextButton;

#pragma mark - INITIALIZER
- (instancetype)initWithPlaceholder:(NSString*)placeholder {
  if (self = [super initWithFrame:CGRectZero]) {
    self.backgroundColor = UIColor.clearColor;
    [self setUpUI];
    [self setPlaceholder:placeholder];
  }
  return self;
}

#pragma mark - SET UP UI COMPONENTS
- (void)setUpUI {
  // Textfield
  UITextField* textField = [UITextField new];
  _textField = textField;
  [textField setDelegate: self];
  textField.adjustsFontForContentSizeCategory = YES;
  textField.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  textField.backgroundColor = UIColor.clearColor;
  textField.textColor = UIColor.labelColor;
  [self addSubview:textField];
  [textField anchorTop:self.topAnchor
               leading:self.leadingAnchor
                bottom:nil
              trailing:nil
               padding:textFieldPadding];
  [textField addTarget:self
                action:@selector(textFieldDidChange)
      forControlEvents:UIControlEventEditingChanged];

  // Delete Text Button
  UIButton* deleteTextButton = [UIButton new];
  _deleteTextButton = deleteTextButton;
  [deleteTextButton setImage:[UIImage
                              systemImageNamed:vTextfieldDeleteIcon]
                    forState:UIControlStateNormal];
  [deleteTextButton setTintColor:UIColor.vSystemGray];
  [deleteTextButton addTarget:self
                       action:@selector(handleDeleteTextButtonTap)
             forControlEvents:UIControlEventTouchUpInside];

  [self addSubview:deleteTextButton];
  [deleteTextButton anchorTop:nil
                      leading:textField.trailingAnchor
                       bottom:nil
                     trailing:self.trailingAnchor
                      padding:deleteTextButtonPadding];
  [deleteTextButton centerYInSuperview];
  // Hide the delete text button initially
  [deleteTextButton setAlpha:0];

  // Bottom underline
  UIView* underlineView = [UIView new];
  underlineView.backgroundColor = UIColor.quaternaryLabelColor;

  [self addSubview:underlineView];
  [underlineView anchorTop:textField.bottomAnchor
                   leading:self.leadingAnchor
                    bottom:self.bottomAnchor
                  trailing:self.trailingAnchor
                   padding:underlinePadding
                      size:underlineSize];
}

#pragma mark - ACTIONS
/// Remove the text from the textfield and hide the delete button when delete button is tapped.
- (void)handleDeleteTextButtonTap {
  self.textField.text = nil;
  [self hideDeleteTextButton];
}

#pragma mark - UITextFieldDelegate

-(void)textFieldDidBeginEditing:(UITextField *)textField {
  if ([self.textField hasText] && self.deleteTextButton.alpha == 0) {
    [self showDeleteTextButton];
  }
}

-(void)textFieldDidEndEditing:(UITextField *)textField {
  [self hideDeleteTextButton];
}

#pragma mark - PRIVATE

/// Triggers the method when textfield is changed.
- (void)textFieldDidChange {
  if ([self.textField hasText]) {
    [self showDeleteTextButton];
  } else {
    [self hideDeleteTextButton];
  }
}

/// Make delete button visible.
- (void)showDeleteTextButton {
  [UIView animateWithDuration:0.3 animations:^{
    [self.deleteTextButton setAlpha:1.0];
  }];
}

/// Make delete button hidden.
- (void)hideDeleteTextButton {
  [UIView animateWithDuration:0.3 animations:^{
    [self.deleteTextButton setAlpha:0];
  }];
}

#pragma mark - SETTERS
- (void)setPlaceholder:(NSString*)placeholder {
  self.textField.placeholder = placeholder;
  self.textField.accessibilityLabel = placeholder;
}

- (void)setText:(NSString*)text {
  self.textField.text = text;
  self.textField.accessibilityLabel = text;
}

- (void)setURLMode {
  self.textField.autocapitalizationType =
    UITextAutocapitalizationTypeNone;
  self.textField.keyboardType = UIKeyboardTypeURL;
}

#pragma mark - GETTERS
- (BOOL)hasText {
  return [self.textField hasText];
}

- (NSString*)getText {
  return self.textField.text;
}

@end
