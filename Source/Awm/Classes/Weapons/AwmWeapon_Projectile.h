// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmWeapon.h"
#include "AwmWeapon_Projectile.generated.h"

// A weapon that fires a visible projectile
UCLASS(Abstract)
class AAwmWeapon_Projectile : public AAwmWeapon
{
	GENERATED_UCLASS_BODY()

	/** apply config on projectile */
	void ApplyWeaponConfig(FProjectileWeaponData& Data);

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ERocket;
	}

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category=Config)
	FProjectileWeaponData ProjectileConfig;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};
