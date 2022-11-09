﻿// All rights reserved Dominik Pavlicek 2022.

#include "Components/ActorInteractableComponentBase.h"

#include "Helpers/ActorInteractionPluginLog.h"

#if WITH_EDITOR
#include "InteractionEditorNotifications/Public/EditorHelper.h"
#endif

#include "Interfaces/ActorInteractorInterface.h"

UActorInteractableComponentBase::UActorInteractableComponentBase()
{
	PrimaryComponentTick.bCanEverTick = false;

	bToggleDebug = false;
	SetupType = ESetupType::EST_Quick;

	InteractableState = EInteractableStateV2::EIS_Asleep;
	DefaultInteractableState = EInteractableStateV2::EIS_Asleep;
	InteractionWeight = 1;
	
	bInteractionHighlight = true;
	StencilID = 133;

	LifecycleMode = EInteractableLifecycle::EIL_Cycled;
	LifecycleCount = 2;
	CooldownPeriod = 3.f;
	RemainingLifecycleCount = LifecycleCount;

	InteractionOwner = GetOwner();

	const FInteractionKeySetup GamepadKeys = FKey("Gamepad Face Button Down");
	FInteractionKeySetup KeyboardKeys = GamepadKeys;
	KeyboardKeys.Keys.Add("E");
		
	InteractionKeysPerPlatform.Add((TEXT("Windows")), KeyboardKeys);
	InteractionKeysPerPlatform.Add((TEXT("Mac")), KeyboardKeys);
	InteractionKeysPerPlatform.Add((TEXT("PS4")), GamepadKeys);
	InteractionKeysPerPlatform.Add((TEXT("XboxOne")), GamepadKeys);
}

void UActorInteractableComponentBase::BeginPlay()
{
	Super::BeginPlay();

	InteractionOwner = GetOwner();

	// Interaction Events
	OnInteractableSelected.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableSelectedEvent);
	OnInteractorFound.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractorFound);
	OnInteractorLost.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractorLost);

	OnInteractorOverlapped.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlapEvent);
	OnInteractorStopOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlapEvent);
	OnInteractorTraced.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableTracedEvent);

	OnInteractionCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCompleted);
	OnInteractionStarted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionStarted);
	OnInteractionStopped.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionStopped);
	OnInteractionCanceled.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCanceled);
	OnLifecycleCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionLifecycleCompleted);
	OnCooldownCompleted.AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractionCooldownCompleted);
	
	// Attributes Events
	OnInteractableDependencyChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableDependencyChangedEvent);
	OnInteractableAutoSetupChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableAutoSetupChangedEvent);
	OnInteractableWeightChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableWeightChangedEvent);
	OnInteractableStateChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStateChangedEvent);
	OnInteractableOwnerChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableOwnerChangedEvent);
	OnInteractableCollisionChannelChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableCollisionChannelChangedEvent);
	OnLifecycleModeChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnLifecycleModeChangedEvent);
	OnLifecycleCountChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnLifecycleCountChangedEvent);
	OnCooldownPeriodChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCooldownPeriodChangedEvent);
	OnInteractorChanged.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractorChangedEvent);

	// Ignored Classes Events
	OnIgnoredInteractorClassAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnIgnoredClassAdded);
	OnIgnoredInteractorClassRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnIgnoredClassRemoved);

	// Highlight Events
	OnHighlightableComponentAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnHighlightableComponentAddedEvent);
	OnHighlightableComponentRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnHighlightableComponentRemovedEvent);
	
	// Collision Events
	OnCollisionComponentAdded.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCollisionComponentAddedEvent);
	OnCollisionComponentRemoved.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnCollisionComponentRemovedEvent);
	
	SetState(DefaultInteractableState);

	AutoSetup();

	RemainingLifecycleCount = LifecycleCount;

#if WITH_EDITOR
	
	DrawDebug();

#endif
}

#pragma region InteractionImplementations

bool UActorInteractableComponentBase::DoesAutoSetup() const
{ return SetupType != ESetupType::EST_None; }

void UActorInteractableComponentBase::ToggleAutoSetup(const ESetupType& NewValue)
{
	SetupType = NewValue;
}

bool UActorInteractableComponentBase::ActivateInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Active);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Active:
			ErrorMessage.Append(TEXT("Interactable Component is already Active"));
			break;
		case EInteractableStateV2::EIS_Awake:
			ErrorMessage.Append(TEXT("Interactable Component has been Activated"));
			return true;
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Completed:
		case EInteractableStateV2::EIS_Disabled:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Activated"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::WakeUpInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Awake);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Awake:
			ErrorMessage.Append(TEXT("Interactable Component is already Awake"));
			break;
		case EInteractableStateV2::EIS_Active:
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Disabled:
			ErrorMessage.Append(TEXT("Interactable Component has been Awaken"));
			return true;
		case EInteractableStateV2::EIS_Completed:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Awaken"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::SnoozeInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Asleep);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Asleep:
			ErrorMessage.Append(TEXT("Interactable Component is already Asleep"));
			break;
		case EInteractableStateV2::EIS_Awake:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Active:
		case EInteractableStateV2::EIS_Disabled:
			ErrorMessage.Append(TEXT("Interactable Component has been Asleep"));
			return true;
		case EInteractableStateV2::EIS_Cooldown:
			// TODO: reset cooldown
			ErrorMessage.Append(TEXT("Interactable Component has been Asleep"));
			return true;
		case EInteractableStateV2::EIS_Completed:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Asleep"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

