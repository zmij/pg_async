// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmProjectile.generated.h"

UCLASS(Abstract, Blueprintable, HideCategories = (Tick, Rendering, Replication, Input, Actor))
class AAwmProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Initial setup */
	virtual void PostInitializeComponents() override;

	/** Setup velocity */
	UFUNCTION(BlueprintCallable, Category = "Awm|Weapon|Projectile")
	void InitVelocity(FVector ShootDirection);

	/** Apply weapon config */
	UFUNCTION(BlueprintCallable, Category = "Awm|Weapon|Projectile")
	void SetWeaponConfig(const FProjectileWeaponData& Data);

	/** Handle hit */
	UFUNCTION(BlueprintCallable, Category = "Awm|Weapon|Projectile")
	void OnImpact(const FHitResult& HitResult);

protected:

	/** Movement component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UProjectileMovementComponent* MovementComp;

	/** Collisions */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UParticleSystemComponent* ParticleComp;

	/** Effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<class AAwmExplosionEffect> ExplosionTemplate;

	/** Default life span (could be overwritten by weapon!) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Effects)
	float DefaultLifeSpan;

	/** Time to live after the hit */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category=Effects)
	float HitLifeSpan;

	/** Controller that fired me (cache for damage calculations) */
	UPROPERTY()
	TWeakObjectPtr<AController> MyController;

	/** Projectile data */
	UPROPERTY(Transient)
	struct FProjectileWeaponData WeaponConfig;

	/** Did it explode? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Exploded)
	bool bExploded;

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** Trigger explosion */
	void Explode(const FHitResult& Impact);

	/** Shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** Update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;


};
