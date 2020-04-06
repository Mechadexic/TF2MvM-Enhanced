//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_lunchbox.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_powerup.h"
#else
#include "c_tf_player.h"
#include "c_tf_viewmodeladdon.h"
#endif

CREATE_SIMPLE_WEAPON_TABLE( TFLunchBox, tf_weapon_lunchbox )

#define TF_SANDVICH_PLATE_MODEL "models/items/plate.mdl"
#define TF_ROBOSANDVICH_PLATE_MODEL "models/items/plate_robo_sandwich.mdl"
#define TF_SANDVICH_FESTIVE_PLATE_MODEL "models/items/plate_sandwich_xmas.mdl"
#define TF_DALOKOH_PLATE_MODEL "models/workshop/weapons/c_models/c_chocolate/plate_chocolate.mdl"
#define TF_FISHCAKE_PLATE_MODEL "models/workshop/weapons/c_models/c_fishcake/plate_fishcake.mdl"
#define TF_STEAK_PLATE_MODEL "models/items/plate_steak.mdl"
#define TF_BANANA_PLATE_MODEL "models/items/banana/plate_banana.mdl"
#define SANDVICH_BODYGROUP_BITE 0
#define SANDVICH_STATE_BITTEN 1
#define SANDVICH_STATE_NORMAL 0

