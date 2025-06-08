#include "UE5_GraphCaptureOptionsDialog.h"
#include "UE5_GraphCapture.h"

#include "Widgets/SBoxPanel.h" 
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

void SJsonExportOptionsDialog::Construct(const FArguments& InArgs)
{
    OkDelegate     = InArgs._OnOk;
    CancelDelegate = InArgs._OnCancel;

    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(STextBlock).Text(NSLOCTEXT("GraphCap","DlgTitle","Select JSON Export Options"))
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State){ Options.bIncludeMeta = (State==ECheckBoxState::Checked); })
            .IsChecked(ECheckBoxState::Checked)
            [
                SNew(STextBlock).Text(NSLOCTEXT("GraphCap","OptMeta","Include Graph Metadata"))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State){ Options.bIncludePosition = (State==ECheckBoxState::Checked); })
            .IsChecked(ECheckBoxState::Checked)
            [
                SNew(STextBlock).Text(NSLOCTEXT("GraphCap","OptPos","Include Node Positions"))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State){ Options.bIncludePins = (State==ECheckBoxState::Checked); })
            .IsChecked(ECheckBoxState::Checked)
            [
                SNew(STextBlock).Text(NSLOCTEXT("GraphCap","OptPins","Include Inputs/Outputs"))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State){ Options.bIncludeHlsl = (State==ECheckBoxState::Checked); })
            .IsChecked(ECheckBoxState::Checked)
            [
                SNew(STextBlock).Text(NSLOCTEXT("GraphCap","OptHlsl","Include Custom HLSL"))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().Padding(4)
        [
            SNew(SCheckBox)
            .OnCheckStateChanged_Lambda([this](ECheckBoxState State){ Options.bIncludeRegions = (State==ECheckBoxState::Checked); })
            .IsChecked(ECheckBoxState::Checked)
            [
                SNew(STextBlock).Text(NSLOCTEXT("GraphCap","OptRegions","Include Comment Regions"))
            ]
        ]
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(4)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GraphCap","BtnOk","OK"))
                .OnClicked(this, &SJsonExportOptionsDialog::OnOkClicked)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(2)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GraphCap","BtnCancel","Cancel"))
                .OnClicked(this, &SJsonExportOptionsDialog::OnCancelClicked)
            ]
        ]
    ];
}

FReply SJsonExportOptionsDialog::OnOkClicked()
{
    OkDelegate.ExecuteIfBound();
    return FReply::Handled();
}

FReply SJsonExportOptionsDialog::OnCancelClicked()
{
    CancelDelegate.ExecuteIfBound();
    return FReply::Handled();
}
