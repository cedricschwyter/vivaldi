// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/payments/manage_saved_iban_bubble_view.h"

#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/autofill/payments/dialog_view_ids.h"
#include "chrome/browser/ui/views/autofill/payments/payments_view_util.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/autofill/core/browser/data_model/iban.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"

namespace autofill {

ManageSavedIbanBubbleView::ManageSavedIbanBubbleView(
    views::View* anchor_view,
    content::WebContents* web_contents,
    IbanBubbleController* controller)
    : LocationBarBubbleDelegateView(anchor_view, web_contents),
      controller_(controller) {
  SetButtons(ui::DIALOG_BUTTON_OK);
  SetButtonLabel(ui::DIALOG_BUTTON_OK, controller->GetAcceptButtonText());
  SetExtraView(
      std::make_unique<views::MdTextButton>(
          base::BindRepeating(
              [](ManageSavedIbanBubbleView* bubble) {
                bubble->controller()->OnManageSavedIbanExtraButtonClicked();
              },
              base::Unretained(this)),
          l10n_util::GetStringUTF16(IDS_AUTOFILL_MANAGE_PAYMENTS)))
      ->SetID(autofill::DialogViewId::MANAGE_IBANS_BUTTON);
  SetShowCloseButton(true);
  set_fixed_width(views::LayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_BUBBLE_PREFERRED_WIDTH));
}

std::u16string ManageSavedIbanBubbleView::GetIBANIdentifierString() {
  return controller_->GetIBAN().GetIdentifierStringForAutofillDisplay();
}

void ManageSavedIbanBubbleView::Show(DisplayReason reason) {
  ShowForReason(reason);
  AssignIdsToDialogButtons();
}

void ManageSavedIbanBubbleView::Hide() {
  CloseBubble();

  // If `controller_` is null, WindowClosing() won't invoke OnBubbleClosed(), so
  // do that here. This will clear out `controller_`'s reference to `this`. Note
  // that WindowClosing() happens only after the _asynchronous_ Close() task
  // posted in CloseBubble() completes, but we need to fix references sooner.
  if (controller_) {
    controller_->OnBubbleClosed(
        GetPaymentsBubbleClosedReasonFromWidget(GetWidget()));
  }
  controller_ = nullptr;
}

std::u16string ManageSavedIbanBubbleView::GetWindowTitle() const {
  return controller_ ? controller_->GetWindowTitle() : std::u16string();
}

void ManageSavedIbanBubbleView::WindowClosing() {
  if (controller_) {
    controller_->OnBubbleClosed(
        GetPaymentsBubbleClosedReasonFromWidget(GetWidget()));
    controller_ = nullptr;
  }
}

ManageSavedIbanBubbleView::~ManageSavedIbanBubbleView() = default;

void ManageSavedIbanBubbleView::AssignIdsToDialogButtons() {
  auto* ok_button = GetOkButton();
  if (ok_button) {
    ok_button->SetID(DialogViewId::OK_BUTTON);
  }
  auto* cancel_button = GetCancelButton();
  if (cancel_button) {
    cancel_button->SetID(DialogViewId::CANCEL_BUTTON);
  }
  if (nickname_label_) {
    nickname_label_->SetID(DialogViewId::NICKNAME_LABEL);
  }
}

void ManageSavedIbanBubbleView::Init() {
  const ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  SetID(DialogViewId::MAIN_CONTENT_VIEW_LOCAL);
  views::TableLayout* layout =
      SetLayoutManager(std::make_unique<views::TableLayout>());
  layout
      ->AddColumn(views::LayoutAlignment::kStart,
                  views::LayoutAlignment::kCenter,
                  views::TableLayout::kFixedSize,
                  views::TableLayout::ColumnSize::kUsePreferred, 0, 0)
      .AddPaddingColumn(
          views::TableLayout::kFixedSize,
          provider->GetDistanceMetric(views::DISTANCE_RELATED_LABEL_HORIZONTAL))
      .AddColumn(views::LayoutAlignment::kStretch,
                 views::LayoutAlignment::kCenter, 1.0,
                 views::TableLayout::ColumnSize::kFixed, 0, 0)
      // Add a row for IBAN label and the value of IBAN.
      .AddRows(1, views::TableLayout::kFixedSize);

  AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_AUTOFILL_SAVE_IBAN_LABEL),
      views::style::CONTEXT_DIALOG_BODY_TEXT, views::style::STYLE_PRIMARY));
  // TODO(crbug.com/1349109): Revisit how the IBAN value will be shown if it's
  // too long, same for nickname row.
  AddChildView(
      std::make_unique<views::Label>(GetIBANIdentifierString(),
                                     views::style::CONTEXT_DIALOG_BODY_TEXT,
                                     views::style::STYLE_PRIMARY))
      ->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Nickname label row will be added if a nickname was saved in the IBAN save
  // bubble, which is displayed previously in the flow.
  if (!controller_->GetIBAN().nickname().empty()) {
    layout
        ->AddPaddingRow(views::TableLayout::kFixedSize,
                        ChromeLayoutProvider::Get()->GetDistanceMetric(
                            views::DISTANCE_RELATED_CONTROL_VERTICAL))
        // Add a row for the nickname label.
        .AddRows(1, views::TableLayout::kFixedSize);
    AddChildView(std::make_unique<views::Label>(
        l10n_util::GetStringUTF16(IDS_AUTOFILL_SAVE_IBAN_PROMPT_NICKNAME),
        views::style::CONTEXT_DIALOG_BODY_TEXT, views::style::STYLE_PRIMARY));
    nickname_label_ = AddChildView(std::make_unique<views::Label>(
        controller_->GetIBAN().nickname(),
        views::style::CONTEXT_DIALOG_BODY_TEXT, views::style::STYLE_PRIMARY));
    nickname_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  }
}

}  // namespace autofill
