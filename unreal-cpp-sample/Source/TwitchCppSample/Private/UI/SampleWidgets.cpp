#include "UI/SampleWidgets.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/TwitchSampleContext.h"

namespace SampleUI
{
	FLinearColor BackgroundColor() { return FLinearColor(0.012f, 0.011f, 0.018f); }
	FLinearColor PanelColor() { return FLinearColor(0.03f, 0.028f, 0.042f); }
	FLinearColor AccentColor() { return FLinearColor(0.52f, 0.33f, 1.0f); }
	FLinearColor MutedColor() { return FLinearColor(0.45f, 0.45f, 0.52f); }

	FLinearColor LevelColor(ESampleLogLevel Level)
	{
		switch (Level)
		{
		case ESampleLogLevel::Success: return FLinearColor(0.25f, 0.8f, 0.3f);
		case ESampleLogLevel::Warning: return FLinearColor(1.f, 0.7f, 0.15f);
		case ESampleLogLevel::Error: return FLinearColor(1.f, 0.25f, 0.25f);
		default: return FLinearColor(0.85f, 0.85f, 0.88f);
		}
	}

	FSlateFontInfo Font(int32 Size, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	TSharedRef<SWidget> Section(const FString& Heading, const FString& Description)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 2)
			[
				SNew(STextBlock).Text(FText::FromString(Heading)).Font(Font(14, true))
			];
		if (!Description.IsEmpty())
		{
			Box->AddSlot().AutoHeight().Padding(0, 0, 0, 4)[ Muted(FText::FromString(Description)) ];
		}
		return Box;
	}

	TSharedRef<SWidget> Muted(const TAttribute<FText>& InText)
	{
		return SNew(STextBlock).Text(InText).Font(Font(10)).ColorAndOpacity(MutedColor()).AutoWrapText(true);
	}

	TSharedRef<SWidget> Text(const TAttribute<FText>& InText, const FLinearColor& Color, int32 Size)
	{
		return SNew(STextBlock).Text(InText).Font(Font(Size)).ColorAndOpacity(Color).AutoWrapText(true);
	}

	TSharedRef<SWidget> Row(const FString& Label, const TSharedRef<SWidget>& Content, const FString& Hint)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(LabelWidth)
					[
						SNew(STextBlock).Text(FText::FromString(Label)).Font(Font(10))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					Content
				]
			];
		if (!Hint.IsEmpty())
		{
			Box->AddSlot().AutoHeight()[ IndentedFill(Muted(FText::FromString(Hint))) ];
		}
		return Box;
	}

	TSharedRef<SWidget> Indented(const TSharedRef<SWidget>& Content)
	{
		// Left-aligned so buttons keep their natural width instead of stretching across the row.
		return SNew(SBox).Padding(FMargin(LabelWidth, 2, 0, 2)).HAlign(HAlign_Left)[ Content ];
	}

	TSharedRef<SWidget> IndentedFill(const TSharedRef<SWidget>& Content)
	{
		// Full width, for text that wraps (left-aligned wrapping text sizes itself too narrow).
		return SNew(SBox).Padding(FMargin(LabelWidth, 2, 0, 2))[ Content ];
	}

	TSharedRef<SWidget> TextBox(FString* Target, const TAttribute<bool>& Enabled)
	{
		check(Target);
		return SNew(SEditableTextBox)
			.Text(FText::FromString(*Target))
			.Font(Font(10))
			.IsEnabled(Enabled)
			.OnTextChanged_Lambda([Target](const FText& NewText) { *Target = NewText.ToString(); });
	}

	TSharedRef<SWidget> TextRow(const FString& Label, FString* Target, const FString& Hint, const TAttribute<bool>& Enabled)
	{
		return Row(Label, TextBox(Target, Enabled), Hint);
	}

	TSharedRef<SWidget> CheckBox(const FString& Label, bool* Target)
	{
		check(Target);
		return SNew(SCheckBox)
			.IsChecked_Lambda([Target]() { return *Target ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Target](ECheckBoxState State) { *Target = State == ECheckBoxState::Checked; })
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Font(10))
			];
	}

	TSharedRef<SWidget> KeyValue(const FString& Key, const FString& Value)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(LabelWidth)
				[
					SNew(STextBlock).Text(FText::FromString(Key)).Font(Font(10)).ColorAndOpacity(MutedColor())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Text(FText::FromString(OrDash(Value))).Font(Font(10)).AutoWrapText(true)
			];
	}

	TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(PanelColor())
			.Padding(10)
			[
				Content
			];
	}

	TSharedRef<SWidget> Bar(float Fraction)
	{
		return SNew(SBox).WidthOverride(220.f).HeightOverride(12.f)
			[
				SNew(SProgressBar)
				.Percent(FMath::Clamp(Fraction, 0.f, 1.f))
				.FillColorAndOpacity(AccentColor())
			];
	}

	TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> OnClick, const TAttribute<bool>& Enabled)
	{
		return SNew(SButton)
			.IsEnabled(Enabled)
			.OnClicked_Lambda([OnClick]()
			{
				if (OnClick)
					OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(Font(10))
			];
	}

	TSharedRef<SWidget> ActionButton(
		const TSharedRef<FTwitchSampleContext>& Context,
		const FString& Action,
		TFunction<void()> OnClick,
		const TAttribute<bool>& Enabled,
		bool bRequiresLogin)
	{
		const TWeakPtr<FTwitchSampleContext> WeakContext = Context;
		return SNew(SButton)
			.IsEnabled_Lambda([WeakContext, Action, Enabled, bRequiresLogin]()
			{
				const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
				if (!Ctx || !Ctx->HasClientId() || Ctx->IsRunning(Action))
					return false;
				if (bRequiresLogin && !Ctx->IsLoggedIn())
					return false;
				return Enabled.Get();
			})
			.OnClicked_Lambda([OnClick]()
			{
				if (OnClick)
					OnClick();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Font(Font(10))
				.Text_Lambda([WeakContext, Action]()
				{
					const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
					const bool bRunning = Ctx && Ctx->IsRunning(Action);
					return FText::FromString(bRunning ? Action + TEXT(" ...") : Action);
				})
			];
	}

	TArray<FString> SplitList(const FString& Text)
	{
		FString Normalized = Text;
		Normalized.ReplaceInline(TEXT(";"), TEXT(","));
		Normalized.ReplaceInline(TEXT("\r"), TEXT(","));
		Normalized.ReplaceInline(TEXT("\n"), TEXT(","));

		TArray<FString> Parts;
		Normalized.ParseIntoArray(Parts, TEXT(","), true);

		TArray<FString> Result;
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (!Part.IsEmpty())
				Result.AddUnique(Part);
		}
		return Result;
	}

	bool TryParseInt(FTwitchSampleContext& Context, const FString& Text, const FString& FieldName, int32 Min, int32 Max, int32& OutValue)
	{
		const FString Trimmed = Text.TrimStartAndEnd();
		if (!Trimmed.IsEmpty() && Trimmed.IsNumeric() && !Trimmed.Contains(TEXT(".")))
		{
			const int64 Parsed = FCString::Atoi64(*Trimmed);
			if (Parsed >= Min && Parsed <= Max)
			{
				OutValue = static_cast<int32>(Parsed);
				return true;
			}
		}
		Context.Error(FString::Printf(TEXT("%s must be a whole number from %d to %d."), *FieldName, Min, Max));
		return false;
	}

	bool ValidateText(FTwitchSampleContext& Context, const FString& Text, const FString& FieldName, int32 MaxLength)
	{
		const FString Trimmed = Text.TrimStartAndEnd();
		if (Trimmed.IsEmpty())
		{
			Context.Error(FString::Printf(TEXT("%s is required."), *FieldName));
			return false;
		}
		if (Trimmed.Len() > MaxLength)
		{
			Context.Error(FString::Printf(TEXT("%s can be at most %d characters."), *FieldName, MaxLength));
			return false;
		}
		return true;
	}

	FString OrDash(const FString& Value)
	{
		return Value.IsEmpty() ? FString(TEXT("-")) : Value;
	}
}