bool UActorInteractableComponentBase::CompleteInteractable(FString& ErrorMessage)
{
	const EInteractableStateV2 CachedState = GetState();

	SetState(EInteractableStateV2::EIS_Completed);

	switch (CachedState)
	{
		case EInteractableStateV2::EIS_Active:
			ErrorMessage.Append(TEXT("Interactable Component is Completed"));
			return true;
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::EIS_Awake:
		case EInteractableStateV2::EIS_Disabled:
		case EInteractableStateV2::EIS_Cooldown:
			ErrorMessage.Append(TEXT("Interactable Component cannot be Completed"));
			break;
		case EInteractableStateV2::EIS_Completed:
			ErrorMessage.Append(TEXT("Interactable Component is already Completed"));
			break;
		case EInteractableStateV2::Default: 
		default:
			ErrorMessage.Append(TEXT("Interactable Component cannot proces activation request, invalid state"));
			break;
	}
	
	return false;
}

void UActorInteractableComponentBase::DeactivateInteractable()
{
	SetState(EInteractableStateV2::EIS_Disabled);
}

bool UActorInteractableComponentBase::CanInteractEvent_Implementation() const
{
	return CanInteract();
}

bool UActorInteractableComponentBase::CanInteract() const
{
	switch (InteractableState)
	{
		case EInteractableStateV2::EIS_Awake:
			return GetInteractor().GetInterface() != nullptr;
		case EInteractableStateV2::EIS_Active: // already interacting
		case EInteractableStateV2::EIS_Asleep:
		case EInteractableStateV2::EIS_Disabled:
		case EInteractableStateV2::EIS_Cooldown:
		case EInteractableStateV2::EIS_Completed:
		case EInteractableStateV2::EIS_Suppressed:
		case EInteractableStateV2::Default: 
		default: break;
	}

	return false;
}

bool UActorInteractableComponentBase::CanBeTriggered() const
{ return InteractableState == EInteractableStateV2::EIS_Awake && Interactor.GetInterface() == nullptr; }

bool UActorInteractableComponentBase::IsInteracting() const
{ return InteractableState == EInteractableStateV2::EIS_Active; }

EInteractableStateV2 UActorInteractableComponentBase::GetState() const
{ return InteractableState; }

void UActorInteractableComponentBase::SetState(const EInteractableStateV2 NewState)
{
	StopHighlight();
	
	switch (NewState)
	{
		case EInteractableStateV2::EIS_Active:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Awake:
					StartHighlight();
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Active:
					StartHighlight();
					break;
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::Default:
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Awake:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Asleep:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Cooldown:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Completed:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
					{
						InteractableState = NewState;
						OnInteractableStateChanged.Broadcast(InteractableState);
						if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
						SetInteractor(nullptr);
					}
					break;
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Disabled:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Asleep:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Disabled:
				case EInteractableStateV2::Default: 
				default: break;
			}
			break;
		case EInteractableStateV2::EIS_Suppressed:
			switch (InteractableState)
			{
				case EInteractableStateV2::EIS_Active:
				case EInteractableStateV2::EIS_Awake:
				case EInteractableStateV2::EIS_Asleep:
				case EInteractableStateV2::EIS_Disabled:
					InteractableState = NewState;
					OnInteractableStateChanged.Broadcast(InteractableState);
					break;
				case EInteractableStateV2::EIS_Cooldown:
				case EInteractableStateV2::EIS_Completed:
				case EInteractableStateV2::EIS_Suppressed:
				case EInteractableStateV2::Default:
				default: break;
			}
			break;
		case EInteractableStateV2::Default: 
		default: break;
	}
	
	ProcessDependencies();
}

void UActorInteractableComponentBase::StartHighlight() const
{
	for (const auto Itr : HighlightableComponents)
	{
		Itr->SetRenderCustomDepth(bInteractionHighlight);
		Itr->SetCustomDepthStencilValue(StencilID);
	}
}

void UActorInteractableComponentBase::StopHighlight() const
{
	for (const auto Itr : HighlightableComponents)
	{
		Itr->SetRenderCustomDepth(false);
		Itr->SetCustomDepthStencilValue(0);
	}
}

TArray<TSoftClassPtr<UObject>> UActorInteractableComponentBase::GetIgnoredClasses() const
{ return IgnoredClasses; }

void UActorInteractableComponentBase::SetIgnoredClasses(const TArray<TSoftClassPtr<UObject>> NewIgnoredClasses)
{
	IgnoredClasses.Empty();

	IgnoredClasses = NewIgnoredClasses;
}

