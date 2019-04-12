
#include "cbase.h"

#include "nav_mesh.h"
#include "nav_colors.h"
#include "fmtstr.h"
#include "props_shared.h"

#include "functorutils.h"
#include "team.h"
#include "nav_entities.h"

#include "tf_nav_area.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_nav_in_combat_duration( "tf_nav_in_combat_duration", "30", FCVAR_CHEAT, "How long after gunfire occurs is this area still considered to be 'in combat'" );
ConVar tf_nav_combat_build_rate( "tf_nav_combat_build_rate", "0.05", FCVAR_CHEAT, "Gunfire/second increase (combat caps at 1.0)" );
ConVar tf_nav_combat_decay_rate( "tf_nav_combat_decay_rate", "0.022", FCVAR_CHEAT, "Decay/second toward zero" );
ConVar tf_nav_show_incursion_distances( "tf_nav_show_incursion_distance", "0", FCVAR_CHEAT, "Display travel distances from current spawn room (1=red, 2=blue)", true, 0.0f, true, 1.0f );
ConVar tf_nav_show_bomb_target_distance( "tf_nav_show_bomb_target_distance", "0", FCVAR_CHEAT, "Display travel distances to bomb target (MvM mode)", true, 0.0f, true, 1.0f );
ConVar tf_nav_show_turf_ownership( "tf_nav_show_turf_ownership", "0", FCVAR_CHEAT, "Color nav area by smallest incursion distance", true, 0.0f, true, 1.0f );
ConVar tf_show_incursion_range( "tf_show_incursion_range", "0", FCVAR_CHEAT, "1 = red, 2 = blue" );
ConVar tf_show_incursion_range_min( "tf_show_incursion_range_min", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );
ConVar tf_show_incursion_range_max( "tf_show_incursion_range_max", "0", FCVAR_CHEAT, "Highlight areas with incursion distances between min and max cvar values" );

int CTFNavArea::m_masterTFMark = 1;

CTFNavArea::CTFNavArea()
{
}

CTFNavArea::~CTFNavArea()
{
	for ( int i = 0; i < 4; i++ )
		m_InvasionAreas[i].RemoveAll();
}

void CTFNavArea::OnServerActivate()
{
	CNavArea::OnServerActivate();

	for ( int i = 0; i < 4; i++ )
		m_InvasionAreas[i].RemoveAll();

	m_fCombatIntensity = 0;
}

void CTFNavArea::OnRoundRestart()
{
	CNavArea::OnRoundRestart();

	m_fCombatIntensity = 0;
}

void CTFNavArea::Save( CUtlBuffer &fileBuffer, unsigned int version ) const
{
	CNavArea::Save( fileBuffer, version );
	fileBuffer.PutUnsignedInt( m_nAttributes );
}

NavErrorType CTFNavArea::Load( CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion )
{
	if ( subVersion > TheNavMesh->GetSubVersionNumber() )
	{
		Warning( "Unknown NavArea sub-version number\n" );
		return NAV_INVALID_FILE;
	}
	else
	{
		CNavArea::Load( fileBuffer, version, subVersion );
		if ( subVersion <= 1 )
		{
			m_nAttributes = (TFNavAttributeType)0;
			return NAV_OK;
		}
		else
		{
			fileBuffer.Get( &m_nAttributes, sizeof( unsigned int ) );
			if ( !fileBuffer.IsValid() )
			{
				Warning( "Can't read TF-specific attributes\n" );
				return NAV_INVALID_FILE;
			}
		}
	}

	return NAV_OK;
}

void CTFNavArea::UpdateBlocked( bool force, int teamID )
{

}

bool CTFNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
	if ( !( m_nAttributes & UNBLOCKABLE ) )
	{
		if ( !( m_nAttributes & BLOCKED ) )
		{
			if ( teamID != TF_TEAM_RED )
			{
				if ( teamID == TF_TEAM_BLUE && ( m_nAttributes & RED_ONE_WAY_DOOR ) )
					return true;

				return CNavArea::IsBlocked( teamID, ignoreNavBlockers );
			}

			if ( !( m_nAttributes & BLUE_ONE_WAY_DOOR ) )
				return CNavArea::IsBlocked( teamID, ignoreNavBlockers );
		}

		return true;
	}

	return false;
}

