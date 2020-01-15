//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_grenadelauncher.h"
#include "tf_fx_shared.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Grenade Launcher tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeLauncher, DT_WeaponGrenadeLauncher )

BEGIN_NETWORK_TABLE( CTFGrenadeLauncher, DT_WeaponGrenadeLauncher )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeLauncher )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenadelauncher, CTFGrenadeLauncher );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenadelauncher );

//=============================================================================

// Server specific.
#ifndef CLIENT_DLL
BEGIN_DATADESC( CTFGrenadeLauncher )
END_DATADESC()
#endif

#define TF_GRENADE_LAUNCER_MIN_VEL 1200
#define TF_GRENADES_SWITCHGROUP 2 
#define TF_GRENADE_BARREL_SPIN 0.25 // barrel increments by one quarter for each pill


CREATE_SIMPLE_WEAPON_TABLE( TFGrenadeLauncher_Legacy, tf_weapon_grenadelauncher_legacy )

extern ConVar tf2v_console_grenadelauncher;

//=============================================================================
//
// Weapon Grenade Launcher functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::CTFGrenadeLauncher()
{
	m_bReloadsSingly = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CTFGrenadeLauncher::~CTFGrenadeLauncher()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_GRENADELAUNCHER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we holster
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the charge when we deploy
//-----------------------------------------------------------------------------
bool CTFGrenadeLauncher::Deploy( void )
{
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetMaxClip1( void ) const
{
#ifdef _X360 
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetMaxClip1() but with the base set to TF_GRENADE_LAUNCHER_XBOX_CLIP.
	// We need to do it this way in order to consider attributes.
	if ( tf2v_console_grenadelauncher.GetBool() )
	{
		
		int iMaxClip = TF_GRENADE_LAUNCHER_XBOX_CLIP;
		if ( iMaxClip < 0 )
			return iMaxClip;

		CALL_ATTRIB_HOOK_FLOAT( iMaxClip, mult_clipsize );
		if ( iMaxClip < 0 )
			return iMaxClip;

		CTFPlayer *pOwner = GetTFPlayerOwner();
		if ( pOwner == NULL )
			return iMaxClip;

		int nClipSizePerKill = 0;
		CALL_ATTRIB_HOOK_INT( nClipSizePerKill, clipsize_increase_on_kill );

		iMaxClip += Min( nClipSizePerKill, pOwner->m_Shared.GetStrikeCount() );

		return iMaxClip;

	}

	return BaseClass::GetMaxClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFGrenadeLauncher::GetDefaultClip1( void ) const
{
#ifdef _X360
	return TF_GRENADE_LAUNCHER_XBOX_CLIP;
#endif

	// BaseClass::GetDefaultClip1() is just checking GetMaxClip1(), nothing fancy to do here.
	if ( tf2v_console_grenadelauncher.GetBool() )
		return GetMaxClip1();
	 
	return BaseClass::GetDefaultClip1();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::PrimaryAttack( void )
{
	// Check for ammunition.
	if ( m_iClip1 <= 0 && m_iClip1 != -1 )
		return;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanAttack() )
	{
		return;
	}

	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	
	LaunchGrenade();
	SwitchBodyGroups();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::WeaponIdle( void )
{
	BaseClass::WeaponIdle();
}

CBaseEntity *CTFGrenadeLauncher::FireProjectileInternal( CTFPlayer *pPlayer )
{
	CTFWeaponBaseGrenadeProj *pGrenade = (CTFWeaponBaseGrenadeProj *)FireProjectile( pPlayer );
	/*if ( pGrenade )
	{
		if ( GetDetonateMode() == TF_GL_MODE_FIZZLE )
			pGrenade->m_bFizzle = true;
	}*/
	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::LaunchGrenade( void )
{
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	CalcIsAttackCritical();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	FireProjectileInternal(pPlayer);

#if 0
	CBaseEntity *pPipeBomb = 

	if (pPipeBomb)
	{
#ifdef GAME_DLL
		// If we've gone over the max pipebomb count, detonate the oldest
		if (m_Pipebombs.Count() >= TF_WEAPON_PIPEBOMB_COUNT)
		{
			CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[0];
			if (pTemp)
			{
				pTemp->SetTimer(gpGlobals->curtime); // explode NOW
			}

			m_Pipebombs.Remove(0);
		}

		CTFGrenadePipebombProjectile *pPipebomb = (CTFGrenadePipebombProjectile*)pProjectile;
		pPipebomb->SetLauncher(this);

		PipebombHandle hHandle;
		hHandle = pPipebomb;
		m_Pipebombs.AddToTail(hHandle);

		m_iPipebombCount = m_Pipebombs.Count();
#endif
	}
#endif

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set next attack times.
	float flDelay = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CALL_ATTRIB_HOOK_FLOAT( flDelay, mult_postfiredelay );
	m_flNextPrimaryAttack = gpGlobals->curtime + flDelay;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );

	// Check the reload mode and behave appropriately.
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}

float CTFGrenadeLauncher::GetProjectileSpeed( void )
{
	float flVelocity = TF_GRENADE_LAUNCER_MIN_VEL;

	CALL_ATTRIB_HOOK_FLOAT( flVelocity, mult_projectile_speed );

	return flVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: Detonate this demoman's pipebombs
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SecondaryAttack( void )
{
	BaseClass::SecondaryAttack();
}

bool CTFGrenadeLauncher::Reload( void )
{
	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: Change model state to reflect available pills in launcher
//-----------------------------------------------------------------------------
void CTFGrenadeLauncher::SwitchBodyGroups( void )
{
	if ( GetNumBodyGroups() < TF_GRENADES_SWITCHGROUP )
		return; 

    int iState = 4;

    iState = m_iClip1;

    SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
	SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );

    CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );

    if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
    {
		CBaseViewModel *vm = pTFPlayer->GetViewModel( m_nViewModelIndex );
        if ( vm )
        {
            vm->SetBodygroup( TF_GRENADES_SWITCHGROUP, iState );
			vm->SetPoseParameter( "barrel_spin", TF_GRENADE_BARREL_SPIN * iState );
        }
    }
}

int CTFGrenadeLauncher::GetDetonateMode( void ) const
{
	int nDetonateMode = 0;
	CALL_ATTRIB_HOOK_INT( nDetonateMode, set_detonate_mode );
	return nDetonateMode;
}