void UActorInteractableComponentBase::AddIgnoredClass(TSoftClassPtr<UObject> AddIgnoredClass)
{
	if (AddIgnoredClass == nullptr) return;

	if (IgnoredClasses.Contains(AddIgnoredClass)) return;

	IgnoredClasses.Add(AddIgnoredClass);

	OnIgnoredInteractorClassAdded.Broadcast(AddIgnoredClass);
}

void UActorInteractableComponentBase::AddIgnoredClasses(TArray<TSoftClassPtr<UObject>> AddIgnoredClasses)
{
	for (const auto Itr : AddIgnoredClasses)
	{
		AddIgnoredClass(Itr);
	}
}

void UActorInteractableComponentBase::RemoveIgnoredClass(TSoftClassPtr<UObject> RemoveIgnoredClass)
{
	if (RemoveIgnoredClass == nullptr) return;

	if (!IgnoredClasses.Contains(RemoveIgnoredClass)) return;

	IgnoredClasses.Remove(RemoveIgnoredClass);

	OnIgnoredInteractorClassRemoved.Broadcast(RemoveIgnoredClass);
}

void UActorInteractableComponentBase::RemoveIgnoredClasses(TArray<TSoftClassPtr<UObject>> RemoveIgnoredClasses)
{
	for (const auto Itr : RemoveIgnoredClasses)
	{
		RemoveIgnoredClass(Itr);
	}
}

void UActorInteractableComponentBase::AddInteractionDependency(const TScriptInterface<IActorInteractableInterface> InteractionDependency)
{
	if (InteractionDependencies.Contains(InteractionDependency)) return;

	OnInteractableDependencyChanged.Broadcast(InteractionDependency);

	InteractionDependencies.Add(InteractionDependency);
}

void UActorInteractableComponentBase::RemoveInteractionDependency(const TScriptInterface<IActorInteractableInterface> InteractionDependency)
{
	if (!InteractionDependencies.Contains(InteractionDependency)) return;

	OnInteractableDependencyChanged.Broadcast(InteractionDependency);

	InteractionDependencies.Remove(InteractionDependency);
}

TArray<TScriptInterface<IActorInteractableInterface>> UActorInteractableComponentBase::GetInteractionDependencies() const
{ return InteractionDependencies; }

void UActorInteractableComponentBase::ProcessDependencies()
{
	if (InteractionDependencies.Num() == 0) return;

	auto Dependencies = InteractionDependencies;
	for (const auto Itr : Dependencies)
	{
		switch (InteractableState)
		{
			case EInteractableStateV2::EIS_Active:
			case EInteractableStateV2::EIS_Suppressed:
				Itr->SetState(EInteractableStateV2::EIS_Suppressed);
				break;
			case EInteractableStateV2::EIS_Disabled:
			case EInteractableStateV2::EIS_Awake:
			case EInteractableStateV2::EIS_Asleep:
				Itr->SetState(InteractableState);
				break;
			case EInteractableStateV2::EIS_Cooldown:
			case EInteractableStateV2::EIS_Completed:
				Itr->SetState(EInteractableStateV2::EIS_Awake);
				RemoveInteractionDependency(Itr);
				break;
			case EInteractableStateV2::Default:
			default:
				break;
		}
	}
}

TScriptInterface<IActorInteractorInterface> UActorInteractableComponentBase::GetInteractor() const
{ return Interactor; }

void UActorInteractableComponentBase::SetInteractor(const TScriptInterface<IActorInteractorInterface> NewInteractor)
{
	if (NewInteractor.GetInterface() != nullptr)
	{
		NewInteractor->GetOnInteractableSelectedHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
		NewInteractor->GetOnInteractableFoundHandle().Broadcast(this);
	}
	else
	{
		if (Interactor.GetInterface() != nullptr)
		{
			Interactor->GetOnInteractableSelectedHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableSelected);
			Interactor->GetOnInteractableFoundHandle().Broadcast(this);
		}
	}

	Interactor = NewInteractor;
	OnInteractorChanged.Broadcast(Interactor);
}

float UActorInteractableComponentBase::GetInteractionProgress() const
{
	if (!GetWorld()) return -1;
	return GetWorld()->GetTimerManager().GetTimerElapsed(Timer_Interaction) / InteractionPeriod;
}

float UActorInteractableComponentBase::GetInteractionPeriod() const
{ return InteractionPeriod; }

void UActorInteractableComponentBase::SetInteractionPeriod(const float NewPeriod)
{
	float TempPeriod = NewPeriod;
	if (TempPeriod > -1.f && TempPeriod < 0.1f)
	{
		TempPeriod = 0.1f;
	}
	if (FMath::IsNearlyZero(TempPeriod, 0.001f))
	{
		TempPeriod = 0.1f;
	}

	InteractionPeriod = FMath::Max(-1.f, TempPeriod);
}

int32 UActorInteractableComponentBase::GetInteractableWeight() const
{ return InteractionWeight; }

void UActorInteractableComponentBase::SetInteractableWeight(const int32 NewWeight)
{
	InteractionWeight = FMath::Max(0, NewWeight);

	OnInteractableWeightChanged.Broadcast(InteractionWeight);
}

