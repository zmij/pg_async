// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "AwmTypes.h"
#include "AwmImpactEffect.generated.h"

/**
 * Spawnable effect for weapon hit impact - NOT replicated to clients
 * Each impact type should be defined as separate blueprint
 */
UCLASS(Abstract, Blueprintable)
class AAwmImpactEffect : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Default impact FX used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	UParticleSystem* DefaultFX;

	/** Impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* ConcreteFX;

	/** Impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* DirtFX;

	/** Impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* WaterFX;

	/** Impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* MetalFX;

	/** Impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* WoodFX;

	/** Impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* GlassFX;

	/** Impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* GrassFX;

	/** Impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Visual)
	UParticleSystem* FleshFX;

	/** Default impact sound used when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	USoundCue* DefaultSound;

	/** Impact FX on concrete */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* ConcreteSound;

	/** Impact FX on dirt */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* DirtSound;

	/** Impact FX on water */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* WaterSound;

	/** Impact FX on metal */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* MetalSound;

	/** Impact FX on wood */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* WoodSound;

	/** Impact FX on glass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* GlassSound;

	/** Impact FX on grass */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* GrassSound;

	/** Impact FX on flesh */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FleshSound;

	/** Default decal when material specific override doesn't exist */
	UPROPERTY(EditDefaultsOnly, Category=Defaults)
	struct FDecalData DefaultDecal;

	/** Surface data for spawning */
	UPROPERTY(BlueprintReadOnly, Category=Surface)
	FHitResult SurfaceHit;

	/** Spawn effect */
	virtual void PostInitializeComponents() override;

protected:
	/** Get FX for material type */
	UParticleSystem* GetImpactFX(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

	/** Get sound for material type */
	USoundCue* GetImpactSound(TEnumAsByte<EPhysicalSurface> SurfaceType) const;

};
