#include "UI/SSamplePage.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/SampleWidgets.h"

TSharedRef<SWidget> SSamplePage::MakeConnectionHint()
{
	const TWeakPtr<FTwitchSampleContext> WeakContext = Context;
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(SampleUI::PanelColor())
		.Padding(8)
		.Visibility_Lambda([WeakContext]()
		{
			const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
			return Ctx && Ctx->IsLoggedIn() ? EVisibility::Collapsed : EVisibility::Visible;
		})
		[
			SNew(STextBlock)
			.Font(SampleUI::Font(10))
			.ColorAndOpacity(SampleUI::LevelColor(ESampleLogLevel::Warning))
			.AutoWrapText(true)
			.Text_Lambda([WeakContext]()
			{
				const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
				if (!Ctx || !Ctx->HasClientId())
					return FText::FromString(TEXT("No Client ID yet. Enter yours on the Authorization page and press Use this Client ID."));
				return FText::FromString(TEXT("Not logged in. Log in on the Authorization page before using this page."));
			})
		];
}