AActor* UActorInteractableComponentBase::GetInteractableOwner() const
{ return InteractionOwner; }

void UActorInteractableComponentBase::SetInteractableOwner(AActor* NewOwner)
{
	if (NewOwner == nullptr) return;
	InteractionOwner = NewOwner;
	
	OnInteractableOwnerChanged.Broadcast(InteractionOwner);
}

ECollisionChannel UActorInteractableComponentBase::GetCollisionChannel() const
{ return CollisionChannel; }

void UActorInteractableComponentBase::SetCollisionChannel(const ECollisionChannel& NewChannel)
{
	CollisionChannel = NewChannel;

	OnInteractableCollisionChannelChanged.Broadcast(CollisionChannel);
}

TArray<UPrimitiveComponent*> UActorInteractableComponentBase::GetCollisionComponents() const
{	return CollisionComponents;}

EInteractableLifecycle UActorInteractableComponentBase::GetLifecycleMode() const
{	return LifecycleMode;}

void UActorInteractableComponentBase::SetLifecycleMode(const EInteractableLifecycle& NewMode)
{
	LifecycleMode = NewMode;

	OnLifecycleModeChanged.Broadcast(LifecycleMode);
}

int32 UActorInteractableComponentBase::GetLifecycleCount() const
{	return LifecycleCount;}