void CTFNavArea::Draw() const
{
	CNavArea::Draw();

	if ( tf_nav_show_incursion_distances.GetBool() )
	{
		NDebugOverlay::Text( GetCenter(),
							 UTIL_VarArgs( "R:%3.1f   B:%3.1f", m_aIncursionDistances[TF_TEAM_RED], m_aIncursionDistances[TF_TEAM_BLUE] ),
							 true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	if ( tf_nav_show_bomb_target_distance.GetBool() )
	{
		NDebugOverlay::Text( GetCenter(),
							 UTIL_VarArgs( "%3.1f", m_flBombTargetDistance ),
							 true, NDEBUG_PERSIST_TILL_NEXT_SERVER );
	}

	if ( tf_nav_show_turf_ownership.GetBool() )
	{
		float flRadius = 500.0f; //tf_nav_show_turf_ownership_range.GetFloat(); // this is some development-only cvar that I can't find
		bool bRedOwnsMe = IsAwayFromInvasionAreas( TF_TEAM_RED, flRadius );
		bool bBluOwnsMe = IsAwayFromInvasionAreas( TF_TEAM_BLUE, flRadius );

		if ( bBluOwnsMe )
		{
			if ( bRedOwnsMe )
			{
				DrawFilled( 255, 0, 255, 255 );
			}
			else
			{
				DrawFilled( 0, 0, 255, 255 );
			}
		}
		else
		{
			DrawFilled( 255, 0, 0, 255 );
		}
	}

	if ( tf_show_incursion_range.GetInt() )
	{
		float flIncursion = -1.0f;
		if ( tf_show_incursion_range.GetInt() <= 3 )
		{
			flIncursion = GetIncursionDistance( tf_show_incursion_range.GetInt() );
			if ( flIncursion < tf_show_incursion_range_min.GetFloat() )
				return;
		}
		else
		{
			if ( tf_show_incursion_range_min.GetFloat() > -1.0f )
				return;
		}

		if ( flIncursion <= tf_show_incursion_range_max.GetFloat() )
			DrawFilled( 0, 255, 0, 255 );
	}

	// moved here from CTFNavMesh::UpdateDebugDisplay
	extern ConVar tf_show_mesh_decoration;
	if ( tf_show_mesh_decoration.GetBool() )
	{
		if ( HasTFAttributes( BLUE_SPAWN_ROOM ) )
		{
			if ( HasTFAttributes( SPAWN_ROOM_EXIT ) )
			{
				DrawFilled( 100, 100, 255, 255 );
				return;
			}

			DrawFilled( 0, 0, 255, 255 );
			return;
		}

		if ( HasTFAttributes( RED_SPAWN_ROOM ) )
		{
			if ( HasTFAttributes( SPAWN_ROOM_EXIT ) )
			{
				DrawFilled( 255, 100, 100, 255 );
				return;
			}

			DrawFilled( 255, 0, 0, 255 );
			return;
		}

		if ( HasTFAttributes( HEALTH ) || HasTFAttributes( AMMO ) )
		{
			if ( !HasTFAttributes( AMMO ) )
			{
				DrawFilled( 80, 140, 80, 255 );
				return;
			}

			if ( !HasTFAttributes( HEALTH ) )
			{
				DrawFilled( 140, 80, 80, 255 );
				return;
			}

			DrawFilled( 140, 140, 140, 255 );
			return;
		}

		if ( HasTFAttributes( CONTROL_POINT ) )
		{
			DrawFilled( 0, 255, 0, 255 );
			return;
		}
	}

	extern ConVar tf_show_blocked_areas;
	if ( tf_show_blocked_areas.GetBool() )
	{
		if ( HasTFAttributes( BLOCKED ) )
			DrawFilled( 255, 0, 0, 255 );

		if ( IsBlocked( TF_TEAM_RED ) && IsBlocked( TF_TEAM_BLUE ) )
			DrawFilled( 100, 0, 100, 255 );
		else if ( IsBlocked( TF_TEAM_RED ) )
			DrawFilled( 100, 0, 0, 255 );
		else if ( IsBlocked( TF_TEAM_BLUE ) )
			DrawFilled( 0, 0, 100, 255 );
	}
}

void CTFNavArea::CustomAnalysis( bool isIncremental )
{

}

/*bool CTFNavArea::IsPotentiallyVisibleToTeam( int team ) const
{
	if (team > -1 && team < 4)
		return m_PVNPCs[team].Count() > 0;

	return false;
}*/

void CTFNavArea::CollectNextIncursionAreas( int teamNum, CUtlVector<CTFNavArea*>* areas )
{
	areas->RemoveAll();
	// TODO
}

void CTFNavArea::CollectPriorIncursionAreas( int teamNum, CUtlVector<CTFNavArea*>* areas )
{
	areas->RemoveAll();
	// TODO
}

CTFNavArea *CTFNavArea::GetNextIncursionArea( int teamNum ) const
{
	CTFNavArea *result = NULL;

	float incursionDist = GetIncursionDistance( teamNum );

	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < m_InvasionAreas[i].Count(); j++ )
		{
			float otherIncursionDist = m_InvasionAreas[i][j]->GetIncursionDistance( teamNum );

			if ( otherIncursionDist > incursionDist )
			{
				incursionDist = fmaxf( incursionDist, otherIncursionDist );
				result = m_InvasionAreas[i].Element( j );
			}
		}
	}

	return result;
}

