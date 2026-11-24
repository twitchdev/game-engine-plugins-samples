#include "TwitchSamplePlayerController.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"

#include "UI/STwitchSampleRoot.h"
#include "UI/TwitchSampleContext.h"

ATwitchSamplePlayerController::ATwitchSamplePlayerController()
{
	bShowMouseCursor = true;
}

void ATwitchSamplePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !GEngine || !GEngine->GameViewport)
		return;

	Context = MakeShared<FTwitchSampleContext>();
	Context->Start();

	Root = SNew(STwitchSampleRoot, Context.ToSharedRef());
	ViewportWidget = SNew(SWeakWidget).PossiblyNullContent(Root);
	GEngine->GameViewport->AddViewportWidgetContent(ViewportWidget.ToSharedRef());

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Root);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void ATwitchSamplePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stop polls, predictions, and event streams before the UI goes away.
	if (Root.IsValid())
		Root->ReleaseApiResources();
	if (Context.IsValid())
		Context->Shutdown();

	if (ViewportWidget.IsValid() && GEngine && GEngine->GameViewport)
		GEngine->GameViewport->RemoveViewportWidgetContent(ViewportWidget.ToSharedRef());

	ViewportWidget.Reset();
	Root.Reset();
	Context.Reset();

	Super::EndPlay(EndPlayReason);
}