void UActorInteractableComponentBase::SetLifecycleCount(const int32 NewLifecycleCount)
{
	switch (LifecycleMode)
	{
		case EInteractableLifecycle::EIL_Cycled:
			if (NewLifecycleCount < -1)
			{
				LifecycleCount = -1;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			else if (NewLifecycleCount < 2)
			{
				LifecycleCount = 2;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			else if (NewLifecycleCount > 2)
			{
				LifecycleCount = NewLifecycleCount;
				OnLifecycleCountChanged.Broadcast(LifecycleCount);
			}
			break;
		case EInteractableLifecycle::EIL_OnlyOnce:
		case EInteractableLifecycle::Default:
		default: break;
	}
}

int32 UActorInteractableComponentBase::GetRemainingLifecycleCount() const
{ return RemainingLifecycleCount; }

float UActorInteractableComponentBase::GetCooldownPeriod() const
{ return CooldownPeriod; }

void UActorInteractableComponentBase::SetCooldownPeriod(const float NewCooldownPeriod)
{
	switch (LifecycleMode)
	{
		case EInteractableLifecycle::EIL_Cycled:
			LifecycleCount = FMath::Max(0.1f, NewCooldownPeriod);
			OnLifecycleCountChanged.Broadcast(LifecycleCount);
			break;
		case EInteractableLifecycle::EIL_OnlyOnce:
		case EInteractableLifecycle::Default:
		default: break;
	}
}

FKey UActorInteractableComponentBase::GetInteractionKey(const FString& RequestedPlatform) const
{
	if(const FInteractionKeySetup* KeySet = InteractionKeysPerPlatform.Find(RequestedPlatform))
	{
		if (KeySet->Keys.Num() == 0) return FKey();

		return KeySet->Keys[0];
	}
	
	return FKey();
}

void UActorInteractableComponentBase::SetInteractionKey(const FString& Platform, const FKey NewInteractorKey)
{
	if (const auto KeySet = InteractionKeysPerPlatform.Find(Platform))
	{
		if (KeySet->Keys.Contains(NewInteractorKey)) return;

		KeySet->Keys.Add(NewInteractorKey);
	}
}

TMap<FString, FInteractionKeySetup> UActorInteractableComponentBase::GetInteractionKeys() const
{	return InteractionKeysPerPlatform;}

bool UActorInteractableComponentBase::FindKey(const FKey& RequestedKey, const FString& Platform) const
{
	if (const auto KeySet = InteractionKeysPerPlatform.Find(Platform))
	{
		return KeySet->Keys.Contains(RequestedKey);
	}
	
	return false;
}

void UActorInteractableComponentBase::AddCollisionComponent(UPrimitiveComponent* CollisionComp)
{
	if (CollisionComp == nullptr) return;
	if (CollisionComponents.Contains(CollisionComp)) return;
	
	CollisionComponents.Add(CollisionComp);
	
	BindCollisionShape(CollisionComp);
	
	OnCollisionComponentAdded.Broadcast(CollisionComp);
}

void UActorInteractableComponentBase::AddCollisionComponents(const TArray<UPrimitiveComponent*> NewCollisionComponents)
{
	for (UPrimitiveComponent* const Itr : NewCollisionComponents)
	{
		AddCollisionComponent(Itr);
	}
}

void UActorInteractableComponentBase::RemoveCollisionComponent(UPrimitiveComponent* CollisionComp)
{
	if (CollisionComp == nullptr) return;
	if (!CollisionComponents.Contains(CollisionComp)) return;
	
	CollisionComponents.Remove(CollisionComp);

	UnbindCollisionShape(CollisionComp);
	
	OnCollisionComponentRemoved.Broadcast(CollisionComp);
}

void UActorInteractableComponentBase::RemoveCollisionComponents(const TArray<UPrimitiveComponent*> RemoveCollisionComponents)
{
	for (UPrimitiveComponent* const Itr : RemoveCollisionComponents)
	{
		RemoveCollisionComponent(Itr);
	}
}

TArray<UMeshComponent*> UActorInteractableComponentBase::GetHighlightableComponents() const
{	return HighlightableComponents;}

void UActorInteractableComponentBase::AddHighlightableComponent(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr) return;
	if (HighlightableComponents.Contains(MeshComponent)) return;

	HighlightableComponents.Add(MeshComponent);

	BindHighlightableMesh(MeshComponent);

	OnHighlightableComponentAdded.Broadcast(MeshComponent);
}

void UActorInteractableComponentBase::AddHighlightableComponents(const TArray<UMeshComponent*> AddMeshComponents)
{
	for (UMeshComponent* const Itr : HighlightableComponents)
	{
		AddHighlightableComponent(Itr);
	}
}

void UActorInteractableComponentBase::RemoveHighlightableComponent(UMeshComponent* MeshComponent)
{
	if (MeshComponent == nullptr) return;
	if (!HighlightableComponents.Contains(MeshComponent)) return;

	HighlightableComponents.Remove(MeshComponent);

	UnbindHighlightableMesh(MeshComponent);

	OnHighlightableComponentRemoved.Broadcast(MeshComponent);
}

void UActorInteractableComponentBase::RemoveHighlightableComponents(const TArray<UMeshComponent*> RemoveMeshComponents)
{
	for (UMeshComponent* const Itr : RemoveMeshComponents)
	{
		RemoveHighlightableComponent(Itr);
	}
}

UMeshComponent* UActorInteractableComponentBase::FindMeshByName(const FName Name) const
{
	if (!GetOwner()) return nullptr;

	TArray<UMeshComponent*> MeshComponents;
	GetOwner()->GetComponents(MeshComponents);

	for (const auto Itr : MeshComponents)
	{
		if (Itr && Itr->GetName().Equals(Name.ToString()))
		{
			return Itr;
		}
	}

	return nullptr;
}

UMeshComponent* UActorInteractableComponentBase::FindMeshByTag(const FName Tag) const
{
	if (!GetOwner()) return nullptr;

	TArray<UMeshComponent*> MeshComponents;
	GetOwner()->GetComponents(MeshComponents);

	for (const auto Itr : MeshComponents)
	{
		if (Itr && Itr->ComponentHasTag(Tag))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UPrimitiveComponent* UActorInteractableComponentBase::FindPrimitiveByName(const FName Name) const
{
	if (!GetOwner()) return nullptr;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetOwner()->GetComponents(PrimitiveComponents);

	for (const auto Itr : PrimitiveComponents)
	{
		if (Itr && Itr->GetName().Equals(Name.ToString()))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

UPrimitiveComponent* UActorInteractableComponentBase::FindPrimitiveByTag(const FName Tag) const
{
	if (!GetOwner()) return nullptr;

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetOwner()->GetComponents(PrimitiveComponents);

	for (const auto Itr : PrimitiveComponents)
	{
		if (Itr && Itr->ComponentHasTag(Tag))
		{
			return Itr;
		}
	}
	
	return nullptr;
}

TArray<FName> UActorInteractableComponentBase::GetCollisionOverrides() const
{	return CollisionOverrides;}

TArray<FName> UActorInteractableComponentBase::GetHighlightableOverrides() const
{	return HighlightableOverrides;}

void UActorInteractableComponentBase::InteractorFound(const TScriptInterface<IActorInteractorInterface>& FoundInteractor)
{
	if (CanBeTriggered())
	{
		SetInteractor(FoundInteractor);
		
		OnInteractorFoundEvent(FoundInteractor);
	}
}

void UActorInteractableComponentBase::InteractorLost(const TScriptInterface<IActorInteractorInterface>& LostInteractor)
{
	SetState(EInteractableStateV2::EIS_Awake);
	
	SetInteractor(nullptr);
	OnInteractorLostEvent(LostInteractor);
}

void UActorInteractableComponentBase::InteractionCompleted(const float& TimeCompleted)
{
	if (LifecycleMode == EInteractableLifecycle::EIL_Cycled)
	{
		if (TriggerCooldown()) return;
	}
	
	FString ErrorMessage;
	if( CompleteInteractable(ErrorMessage))
	{
		OnInteractionCompletedEvent(TimeCompleted);
	}
	else AIntP_LOG(Display, TEXT("%s"), *ErrorMessage);
}

void UActorInteractableComponentBase::InteractionStarted(const float& TimeStarted, const FKey& PressedKey)
{
	/**
	 * TODO
	 * Validation
	 * If Valid, then Broadcast
	 */

	if (CanInteract())
	{
		OnInteractionStartedEvent(TimeStarted, PressedKey);
	}
}

void UActorInteractableComponentBase::InteractionStopped()
{
	/**
	 * TODO
	 * Validation
	 * If Valid, then Broadcast
	 */
	
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(Timer_Interaction);

	OnInteractionStoppedEvent();
}

void UActorInteractableComponentBase::InteractionCanceled()
{
	if (IsInteracting())
	{
		GetWorld()->GetTimerManager().ClearTimer(Timer_Interaction);

		SetState(EInteractableStateV2::EIS_Awake);
		
		OnInteractionCanceledEvent();
	}
}

void UActorInteractableComponentBase::InteractionLifecycleCompleted()
{
	SetState(EInteractableStateV2::EIS_Completed);

	OnLifecycleCompletedEvent();
}

void UActorInteractableComponentBase::InteractionCooldownCompleted()
{
	if (CanInteract())	SetState(EInteractableStateV2::EIS_Awake);
	else SetState(EInteractableStateV2::EIS_Asleep);
	
	OnCooldownCompletedEvent();
}

void UActorInteractableComponentBase::OnInteractableBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsInteracting()) return;
	if (!OtherActor) return;
	if (!OtherComp) return;

	if (OtherComp->GetCollisionResponseToChannel(CollisionChannel) == ECollisionResponse::ECR_Ignore) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;
	
	for (const auto Itr : InteractorComponents)
	{
		TScriptInterface<IActorInteractorInterface> FoundInteractor;
		if (IgnoredClasses.Contains(Itr->StaticClass())) continue;
		
		FoundInteractor = Itr;
		FoundInteractor.SetObject(Itr);
		FoundInteractor.SetInterface(Cast<IActorInteractorInterface>(Itr));

		switch (FoundInteractor->GetState())
		{
		case EInteractorStateV2::EIS_Active:
		case EInteractorStateV2::EIS_Awake:
			if (FoundInteractor->GetResponseChannel() != GetCollisionChannel()) continue;
			FoundInteractor->GetOnInteractableLostHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableLost);
			OnInteractorOverlapped.Broadcast(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
			OnInteractorFound.Broadcast(FoundInteractor);
			break;
		case EInteractorStateV2::EIS_Asleep:
		case EInteractorStateV2::EIS_Suppressed:
		case EInteractorStateV2::EIS_Disabled:
		case EInteractorStateV2::Default:
			break;
		}
	}
}

void UActorInteractableComponentBase::OnInteractableStopOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;

	for (const auto Itr : InteractorComponents)
	{
		if (Itr == GetInteractor().GetInterface())
		{
			GetInteractor()->GetOnInteractableLostHandle().RemoveDynamic(this, &UActorInteractableComponentBase::InteractableLost);
			GetInteractor()->GetOnInteractableLostHandle().Broadcast(this);
			
			OnInteractorLost.Broadcast(GetInteractor());
			
			SetInteractor(nullptr);
			
			OnInteractorStopOverlap.Broadcast(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
			
			return;
		}
	}
}

void UActorInteractableComponentBase::OnInteractableTraced(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (IsInteracting()) return;
	if (!OtherActor) return;

	TArray<UActorComponent*> InteractorComponents = OtherActor->GetComponentsByInterface(UActorInteractorInterface::StaticClass());

	if (InteractorComponents.Num() == 0) return;
	
	for (const auto Itr : InteractorComponents)
	{
		TScriptInterface<IActorInteractorInterface> FoundInteractor;
		if (IgnoredClasses.Contains(Itr->StaticClass())) continue;
		
		FoundInteractor = Itr;
		FoundInteractor.SetObject(Itr);
		FoundInteractor.SetInterface(Cast<IActorInteractorInterface>(Itr));

		switch (FoundInteractor->GetState())
		{
			case EInteractorStateV2::EIS_Active:
			case EInteractorStateV2::EIS_Awake:
				if (FoundInteractor->CanInteract() == false) return;
				if (FoundInteractor->GetResponseChannel() != GetCollisionChannel()) continue;
				FoundInteractor->GetOnInteractableLostHandle().AddUniqueDynamic(this, &UActorInteractableComponentBase::InteractableLost);
				OnInteractorTraced.Broadcast(HitComponent, OtherActor, OtherComp, NormalImpulse, Hit);
				OnInteractorFound.Broadcast(FoundInteractor);
				break;
			case EInteractorStateV2::EIS_Asleep:
			case EInteractorStateV2::EIS_Suppressed:
			case EInteractorStateV2::EIS_Disabled:
			case EInteractorStateV2::Default:
				break;
		}
	}
}

void UActorInteractableComponentBase::InteractableSelected(const TScriptInterface<IActorInteractableInterface>& Interactable)
{
 	if (Interactable == this)
 	{ 		
 		SetState(EInteractableStateV2::EIS_Active);
 	}
	else SetState(EInteractableStateV2::EIS_Awake);
	
	OnInteractableSelected.Broadcast(Interactable);
}


void UActorInteractableComponentBase::InteractableLost(const TScriptInterface<IActorInteractableInterface>& Interactable)
{
	if (Interactable == this)
	{
		SetState(EInteractableStateV2::EIS_Awake);
		
		OnInteractorLost.Broadcast(GetInteractor());
	}
}

void UActorInteractableComponentBase::FindAndAddCollisionShapes()
{
	for (const auto Itr : CollisionOverrides)
	{
		if (const auto NewCollision = FindPrimitiveByName(Itr))
		{
			AddCollisionComponent(NewCollision);
			BindCollisionShape(NewCollision);
		}
		else
		{
			AIntP_LOG(Error, TEXT("[Actor Interactable Component] Primitive Component '%s' not found!"), *Itr.ToString())
		}
	}
}

void UActorInteractableComponentBase::FindAndAddHighlightableMeshes()
{
	for (const auto Itr : HighlightableOverrides)
	{
		if (const auto NewMesh = FindMeshByName(Itr))
		{
			AddHighlightableComponent(NewMesh);
			BindHighlightableMesh(NewMesh);
		}
		else
		{
			AIntP_LOG(Error, TEXT("[Actor Interactable Component] Mesh Component '%s' not found!"), *Itr.ToString())
		}
	}
}

bool UActorInteractableComponentBase::TriggerCooldown()
{
	const int32 TempRemainingLifecycleCount = RemainingLifecycleCount - 1;
	RemainingLifecycleCount = FMath::Max(0, TempRemainingLifecycleCount);
	
	if (GetWorld())
	{
		if (RemainingLifecycleCount <= 0) return false;

		SetState(EInteractableStateV2::EIS_Cooldown);
		GetWorld()->GetTimerManager().SetTimer
		(
			Timer_Cooldown,
			this,
			&UActorInteractableComponentBase::OnCooldownCompletedEvent,
			CooldownPeriod
		);

		return true;
	}

	return false;
}

void UActorInteractableComponentBase::BindCollisionShape(UPrimitiveComponent* PrimitiveComponent) const
{
	if (!PrimitiveComponent) return;
	
	PrimitiveComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlap);
	PrimitiveComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlap);
	PrimitiveComponent->OnComponentHit.AddUniqueDynamic(this, &UActorInteractableComponentBase::OnInteractableTraced);

	FCollisionShapeCache CachedValues;
	CachedValues.bGenerateOverlapEvents = PrimitiveComponent->GetGenerateOverlapEvents();
	CachedValues.CollisionEnabled = PrimitiveComponent->GetCollisionEnabled();
	CachedValues.CollisionResponse = GetCollisionResponseToChannel(CollisionChannel);
	
	CachedCollisionShapesSettings.Add(PrimitiveComponent, CachedValues);
	
	PrimitiveComponent->SetGenerateOverlapEvents(true);
	PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, ECollisionResponse::ECR_Overlap);

	switch (PrimitiveComponent->GetCollisionEnabled())
	{
		case ECollisionEnabled::NoCollision:
		case ECollisionEnabled::QueryOnly:
		case ECollisionEnabled::PhysicsOnly:
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			break;
		case ECollisionEnabled::QueryAndPhysics:
		default: break;
	}
}

