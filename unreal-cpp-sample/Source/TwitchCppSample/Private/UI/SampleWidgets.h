#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SWidget.h"

class FTwitchSampleContext;
enum class ESampleLogLevel : uint8;

/**
 * Small Slate helpers so every page lays out its fields, sections, and buttons the same way.
 */
namespace SampleUI
{
	constexpr float LabelWidth = 220.f;

	FLinearColor BackgroundColor();
	FLinearColor PanelColor();
	FLinearColor AccentColor();
	FLinearColor MutedColor();
	FLinearColor LevelColor(ESampleLogLevel Level);

	FSlateFontInfo Font(int32 Size, bool bBold = false);

	/** A heading with an optional explanation underneath. */
	TSharedRef<SWidget> Section(const FString& Heading, const FString& Description = FString());

	/** Grey explanatory text that wraps. */
	TSharedRef<SWidget> Muted(const TAttribute<FText>& Text);

	/** Plain text that wraps. */
	TSharedRef<SWidget> Text(const TAttribute<FText>& Text, const FLinearColor& Color = FLinearColor::White, int32 Size = 10);

	/** A label on the left and any widget on the right, with an optional hint underneath. */
	TSharedRef<SWidget> Row(const FString& Label, const TSharedRef<SWidget>& Content, const FString& Hint = FString());

	/** Indents content to line up with the right-hand column of Row, at its natural width (for buttons). */
	TSharedRef<SWidget> Indented(const TSharedRef<SWidget>& Content);

	/** Like Indented, but full width, for text that wraps. */
	TSharedRef<SWidget> IndentedFill(const TSharedRef<SWidget>& Content);

	/** A text box that writes its text into *Target. Target must outlive the widget (a member of the page). */
	TSharedRef<SWidget> TextBox(FString* Target, const TAttribute<bool>& Enabled = true);

	/** Row(Label, TextBox(Target), Hint). */
	TSharedRef<SWidget> TextRow(const FString& Label, FString* Target, const FString& Hint = FString(), const TAttribute<bool>& Enabled = true);

	/** A check box bound to *Target. Target must outlive the widget. */
	TSharedRef<SWidget> CheckBox(const FString& Label, bool* Target);

	/** A key/value line, used to show results. */
	TSharedRef<SWidget> KeyValue(const FString& Key, const FString& Value);

	/** A bordered panel around content. */
	TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content);

	/** A horizontal bar filled to Fraction (0 to 1), used for vote counts. */
	TSharedRef<SWidget> Bar(float Fraction);

	/** A button that runs OnClick. */
	TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> OnClick, const TAttribute<bool>& Enabled = true);

	/**
	 * A button for an API call. Shows "..." while the call named Action runs, and stays disabled
	 * until the plugin has a Client ID and, unless bRequiresLogin is false, the user is logged in.
	 *
	 * The core library doesn't reject calls made while logged out; it holds them until a login
	 * happens. Disabling the button avoids a click that waits with no feedback.
	 */
	TSharedRef<SWidget> ActionButton(
		const TSharedRef<FTwitchSampleContext>& Context,
		const FString& Action,
		TFunction<void()> OnClick,
		const TAttribute<bool>& Enabled = true,
		bool bRequiresLogin = true);

	/** Splits a comma, semicolon, or newline separated list, trimming and dropping empty items. */
	TArray<FString> SplitList(const FString& Text);

	/** Parses a whole number in range, logging a readable error when it isn't. */
	bool TryParseInt(FTwitchSampleContext& Context, const FString& Text, const FString& FieldName, int32 Min, int32 Max, int32& OutValue);

	/** Checks a required text field, logging a readable error when it's blank or too long. */
	bool ValidateText(FTwitchSampleContext& Context, const FString& Text, const FString& FieldName, int32 MaxLength);

	/** "-" for empty strings, so result rows never look blank. */
	FString OrDash(const FString& Value);
}