ConVar tf2v_new_sandvich_behavior( "tf2v_new_sandvich_behavior", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Use Gun Mettle rebalancing on sandviches." );

//-----------------------------------------------------------------------------
// Purpose: Give us a fresh sandwich.
//-----------------------------------------------------------------------------
CTFLunchBox::CTFLunchBox()
{
	m_bBitten = false;
	m_flBiteTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::PrimaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
	{
		return;
	}

#ifdef GAME_DLL
	pOwner->Taunt();
#endif
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	
	BiteLunch();
}

//-----------------------------------------------------------------------------
// Purpose: Times our bite to make it look more authentic.
//-----------------------------------------------------------------------------
void CTFLunchBox::BiteLunch( void )
{
	if (m_bBitten)	//If we already bit the sandwich, this is redundant.
		return;
	
	// Our bite happens around the 25th frame of animation.
	ConVarRef host_timescale( "host_timescale" );
	m_flBiteTime = gpGlobals->curtime + ( (25 / 30) / host_timescale.GetFloat() );

	SetNextThink( gpGlobals->curtime + ( 1 / 30 ) );
	SetThink( &CTFLunchBox::BiteLunchThink );

	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::BiteLunchThink( void )
{
	// Intentionally wait before doing our action.
	if ( gpGlobals->curtime > m_flBiteTime )
	{
		// We waited for the bite, switch bodygroups.
		m_bBitten = true;
		m_flBiteTime = 0;
		SwitchBodyGroups();	
		return;
	}
	else
	{
		SetNextThink( gpGlobals->curtime + ( 1 / 30 ) );
		SetThink( &CTFLunchBox::BiteLunchThink );
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::SecondaryAttack( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	if ( !CanAttack() || !CanDrop() )
		return;

#ifdef GAME_DLL
	// Remove the previous dropped lunch box.
	if ( m_hDroppedLunch.Get() )
	{
		UTIL_Remove( m_hDroppedLunch.Get() );
		m_hDroppedLunch = NULL;
	}

	// Throw a sandvich plate down on the ground.
	Vector vecSrc, vecThrow;
	QAngle angThrow;
	vecSrc = pOwner->EyePosition();

	// A bit below the eye position.
	vecSrc.z -= 8.0f;

	const char *pszItemName = "item_healthkit_medium";

	int nLunchboxAddsMaxHealth = 0;
	CALL_ATTRIB_HOOK_INT( nLunchboxAddsMaxHealth, set_weapon_mode );
	if ( ( nLunchboxAddsMaxHealth == 1 ) || ( nLunchboxAddsMaxHealth == 6 ) || ( nLunchboxAddsMaxHealth == 7 ) ) // Chocolate, Fishcake and Banana are small health drops
		pszItemName = "item_healthkit_small";

	CTFPowerup *pPowerup = static_cast<CTFPowerup *>( CBaseEntity::Create( pszItemName, vecSrc, vec3_angle, pOwner ) );
	if ( !pPowerup )
		return;

	// Don't collide with the player owner for the first portion of its life
	pPowerup->m_flNextCollideTime = gpGlobals->curtime + 0.5f;
	
	switch ( nLunchboxAddsMaxHealth)
	{
		case 1:
			pPowerup->SetModel( TF_DALOKOH_PLATE_MODEL );
			break;
		case 2:
			pPowerup->SetModel( TF_STEAK_PLATE_MODEL );
			break;
		case 3:
			pPowerup->SetModel( TF_ROBOSANDVICH_PLATE_MODEL );
			break;
		case 4:
			pPowerup->SetModel( TF_SANDVICH_FESTIVE_PLATE_MODEL );
			break;
		case 6:
			pPowerup->SetModel( TF_BANANA_PLATE_MODEL );
			break;
		case 7:
			pPowerup->SetModel( TF_FISHCAKE_PLATE_MODEL );
			break;
		default:
			if ( GameRules()->IsHolidayActive( kHoliday_Christmas ) )
				pPowerup->SetModel( TF_SANDVICH_FESTIVE_PLATE_MODEL );
			else
				pPowerup->SetModel( TF_SANDVICH_PLATE_MODEL );
			break;
	}
	
	UTIL_SetSize( pPowerup, -Vector( 17, 17, 10 ), Vector( 17, 17, 10 ) );

	// Throw it down.
	angThrow = pOwner->EyeAngles();
	angThrow[PITCH] -= 10.0f;
	AngleVectors( angThrow, &vecThrow );
	vecThrow *= 500;

	pPowerup->DropSingleInstance( vecThrow, pOwner, 0.3f, 0.1f );

	m_hDroppedLunch = pPowerup;
#endif

	// Switch away from it immediately, don't want it to stick around.
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	pOwner->SwitchToNextBestWeapon( this );

	StartEffectBarRegen();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::DepleteAmmo( void )
{
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if ( !pOwner )
		return;

	int nLunchboxAddsMaxHealth = 0;
	CALL_ATTRIB_HOOK_INT( nLunchboxAddsMaxHealth, set_weapon_mode );
	
	if ( !tf2v_new_sandvich_behavior.GetBool() && ( nLunchboxAddsMaxHealth == 1 ) || ( nLunchboxAddsMaxHealth == 7 ) )
	{
		return;
	}

	if ( pOwner->HealthFraction() >= 1.0f && ( nLunchboxAddsMaxHealth != 2 ) )
		return;

	// Switch away from it immediately, don't want it to stick around.
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
	pOwner->SwitchToNextBestWeapon( this );

	StartEffectBarRegen();
}

bool CTFLunchBox::UsesPrimaryAmmo( void )
{
	if ( !tf2v_new_sandvich_behavior.GetBool() )
	{
		if ( (CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 1) || (CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 7) )
			return false;
	}

	return BaseClass::UsesPrimaryAmmo();
}

float CTFLunchBox::InternalGetEffectBarRechargeTime( void )
{
	// If we're using the Dalokoh, regen in 10 seconds.
	if ( (CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 1) || (CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 7) )
		return 10.0f;
	
	// Everything else is 30 seconds.
	return 30.0f;
	
	
}

//-----------------------------------------------------------------------------
// Purpose: Update the sandvich bite effects
//-----------------------------------------------------------------------------
void CTFLunchBox::SwitchBodyGroups( void )
{
#ifndef GAME_DLL
	C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
	if ( pAttach )
	{
		int iState = m_bBitten ? SANDVICH_STATE_BITTEN : SANDVICH_STATE_NORMAL;
		pAttach->SetBodygroup( SANDVICH_BODYGROUP_BITE, iState );
	}
#endif
}

void CTFLunchBox::WeaponRegenerate()
{
	m_bBitten = false;
	SetContextThink( &CTFLunchBox::SwitchBodyGroups, gpGlobals->curtime + 0.01f, "SwitchBodyGroups" );
	BaseClass::WeaponRegenerate();
}

void CTFLunchBox::WeaponReset()
{
	m_bBitten = false;
	BaseClass::WeaponReset();
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::Precache( void )
{
	UTIL_PrecacheOther( "item_healthkit_medium" );
	PrecacheModel( TF_SANDVICH_PLATE_MODEL );
	PrecacheModel( TF_ROBOSANDVICH_PLATE_MODEL );
	PrecacheModel( TF_SANDVICH_FESTIVE_PLATE_MODEL );
	PrecacheModel( TF_DALOKOH_PLATE_MODEL );
	PrecacheModel( TF_FISHCAKE_PLATE_MODEL );
	PrecacheModel( TF_STEAK_PLATE_MODEL );
	PrecacheModel( TF_BANANA_PLATE_MODEL );
						
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLunchBox::ApplyBiteEffects( bool bHurt )
{
	bool bIsSteak = ( CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 2);
	
	if ( !bHurt && !bIsSteak )
		return;

	// Heal 25% of the player's max health per second for a total of 100%.
	CTFPlayer *pOwner = GetTFPlayerOwner();
	
	bool bIsChocolate = ( CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 1) || (CAttributeManager::AttribHookValue<int>( 0, "set_weapon_mode", this ) == 7);

	float flAmt = 75.0;
	if ( bIsChocolate )
		flAmt = 25.0;
	
	// Adjust our healing scale if defined.
	CALL_ATTRIB_HOOK_FLOAT( flAmt, lunchbox_healing_scale );

	if ( pOwner && !bIsSteak )
	{
		pOwner->TakeHealth( flAmt, DMG_GENERIC );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFLunchBox::CanDrop( void ) const
{
	if ( !tf2v_new_sandvich_behavior.GetBool() )
	{
		int nSetLunchboxMode = 0;
		CALL_ATTRIB_HOOK_INT( nSetLunchboxMode, set_weapon_mode );
		if ( ( nSetLunchboxMode == 1 ) || ( nSetLunchboxMode == 7 ) )
			return false;
	}
	
	CTFPlayer *pOwner = GetTFPlayerOwner();
	if (pOwner && pOwner->IsAlive())
		return !pOwner->m_Shared.InCond( TF_COND_TAUNTING );

	return false;
}

#endif