void UActorInteractableComponentBase::UnbindCollisionShape(UPrimitiveComponent* PrimitiveComponent) const
{
	if(!PrimitiveComponent) return;
	
	PrimitiveComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UActorInteractableComponentBase::OnInteractableBeginOverlap);
	PrimitiveComponent->OnComponentEndOverlap.RemoveDynamic(this, &UActorInteractableComponentBase::OnInteractableStopOverlap);
	PrimitiveComponent->OnComponentHit.RemoveDynamic(this, &UActorInteractableComponentBase::OnInteractableTraced);

	if (CachedCollisionShapesSettings.Find(PrimitiveComponent))
	{
		PrimitiveComponent->SetGenerateOverlapEvents(CachedCollisionShapesSettings[PrimitiveComponent].bGenerateOverlapEvents);
		PrimitiveComponent->SetCollisionEnabled(CachedCollisionShapesSettings[PrimitiveComponent].CollisionEnabled);
		PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, CachedCollisionShapesSettings[PrimitiveComponent].CollisionResponse);
	}
	else
	{
		PrimitiveComponent->SetGenerateOverlapEvents(true);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PrimitiveComponent->SetCollisionResponseToChannel(CollisionChannel, ECollisionResponse::ECR_Overlap);
	}
}

void UActorInteractableComponentBase::BindHighlightableMesh(UMeshComponent* MeshComponent) const
{
	if (!MeshComponent) return;
	
	MeshComponent->SetRenderCustomDepth(true);
}

