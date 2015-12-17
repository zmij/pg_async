// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmWeapon::AAwmWeapon(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh"));
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh->bReceivesDecals = false;
	Mesh->CastShadow = true;
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	RootComponent = Mesh;

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

//////////////////////////////////////////////////////////////////////////
// Initialization

void AAwmWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
		CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
	}

	DetachMeshFromPawn();
}

void AAwmWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}


//////////////////////////////////////////////////////////////////////////
// Inventory

void AAwmWeapon::OnEquip(const AAwmWeapon* LastWeapon)
{
	AttachMeshToPawn();

	bPendingEquip = true;
	DetermineWeaponState();

	// Only play animation if last weapon is valid
	if (LastWeapon)
	{
		float Duration = PlayWeaponAnimation(EquipAnim);
		if (Duration <= 0.0f)
		{
			// failsafe
			Duration = 0.5f;
		}
		EquipStartedTime = GetWorld()->GetTimeSeconds();
		EquipDuration = Duration;

		GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &AAwmWeapon::OnEquipFinished, Duration, false);
	}
	else
	{
		OnEquipFinished();
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void AAwmWeapon::OnEquipFinished()
{
	AttachMeshToPawn();

	bIsEquipped = true;
	bPendingEquip = false;

	// Determine the state so that the can reload checks will work
	DetermineWeaponState(); 
	
	if (MyPawn)
	{
		// try to reload empty clip
		if (MyPawn->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
}

void AAwmWeapon::OnUnEquip()
{
	DetachMeshFromPawn();
	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void AAwmWeapon::OnEnterInventory(AAwmVehicle* NewOwner)
{
	SetVehicleOwner(NewOwner);
}

void AAwmWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetVehicleOwner(NULL);
	}

	if (IsAttachedToPawn())
	{
		OnUnEquip();
	}
}

void AAwmWeapon::AttachMeshToPawn()
{
	if (MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = MyPawn->GetWeaponAttachPoint();

		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		USkeletalMeshComponent* UsePawnMesh = MyPawn->GetMesh();
		UseWeaponMesh->AttachTo(UsePawnMesh, AttachPoint);
		UseWeaponMesh->SetHiddenInGame(false);
	}
}

void AAwmWeapon::DetachMeshFromPawn()
{
	Mesh->DetachFromParent();
	Mesh->SetHiddenInGame(true);
}


//////////////////////////////////////////////////////////////////////////
// Targeting and rotation

void AAwmWeapon::SetTurretYaw(float YawRotation)
{
	TurretYaw = YawRotation;
}

void AAwmWeapon::SetTurretPitch(float PitchRotation)
{
	TurretPitch = PitchRotation;
}

void AAwmWeapon::SetCameraViewLocation(FVector ViewLocation)
{
	CameraViewLocation = ViewLocation;

	if (Role < ROLE_Authority)
	{
		ServerSetCameraViewLocation(ViewLocation);
	}
}

bool AAwmWeapon::ServerSetCameraViewLocation_Validate(FVector ViewLocation)
{
	return true;
}

void AAwmWeapon::ServerSetCameraViewLocation_Implementation(FVector ViewLocation)
{
	SetCameraViewLocation(ViewLocation);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAwmWeapon::StartFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void AAwmWeapon::StopFire()
{
	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

void AAwmWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);		
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AAwmWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AAwmWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}
		
		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void AAwmWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool AAwmWeapon::ServerStartFire_Validate()
{
	return true;
}

void AAwmWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AAwmWeapon::ServerStopFire_Validate()
{
	return true;
}

void AAwmWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AAwmWeapon::ServerStartReload_Validate()
{
	return true;
}

void AAwmWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AAwmWeapon::ServerStopReload_Validate()
{
	return true;
}

void AAwmWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void AAwmWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

void AAwmWeapon::LockTarget()
{
	if (Role < ROLE_Authority)
	{
		ServerLockTarget();
	}
	LockedTarget = LockTargetBP();
}

void AAwmWeapon::ServerLockTarget_Implementation()
{
	LockTarget();
}

bool AAwmWeapon::ServerLockTarget_Validate()
{
	return true;
}


//////////////////////////////////////////////////////////////////////////
// Control

bool AAwmWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );	
	return (( bCanFire == true ) && ( bStateOKToFire == true ) && ( bPendingReload == false ));
}

bool AAwmWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = ( CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );
	return ( ( bCanReload == true ) && ( bGotAmmo == true ) && ( bStateOKToReload == true) );	
}


//////////////////////////////////////////////////////////////////////////
// Ammo

void AAwmWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	AAwmAIController* BotAI = MyPawn ? Cast<AAwmAIController>(MyPawn->GetController()) : NULL;
	if (BotAI)
	{
		// @optional
		//BotAI->CheckAmmo(this);
	}
	
	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 &&
		CanReload() &&
		MyPawn->GetWeapon() == this)
	{
		ClientStartReload();
	}
}

void AAwmWeapon::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}

	// @optional Any notifiers to bot or pawn about ammo usage
}

int32 AAwmWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 AAwmWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 AAwmWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 AAwmWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool AAwmWeapon::HasInfiniteAmmo() const
{
	const AAwmPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AAwmPlayerController>(MyPawn->Controller) : NULL;
	// @todo
	return WeaponConfig.bInfiniteAmmo;// || (MyPC && MyPC->HasInfiniteAmmo());
}

bool AAwmWeapon::HasInfiniteClip() const
{
	const AAwmPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AAwmPlayerController>(MyPawn->Controller) : NULL;
	// @todo
	return WeaponConfig.bInfiniteClip;// || (MyPC && MyPC->HasInfiniteClip());
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AAwmWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			FireWeapon();

			UseAmmo();
			
			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			AAwmPlayerController* MyPC = Cast<AAwmPlayerController>(MyPawn->Controller);
			AAwmHUD* MyHUD = MyPC ? Cast<AAwmHUD>(MyPC->GetHUD()) : NULL;
			if (MyHUD)
			{
				MyHUD->NotifyOutOfAmmo();
			}
		}
		
		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AAwmWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool AAwmWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AAwmWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

void AAwmWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}

void AAwmWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AAwmWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AAwmWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}
	
	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}

void AAwmWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void AAwmWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}


//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AAwmWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory(MyPawn);
	}
	else
	{
		OnLeaveInventory();
	}
}

void AAwmWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AAwmWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void AAwmWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	if (MuzzleFX)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((MyPawn != NULL) && (MyPawn->IsLocallyControlled() == true))
			{
				AController* PlayerCon = MyPawn->GetController();
				if (PlayerCon != NULL)
				{
					Mesh->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh, MuzzleAttachPoint);
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, UseWeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	AAwmPlayerController* PC = (MyPawn != NULL) ? Cast<AAwmPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
	}
}

void AAwmWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void AAwmWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAwmWeapon, MyPawn);

	// Only to local owner
	DOREPLIFETIME_CONDITION(AAwmWeapon, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAwmWeapon, CurrentAmmoInClip, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAwmWeapon, LockedTarget, COND_OwnerOnly);

	// Everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AAwmWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AAwmWeapon, bPendingReload, COND_SkipOwner);

	// Everyone
	DOREPLIFETIME(AAwmWeapon, TurretYaw);
	DOREPLIFETIME(AAwmWeapon, TurretPitch);
}


//////////////////////////////////////////////////////////////////////////
// Reading data

USkeletalMeshComponent* AAwmWeapon::GetWeaponMesh() const
{
	return Mesh;
}

class AAwmVehicle* AAwmWeapon::GetVehicleOwner() const
{
	return MyPawn;
}

void AAwmWeapon::SetVehicleOwner(AAwmVehicle* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;

		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

bool AAwmWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AAwmWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

EWeaponState::Type AAwmWeapon::GetCurrentState() const
{
	return CurrentState;
}

FVector AAwmWeapon::GetCameraViewLocation() const
{
	return CameraViewLocation;
}

AAwmVehicle* AAwmWeapon::GetLockedTarget() const
{
	return LockedTarget;
}

float AAwmWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AAwmWeapon::GetEquipDuration() const
{
	return EquipDuration;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* AAwmWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

float AAwmWeapon::PlayWeaponAnimation(const UAnimMontage* Animation)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		// @todo Play weapon animation
	}

	return Duration;
}

void AAwmWeapon::StopWeaponAnimation(const UAnimMontage* Animation)
{
	if (MyPawn)
	{
		// @todo Stop weapon animation
	}
}

FVector AAwmWeapon::GetCameraAim() const
{
	AAwmPlayerController* const PlayerController = Instigator ? Cast<AAwmPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();		
	}

	return FinalAim;
}

FVector AAwmWeapon::GetAdjustedAim_Implementation()
{
	AAwmPlayerController* const PlayerController = Instigator ? Cast<AAwmPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;
	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		AAwmAIController* AIController = MyPawn ? Cast<AAwmAIController>(MyPawn->Controller) : NULL;
		if(AIController != NULL )
		{
			FinalAim = AIController->GetControlRotation().Vector();
		}
		else
		{			
			FinalAim = Instigator->GetBaseAimRotation().Vector();
		}
	}

	return FinalAim;
}

FVector AAwmWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	// In AWM we trust the muzzle
	return GetMuzzleLocation();
}

FVector AAwmWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector AAwmWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult AAwmWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}
