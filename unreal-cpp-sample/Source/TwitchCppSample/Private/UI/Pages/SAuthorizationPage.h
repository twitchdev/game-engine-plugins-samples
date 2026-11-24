#pragma once

#include "CoreMinimal.h"

#include "UI/SSamplePage.h"

/**
 * Enter a Client ID, choose scopes, and log in with Twitch's device code flow.
 */
class SAuthorizationPage : public SSamplePage
{
public:
	SLATE_BEGIN_ARGS(SAuthorizationPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);

	virtual FString GetTitle() const override { return TEXT("Authorization"); }
	virtual void ReleaseApiResources() override;

private:
	/** Starts the device code login with the selected scopes and opens the activation page. */
	void LogIn();

	struct FScopeOption
	{
		FString Scope;
		FString UsedFor;
		bool bEnabled = true;
	};

	TSharedRef<SWidget> MakeClientIdSection();
	TSharedRef<SWidget> MakeScopesSection();
	TSharedRef<SWidget> MakeLoginSection();

	FString ClientIdText;
	TArray<FScopeOption> Scopes;

	FString UserCode;
	FString ActivationUri;

	TSharedPtr<SVerticalBox> UserInfoBox;
};