void UActorInteractableComponentBase::UnbindHighlightableMesh(UMeshComponent* MeshComponent) const
{
	if (!MeshComponent) return;
	
	MeshComponent->SetRenderCustomDepth(false);
}

void UActorInteractableComponentBase::AutoSetup()
{
	switch (SetupType)
	{
		case ESetupType::EST_Full:
			{
				// Get all Parent Components
				TArray<USceneComponent*> ParentComponents;
				GetParentComponents(ParentComponents);

				// Iterate over them and assign them properly
				if (ParentComponents.Num() > 0)
				{
					for (const auto Itr : ParentComponents)
					{
						if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(Itr))
						{
							AddCollisionComponent(PrimitiveComp);

							if (UMeshComponent* MeshComp = Cast<UMeshComponent>(PrimitiveComp))
							{
								AddHighlightableComponent(MeshComp);
							}
						}
					}
				}
			}
			break;
		case ESetupType::EST_Quick:
			{
				if (USceneComponent* ParentComponent = GetAttachParent())
				{
					if (UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(ParentComponent))
					{
						AddCollisionComponent(PrimitiveComp);

						if (UMeshComponent* MeshComp = Cast<UMeshComponent>(PrimitiveComp))
						{
							AddHighlightableComponent(MeshComp);
						}
					}
				}
			}
			break;
		default:
			break;
	}
	
	FindAndAddCollisionShapes();
	FindAndAddHighlightableMeshes();
}