void CTFNavArea::ComputeInvasionAreaVectors()
{
	// This is clearly a CNavMesh::ForAllAreasInRadius call with each area doing a ForAllCompletelyVisibleAreas
	// but what exactly is going on is hard to decipher
	// TODO
}

bool CTFNavArea::IsAwayFromInvasionAreas( int teamNum, float radius ) const
{
	Assert( teamNum < 4 );
	if ( teamNum < 4 )
	{
		const CUtlVector<CTFNavArea *> *invasionAreas = &m_InvasionAreas[teamNum];
		for ( int i=0; i<invasionAreas->Count(); ++i )
		{
			CTFNavArea *area = invasionAreas->Element( i );
			if ( Square( radius ) >( m_center - area->GetCenter() ).LengthSqr() )
				return false;
		}
	}

	return true;
}

/*void CTFNavArea::AddPotentiallyVisibleActor( CBaseCombatCharacter *actor )
{
	int team;
	if (!actor || ( team = actor->GetTeamNumber() ) > 3)
		return;

	// TODO
}*/

float CTFNavArea::GetCombatIntensity() const
{
	float intensity = 0.0f;
	if ( m_combatTimer.HasStarted() )
	{
		const float combatTime = m_combatTimer.GetElapsedTime();
		intensity = fmax( m_fCombatIntensity - ( combatTime * tf_nav_combat_decay_rate.GetFloat() ), 0.0f );
	}
	return intensity;
}

bool CTFNavArea::IsInCombat() const
{
	return GetCombatIntensity() > 0.01f;
}

void CTFNavArea::OnCombat()
{
	m_combatTimer.Reset();
	m_fCombatIntensity = fmin( m_fCombatIntensity + tf_nav_combat_build_rate.GetFloat(), 1.0f );
}

void CTFNavArea::ResetTFMarker()
{
	m_masterTFMark = 1;
}

void CTFNavArea::MakeNewTFMarker()
{
	++m_masterTFMark;
}

bool CTFNavArea::IsTFMarked() const
{
	return m_TFMarker == m_masterTFMark;
}

void CTFNavArea::TFMark()
{
	m_TFMarker = m_masterTFMark;
}

bool CTFNavArea::IsValidForWanderingPopulation() const
{
	return ( m_nAttributes & ( BLOCKED|RESCUE_CLOSET|BLUE_SPAWN_ROOM|RED_SPAWN_ROOM|NO_SPAWNING ) ) == 0;
}

float CTFNavArea::GetIncursionDistance( int teamnum ) const
{
	Assert( teamnum < 4 );
	if ( teamnum < 4 )
		return m_aIncursionDistances[teamnum];

	return -1.0f;
}