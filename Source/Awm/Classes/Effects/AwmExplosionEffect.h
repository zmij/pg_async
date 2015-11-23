// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "AwmTypes.h"
#include "AwmExplosionEffect.generated.h"

/**
 * Spawnable effect for explosion - NOT replicated to clients
 * Each explosion type should be defined as separate blueprint
 */
UCLASS(Abstract, Blueprintable, HideCategories = (Tick, Rendering, Replication, Input, Actor))
class AAwmExplosionEffect : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Explosion FX */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	UParticleSystem* ExplosionFX;

	/** Explosion light */
	UPROPERTY(VisibleDefaultsOnly, Category=Effect)
	UPointLightComponent* ExplosionLight;

	/** How long keep explosion light on? */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	float ExplosionLightFadeOut;

	/** Explosion sound */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	USoundCue* ExplosionSound;
	
	/** Explosion decals */
	UPROPERTY(EditDefaultsOnly, Category=Effect)
	struct FDecalData Decal;

	/** Surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;

	/** Spawn explosion */
	virtual void BeginPlay() override;

	/** Update fading light */
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Point light component name */
	FName ExplosionLightComponentName;

};