#if WITH_EDITOR

void UActorInteractableComponentBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.GetPropertyName() : NAME_None;

	if (PropertyName == "DefaultInteractableState")
	{
		if
		(
			DefaultInteractableState == EInteractableStateV2::EIS_Active ||
			DefaultInteractableState == EInteractableStateV2::EIS_Completed ||
			DefaultInteractableState == EInteractableStateV2::EIS_Cooldown
		)
		{
			const FText ErrorMessage = FText::FromString
			(
				FString("Interactable Component:").Append(TEXT(" DefaultInteractableState cannot be")).Append(GetEnumValueAsString("EInteractableStateV2", DefaultInteractableState)).Append(TEXT("!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));

			DefaultInteractableState = EInteractableStateV2::EIS_Awake;
		}
	}

	if (PropertyName == "LifecycleCount" && LifecycleMode == EInteractableLifecycle::EIL_Cycled)
	{
		if (LifecycleCount == 0 || LifecycleCount == 1)
		{
			const FText ErrorMessage = FText::FromString
			(
				FString("Interactable Component:").Append(TEXT(" Cycled LifecycleCount cannot be: ")).Append(FString::FromInt(LifecycleCount)).Append(TEXT("!"))
			);
			
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));

			LifecycleCount = 2.f;
			RemainingLifecycleCount = LifecycleCount;
		}
	}

	if (PropertyName == "InteractionPeriod")
	{
		if (InteractionPeriod < -1.f)
		{
			const FText ErrorMessage = FText::FromString
			(
				FString("Interactable Component:").Append(TEXT(" InteractionPeriod cannot be less than -1!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));

			InteractionPeriod = -1.f;
		}

		if (InteractionPeriod > -1.f && InteractionPeriod < 0.1f)
		{
			/*
			const FText ErrorMessage = FText::FromString
			(
				FString("Interactable Component:").Append(TEXT(" InteractionPeriod cannot be negative value different than -1!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));
			*/

			InteractionPeriod = 0.1f;
		}

		if (FMath::IsNearlyZero(InteractionPeriod, 0.001f))
		{
			/*
			const FText ErrorMessage = FText::FromString
			(
				FString("Interactable Component:").Append(TEXT(" InteractionPeriod cannot be 0!"))
			);
			FEditorHelper::DisplayEditorNotification(ErrorMessage, SNotificationItem::CS_Fail, 5.f, 2.f, TEXT("Icons.Error"));
			*/

			InteractionPeriod = 0.1f;
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

EDataValidationResult UActorInteractableComponentBase::IsDataValid(TArray<FText>& ValidationErrors)
{
	const auto DefaultValue = Super::IsDataValid(ValidationErrors);
	bool bAnyError = false;
	
	if
	(
		DefaultInteractableState == EInteractableStateV2::EIS_Active ||
		DefaultInteractableState == EInteractableStateV2::EIS_Completed ||
		DefaultInteractableState == EInteractableStateV2::EIS_Cooldown
	)
	{
		const FText ErrorMessage = FText::FromString
		(
			FString("Interactable Component:").Append(TEXT(" DefaultInteractableState cannot be")).Append(GetEnumValueAsString("EInteractableStateV2", DefaultInteractableState)).Append(TEXT("!"))
		);

		DefaultInteractableState = EInteractableStateV2::EIS_Awake;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}

	if (InteractionPeriod < -1.f)
	{
		const FText ErrorMessage = FText::FromString
		(
			FString("Interactable Component: DefaultInteractableState cannot be lesser than -1!")
		);

		InteractionPeriod = -1.f;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}
	
	if (LifecycleMode == EInteractableLifecycle::EIL_Cycled && (LifecycleCount == 0 || LifecycleCount == 1))
	{
		const FText ErrorMessage = FText::FromString
		(
			FString("Interactable Component:").Append(TEXT(" LifecycleCount cannot be %d!"), LifecycleCount)
		);
			
		LifecycleCount = 2.f;
		RemainingLifecycleCount = LifecycleCount;
		
		ValidationErrors.Add(ErrorMessage);
		bAnyError = true;
	}
	
	return bAnyError ? EDataValidationResult::Invalid : DefaultValue;
}

void UActorInteractableComponentBase::DrawDebug()
{
	if (bToggleDebug)
	{
		for (const auto Itr : CollisionComponents)
		{
			Itr->SetHiddenInGame(false);
		}
	}
}

#endif
#pragma endregion