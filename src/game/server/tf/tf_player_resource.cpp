//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "player_resource.h"
#include "tf_player_resource.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TFPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 13 ) ),
	SendPropArray3(SENDINFO_ARRAY3( m_iDomination ), SendPropInt( SENDINFO_ARRAY( m_iDomination ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxBuffedHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxBuffedHealth ), 32, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iColors ), SendPropVector( SENDINFO_ARRAY3( m_iColors ), 12, SPROP_COORD ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iKillstreak ), SendPropInt( SENDINFO_ARRAY( m_iKillstreak ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bArenaSpectator ), SendPropBool( SENDINFO_ARRAY( m_bArenaSpectator ) ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

CTFPlayerResource::CTFPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerResource::UpdatePlayerData( void )
{
	BaseClass::UpdatePlayerData();

	for ( int i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{			
			PlayerStats_t *pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
			if ( pPlayerStats ) 
			{
				m_iMaxHealth.Set( i, pPlayer->GetMaxHealth() );
				m_iMaxBuffedHealth.Set( i, pPlayer->GetMaxHealthForBuffing() );

				m_iPlayerClass.Set( i, pPlayer->GetPlayerClass()->GetClassIndex() );

				int iTotalScore = CTFGameRules::CalcPlayerScore( &pPlayerStats->statsAccumulated );
				m_iTotalScore.Set( i, iTotalScore );

				m_iColors.Set( i, pPlayer->m_vecPlayerColor );

				m_iKillstreak.Set( i, pPlayer->m_Shared.GetKillstreak( 0 ) + pPlayer->m_Shared.GetKillstreak( 1 ) + pPlayer->m_Shared.GetKillstreak( 2 ) );
			}	

			m_iDomination.Set( i, pPlayer->m_Shared.GetDominationCount() );

			m_bArenaSpectator.Set( i, pPlayer->IsArenaSpectator() );
		}
	}
}

void CTFPlayerResource::Spawn( void )
{
	for ( int i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iDomination.Set( i, 0 );
		m_iTotalScore.Set( i, 0 );
		m_iMaxHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iMaxBuffedHealth.Set( i, TF_HEALTH_UNDEFINED );
		m_iPlayerClass.Set( i, TF_CLASS_UNDEFINED );
		m_iColors.Set(i, Vector(0.0, 0.0, 0.0));
		m_iKillstreak.Set(i, 0);
		m_bArenaSpectator.Set( i, false );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	CTFPlayer *pPlayer = (CTFPlayer*)UTIL_PlayerByIndex( iIndex );

	if ( pPlayer && pPlayer->IsConnected() )
	{	
		return m_iTotalScore[iIndex];
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CTFPlayerResource::GetPlayerColor( int iIndex )
{
	return Color( m_iColors[iIndex].x * 255.0, m_iColors[iIndex].y * 255.0, m_iColors[iIndex].z * 255.0, 255 );
}