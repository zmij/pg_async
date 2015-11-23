// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

AAwmProjectile::AAwmProjectile(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PROJECTILE);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	RootComponent = CollisionComp;

	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	ParticleComp->bAutoActivate = true;
	ParticleComp->bAutoDestroy = false;
	ParticleComp->AttachParent = RootComponent;

	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;

	HitLifeSpan = 2.0f;
	DefaultLifeSpan = 10.0f;
}

void AAwmProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	MovementComp->OnProjectileStop.AddDynamic(this, &AAwmProjectile::OnImpact);
	CollisionComp->MoveIgnoreActors.Add(Instigator);

	// Override config from character
	// @todo
	/*AAwmBattleChar* OwnerChar = Cast<AAwmBattleChar>(GetOwner());
	if (OwnerChar) {
		OwnerChar->ApplyWeaponConfig(WeaponConfig);
	}
	else {
		// Apply defaul life span
		SetLifeSpan(DefaultLifeSpan);
	}*/
	
	MyController = GetInstigatorController();
}

void AAwmProjectile::InitVelocity(FVector ShootDirection)
{
	if (MovementComp)
	{
		// @WTF Why we should remove it? Is velocity relative?
		//MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AAwmProjectile::SetWeaponConfig(const FProjectileWeaponData& Data)
{
	WeaponConfig = Data;

	SetLifeSpan(WeaponConfig.ProjectileLife);
}

void AAwmProjectile::OnImpact(const FHitResult& HitResult)
{
	if (Role == ROLE_Authority && !bExploded)
	{
		Explode(HitResult);
		DisableAndDestroy();
	}
}

void AAwmProjectile::Explode(const FHitResult& Impact)
{
	if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}

	// Effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
	{
		UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());
	}

	if (ExplosionTemplate)
	{
		const FRotator SpawnRotation = Impact.ImpactNormal.Rotation();

		AAwmExplosionEffect* EffectActor = GetWorld()->SpawnActorDeferred<AAwmExplosionEffect>(ExplosionTemplate, FTransform(SpawnRotation, NudgedImpactLocation));
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(SpawnRotation, NudgedImpactLocation));
		}
	}

	bExploded = true;
}

void AAwmProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	MovementComp->StopMovementImmediately();

	// Remove collision
	if (CollisionComp) {
		CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	}

	// Give clients some time to show explosion
	SetLifeSpan(HitLifeSpan);
}

void AAwmProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorRotation().Vector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;
	
	if (!GetWorld()->LineTraceSingleByChannel(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator))) {
		// Failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}

void AAwmProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AAwmProjectile::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AAwmProjectile, bExploded );
}
