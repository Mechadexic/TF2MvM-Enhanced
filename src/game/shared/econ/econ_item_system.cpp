#include "cbase.h"
#include "econ_item_system.h"
#include "script_parser.h"
#include "activitylist.h"

const char *g_TeamVisualSections[TF_TEAM_COUNT] =
{
	"visuals",			// TEAM_UNASSIGNED
	"",					// TEAM_SPECTATOR
	"visuals_red",		// TEAM_RED
	"visuals_blu",		// TEAM_BLUE
	//"visuals_mvm_boss"	// ???
};

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",
	"value_is_inverted_percentage",
	"value_is_additive",
	"value_is_additive_percentage",
	"value_is_or",
	"value_is_date",
	"value_is_account_id",
	"value_is_particle_index",
	"value_is_killstreakeffect_index",
	"value_is_killstreak_idleeffect_index",
	"value_is_item_def",
	"value_is_from_lookup_table"
};

const char *g_EffectTypes[] =
{
	"unusual",
	"strange",
	"neutral",
	"positive",
	"negative"
};

static const char *const g_szAttributeTypes[] ={
	"",
	"uint64",
	"float",
	"string",
};

const char *g_szQualityStrings[] =
{
	"normal",
	"rarity1",
	"rarity2",
	"vintage",
	"rarity3",
	"rarity4",
	"unique",
	"community",
	"developer",
	"selfmade",
	"customized",
	"strange",
	"completed",
	"haunted",
	"collectors",
	"paintkitWeapon",
};

const char *g_szQualityColorStrings[] =
{
	"QualityColorNormal",
	"QualityColorrarity1",
	"QualityColorrarity2",
	"QualityColorVintage",
	"QualityColorrarity3",
	"QualityColorrarity4",
	"QualityColorUnique",
	"QualityColorCommunity",
	"QualityColorDeveloper",
	"QualityColorSelfMade",
	"QualityColorSelfMadeCustomized",
	"QualityColorStrange",
	"QualityColorCompleted",
	"QualityColorHaunted",
	"QualityColorCollectors",
	"QualityColorPaintkitWeapon",
};

const char *g_szQualityLocalizationStrings[] =
{
	"#Normal",
	"#rarity1",
	"#rarity2",
	"#vintage",
	"#rarity3",
	"#rarity4",
	"#unique",
	"#community",
	"#developer",
	"#selfmade",
	"#customized",
	"#strange",
	"#completed",
	"#haunted",
	"#collectors",
	"#paintkitWeapon",
};

//-----------------------------------------------------------------------------
// Purpose: for the UtlMap
//-----------------------------------------------------------------------------
static bool schemaLessFunc( const int &lhs, const int &rhs )
{
	return lhs < rhs;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static CEconItemSchema g_EconItemSchema;
CEconItemSchema *GetItemSchema()
{
	return &g_EconItemSchema;
}

class CEconSchemaParser : public CScriptParser
{
public:
	DECLARE_CLASS_GAMEROOT( CEconSchemaParser, CScriptParser );

#define GET_STRING(copyto, from, name)													\
		if (from->GetString(#name, NULL))												\
			V_strncpy(copyto->name, from->GetString(#name), sizeof(copyto->name))

#define GET_STRING_DEFAULT(copyto, from, name, defaultstring) \
		V_strncpy(copyto->name, from->GetString(#name, #defaultstring), sizeof(copyto->name))

#define GET_BOOL(copyto, from, name) \
		copyto->name = from->GetBool(#name, copyto->name)

#define GET_FLOAT(copyto, from, name) \
		copyto->name = from->GetFloat(#name, copyto->name)

#define GET_INT(copyto, from, name) \
		copyto->name = from->GetInt(#name, copyto->name)

#define GET_STRING_CONVERT(copyto, from, name) \
		if (from->GetString(#name, NULL))

#define FIND_ELEMENT(map, key, val)						\
		unsigned int index = map.Find(key);				\
		if (index != map.InvalidIndex())				\
			val = map.Element(index)				

#define FIND_ELEMENT_STRING(map, key, val)						\
		unsigned int index = map.Find(key);						\
		if (index != map.InvalidIndex())						\
			Q_snprintf(val, sizeof(val), map.Element(index))

#define IF_ELEMENT_FOUND(map, key)						\
		unsigned int index = map.Find(key);				\
		if (index != map.InvalidIndex())			

#define GET_VALUES_FAST_BOOL(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				dict.Element(index) = pKeyData->GetBool();	\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), pKeyData->GetBool());\
			}												\
		}


#define GET_VALUES_FAST_STRING(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())	\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				Q_snprintf((char*)dict.Element(index), sizeof(dict.Element(index)), pKeyData->GetString());		\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), strdup(pKeyData->GetString()));\
			}												\
		}	

	void Parse( KeyValues *pKeyValuesData, bool bWildcard, const char *szFileWithoutEXT )
	{
		KeyValues *pPrefabs = pKeyValuesData->FindKey( "prefabs" );
		if ( pPrefabs )
		{
			ParsePrefabs( pPrefabs );
		}

		KeyValues *pGameInfo = pKeyValuesData->FindKey( "game_info" );
		if ( pGameInfo )
		{
			ParseGameInfo( pGameInfo );
		}

		KeyValues *pQualities = pKeyValuesData->FindKey( "qualities" );
		if ( pQualities )
		{
			ParseQualities( pQualities );
		}

		KeyValues *pColors = pKeyValuesData->FindKey( "colors" );
		if ( pColors )
		{
			ParseColors( pColors );
		}

		KeyValues *pAttributes = pKeyValuesData->FindKey( "attributes" );
		if ( pAttributes )
		{
			ParseAttributes( pAttributes );
		}

		KeyValues *pItems = pKeyValuesData->FindKey( "items" );
		if ( pItems )
		{
			ParseItems( pItems );
		}
	};

	void ParseGameInfo( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			GetItemSchema()->m_GameInfo.Insert( pSubData->GetName(), pSubData->GetFloat() );
		}
	};

	void ParseQualities( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			EconQuality Quality;
			GET_INT( ( &Quality ), pSubData, value );
			GetItemSchema()->m_Qualities.Insert( pSubData->GetName(), Quality );
		}

	};

	void ParseColors( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			EconColor ColorDesc;
			GET_STRING( ( &ColorDesc ), pSubData, color_name );
			GetItemSchema()->m_Colors.Insert( pSubData->GetName(), ColorDesc );
		}
	};

	void ParsePrefabs( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			KeyValues *Values = pSubData->MakeCopy();
			GetItemSchema()->m_PrefabsValues.Insert( pSubData->GetName(), Values );
		}
	};

	void ParseItems( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			// Skip over default item, not sure why it's there.
			if ( !V_stricmp( pSubData->GetName(), "default" ) )
				continue;

			CEconItemDefinition *Item = new CEconItemDefinition;
			int index = atoi( pSubData->GetName() );

			if ( ParseItemRec( pSubData, Item ) )
			{
				Item->index = index;
				GetItemSchema()->m_Items.Insert( index, Item );
			}
			else
			{
				delete Item;
			}
		}
		for ( unsigned int i = 0; i < GetItemSchema()->m_PrefabsValues.Count(); i++ )
		{
			GetItemSchema()->m_PrefabsValues[i]->deleteThis();
		}
		GetItemSchema()->m_PrefabsValues.RemoveAll();
	};

	void ParseAttributes( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			EconAttributeDefinition *pAttribute = new EconAttributeDefinition;
			pAttribute->index = V_atoi( pSubData->GetName() );

			GET_STRING_DEFAULT( pAttribute, pSubData, name, ( unnamed ) );
			GET_STRING( pAttribute, pSubData, attribute_class );
			GET_STRING( pAttribute, pSubData, description_string );
			pAttribute->string_attribute = ( V_stricmp( pSubData->GetString( "attribute_type" ), "string" ) == 0 );

			const char *szFormat = pSubData->GetString( "description_format" );
			pAttribute->description_format = UTIL_StringFieldToInt( szFormat, g_AttributeDescriptionFormats, ARRAYSIZE( g_AttributeDescriptionFormats ) );

			const char *szEffect = pSubData->GetString( "effect_type" );
			pAttribute->effect_type = UTIL_StringFieldToInt( szEffect, g_EffectTypes, ARRAYSIZE( g_EffectTypes ) );

			const char *szType = pSubData->GetString( "attribute_type" );
			pAttribute->attribute_type = GetItemSchema()->GetAttributeType( szType );

			GET_BOOL( pAttribute, pSubData, hidden );
			GET_BOOL( pAttribute, pSubData, stored_as_integer );

			GetItemSchema()->m_Attributes.Insert( pAttribute->index, pAttribute );
		}
	};

	bool ParseVisuals( KeyValues *pData, CEconItemDefinition* pItem, int iIndex )
	{
		EconPerTeamVisuals *pVisuals = &pItem->visual[iIndex];

		for ( KeyValues *pVisualData = pData->GetFirstSubKey(); pVisualData != NULL; pVisualData = pVisualData->GetNextKey() )
		{
			if ( !V_stricmp( pVisualData->GetName(), "player_bodygroups" ) )
			{
				GET_VALUES_FAST_BOOL( pVisuals->player_bodygroups, pVisualData );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "attached_models" ) )
			{
				for (KeyValues *pAttachment = pVisualData->GetFirstSubKey(); pAttachment != NULL; pAttachment = pAttachment->GetNextKey())
				{
					AttachedModel_t attached_model;
					attached_model.model_display_flags = pAttachment->GetInt( "model_display_flags", AM_VIEWMODEL|AM_WORLDMODEL );
					V_strncpy( attached_model.model, pAttachment->GetString( "model" ), sizeof( attached_model.model ) );

					pVisuals->attached_models.AddToTail( attached_model );
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "custom_particlesystem" ) )
			{
				V_strncpy( pVisuals->custom_particlesystem, pVisualData->GetString( "system" ), sizeof( pVisuals->custom_particlesystem ) );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "muzzle_flash" ) )
			{
				// Fetching this similar to weapon script file parsing.
				V_strncpy( pVisuals->muzzle_flash, pVisualData->GetString( "system" ), sizeof( pVisuals->muzzle_flash ) );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "tracer_effect" ) )
			{
				// Fetching this similar to weapon script file parsing.
				V_strncpy( pVisuals->tracer_effect, pVisualData->GetString( "system" ), sizeof( pVisuals->tracer_effect ) );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "animation_replacement" ) )
			{
				for ( KeyValues *pKeyData = pVisualData->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey() )
				{
					int key = ActivityList_IndexForName( pKeyData->GetName() );
					int value = ActivityList_IndexForName( pKeyData->GetString() );

					if ( key != kActivityLookup_Missing && value != kActivityLookup_Missing )
					{
						pVisuals->animation_replacement.Insert( key, value );
					}
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "playback_activity" ) )
			{
				GET_VALUES_FAST_STRING( pVisuals->playback_activity, pVisualData );
			}
			else if ( !V_strnicmp( pVisualData->GetName(), "sound_", 6 ) )
			{
				// Fetching this similar to weapon script file parsing.
				const char *pszSoundName = pVisualData->GetString();
				
				// Advancing pointer past sound_ prefix... why couldn't they just make a subsection for sounds?
				int iSound = GetWeaponSoundFromString( pVisualData->GetName() + 6 );

				if ( iSound != -1 )
				{
					V_strncpy( pVisuals->aWeaponSounds[iSound], pszSoundName, MAX_WEAPON_STRING );
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "styles" ) )
			{
				for (KeyValues *pStyleData = pVisualData->GetFirstSubKey(); pStyleData != NULL; pStyleData = pStyleData->GetNextKey())
				{
					EconItemStyle *style;
					IF_ELEMENT_FOUND( pVisuals->styles, pStyleData->GetName() )
					{
						style = pVisuals->styles.Element( index );
					}
					else
					{
						style = new EconItemStyle;
						pVisuals->styles.Insert( pStyleData->GetName(), style );
					}

					GET_STRING( style, pStyleData, name );
					GET_STRING( style, pStyleData, model_player );
					GET_STRING( style, pStyleData, image_inventory );
					GET_BOOL( style, pStyleData, selectable );
					GET_INT( style, pStyleData, skin_red );
					GET_INT( style, pStyleData, skin_blu );

					for ( KeyValues *pStyleModelData = pStyleData->GetFirstSubKey(); pStyleModelData != NULL; pStyleModelData = pStyleModelData->GetNextKey() )
					{
						if ( !V_stricmp( pStyleModelData->GetName(), "model_player_per_class" ) )
						{
							GET_VALUES_FAST_STRING( style->model_player_per_class, pStyleModelData );
						}
					}
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "skin" ) )
			{
				pVisuals->skin = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "use_per_class_bodygroups" ) )
			{
				pVisuals->use_per_class_bodygroups = pVisualData->GetInt();
			}
			else
			{
				GET_VALUES_FAST_STRING( pVisuals->misc_info, pVisualData );
			}
		}

		return true;
	}

	bool ParseItemRec( KeyValues *pData, CEconItemDefinition* pItem )
	{
		char prefab[64];
		Q_snprintf( prefab, sizeof( prefab ), pData->GetString( "prefab" ) );	//check if there's prefab for prefab.. PREFABSEPTION

		if ( prefab[0] != '\0' )
		{
			CUtlStringList strings;
			V_SplitString( prefab, " ", strings );

			for( const char *pch : strings )
			{
				KeyValues *pPrefabValues = NULL;
				FIND_ELEMENT( GetItemSchema()->m_PrefabsValues, pch, pPrefabValues );
				if ( pPrefabValues )
				{
					ParseItemRec( pPrefabValues, pItem );
				}
			}
		}

		GET_STRING( pItem, pData, name );
		GET_BOOL( pItem, pData, show_in_armory );

		GET_STRING( pItem, pData, item_class );
		GET_STRING( pItem, pData, item_name );
		GET_STRING( pItem, pData, item_description );
		GET_STRING( pItem, pData, item_type_name );
		
		const char *pszQuality = pData->GetString( "item_quality" );
		if ( pszQuality[0] )
		{
			int iQuality = UTIL_StringFieldToInt( pszQuality, g_szQualityStrings, ARRAYSIZE( g_szQualityStrings ) );
			if ( iQuality != -1 )
			{
				pItem->item_quality = iQuality;
			}
		}
		
		// All items are vintage quality
		//pItem->item_quality = QUALITY_VINTAGE;

		GET_STRING( pItem, pData, item_logname );
		GET_STRING( pItem, pData, item_iconname );

		const char *pszLoadoutSlot = pData->GetString( "item_slot" );

		if ( pszLoadoutSlot[0] )
		{
			pItem->item_slot = UTIL_StringFieldToInt( pszLoadoutSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
		}

		const char *pszAnimSlot = pData->GetString( "anim_slot" );
		if ( pszAnimSlot[0] )
		{
			if ( V_strcmp( pszAnimSlot, "FORCE_NOT_USED" ) != 0 )
			{
				pItem->anim_slot = UTIL_StringFieldToInt( pszAnimSlot, g_AnimSlots, TF_WPN_TYPE_COUNT );
			}
			else
			{
				pItem->anim_slot = -2;
			}
		}

		GET_BOOL( pItem, pData, baseitem );
		GET_INT( pItem, pData, min_ilevel );
		GET_INT( pItem, pData, max_ilevel );

		GET_STRING( pItem, pData, image_inventory );
		GET_INT( pItem, pData, image_inventory_size_w );
		GET_INT( pItem, pData, image_inventory_size_h );

		GET_STRING( pItem, pData, model_player );
		GET_STRING( pItem, pData, model_vision_filtered );
		GET_STRING( pItem, pData, model_world );
		GET_STRING( pItem, pData, extra_wearable );

		GET_INT( pItem, pData, attach_to_hands );
		GET_INT( pItem, pData, attach_to_hands_vm_only );
		GET_BOOL( pItem, pData, act_as_wearable );
		GET_INT( pItem, pData, hide_bodygroups_deployed_only );
		
		GET_BOOL( pItem, pData, is_reskin );
		GET_BOOL( pItem, pData, specialitem );
		GET_BOOL( pItem, pData, demoknight );
		GET_STRING( pItem, pData, holiday_restriction );
		GET_BOOL( pItem, pData, itemfalloff );
		GET_INT( pItem, pData, year );
		GET_BOOL( pItem, pData, is_custom_content );
		GET_STRING( pItem, pData, custom_projectile_model );

		for ( KeyValues *pSubData = pData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			if ( !V_stricmp( pSubData->GetName(), "capabilities" ) )
			{
				GET_VALUES_FAST_BOOL( pItem->capabilities, pSubData );
			}
			else if ( !V_stricmp( pSubData->GetName(), "tags" ) )
			{
				GET_VALUES_FAST_BOOL( pItem->tags, pSubData );
			}
			else if ( !V_stricmp( pSubData->GetName(), "model_player_per_class" ) )
			{
				for ( KeyValues *pClassData = pSubData->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
				{
					const char *pszClass = pClassData->GetName();
					int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );

					if ( iClass != -1 )
					{
						V_strncpy( pItem->model_player_per_class[iClass], pClassData->GetString(), 128 );
					}
				}
			}
			else if ( !V_stricmp( pSubData->GetName(), "used_by_classes" ) )
			{
				for ( KeyValues *pClassData = pSubData->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
				{
					const char *pszClass = pClassData->GetName();
					int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );

					if ( iClass != -1 )
					{
						pItem->used_by_classes |= ( 1 << iClass );
						const char *pszSlotname = pClassData->GetString();

						if ( pszSlotname[0] != '1' )
						{
							int iSlot = UTIL_StringFieldToInt( pszSlotname, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
							
							if ( iSlot != -1 )
								pItem->item_slot_per_class[iClass] = iSlot;
						}
					}
				}
			}
			else if ( !V_stricmp( pSubData->GetName(), "attributes" ) )
			{
				for ( KeyValues *pAttribData = pSubData->GetFirstSubKey(); pAttribData != NULL; pAttribData = pAttribData->GetNextKey() )
				{
					static_attrib_t attribute;
					if ( !attribute.BInitFromKV_MultiLine( pAttribData ) )
						continue;

					pItem->attributes.AddToTail( attribute );
				}
			}
			else if ( !V_stricmp( pSubData->GetName(), "static_attrs" ) )
			{
				for ( KeyValues *pAttribData = pSubData->GetFirstSubKey(); pAttribData != NULL; pAttribData = pAttribData->GetNextKey() )
				{
					static_attrib_t attribute;
					if ( !attribute.BInitFromKV_SingleLine( pAttribData ) )
						continue;

					pItem->attributes.AddToTail( attribute );
				}
			}
			else if ( !V_stricmp( pSubData->GetName(), "visuals_mvm_boss" ) )
			{
				// Deliberately skipping this.
			}
			else if ( !V_strnicmp( pSubData->GetName(), "visuals", 7 ) )
			{
				// Figure out what team is this meant for.
				int iVisuals = UTIL_StringFieldToInt( pSubData->GetName(), g_TeamVisualSections, TF_TEAM_COUNT );

				if ( iVisuals != -1 )
				{
					if ( iVisuals == TEAM_UNASSIGNED )
					{
						// Hacky: for standard visuals block, assign it to all teams at once.
						for ( int i = 0; i < TF_TEAM_COUNT; i++ )
						{
							if ( i == TEAM_SPECTATOR )
								continue;

							ParseVisuals( pSubData, pItem, i );
						}
					}
					else
					{
						ParseVisuals( pSubData, pItem, iVisuals );
					}
				}
			}
		}

		return true;
	};

private:
};
CEconSchemaParser g_EconSchemaParser;

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema()
{
	m_Items.SetLessFunc( schemaLessFunc );
	m_Attributes.SetLessFunc( schemaLessFunc );

	m_bInited = false;
}

CEconItemSchema::~CEconItemSchema()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CEconItemSchema::Init( void )
{
	if ( !m_bInited )
	{
		// Must register activities early so we can parse animation replacements.
		ActivityList_Free();
		ActivityList_RegisterSharedActivities();

		float flStartTime = engine->Time();
		g_EconSchemaParser.InitParser( "scripts/items/items_game.txt", true, false );
		float flEndTime = engine->Time();
		Msg( "Processing item schema took %.02fms. Parsed %d items and %d attributes.\n", ( flEndTime - flStartTime ) * 1000.0f, m_Items.Count(), m_Attributes.Count() );

		m_bInited = true;
	}

	return true;
}

void CEconItemSchema::Precache( void )
{
	static CSchemaFieldHandle<AttributeDefinition_t> pAttribDef_CustomProjectile( "custom projectile model" );

	// Precache everything from schema.
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];

		// Precache projectiles.
		if ( pItem->custom_projectile_model[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->custom_projectile_model );
		
		// Precache models.
		if ( pItem->model_world[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_world );

		if ( pItem->model_player[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_player );

		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			const char *pszModel = pItem->model_player_per_class[iClass];
			if ( pszModel[0] != '\0' )
				CBaseEntity::PrecacheModel( pszModel );
		}

		if ( pItem->extra_wearable[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->extra_wearable );
		
		// Precache visuals.
		for ( int i = 0; i < TF_TEAM_COUNT; i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			PerTeamVisuals_t *pVisuals = &pItem->visual[i];

			// Precache sounds.
			for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
			{
				if ( pVisuals->aWeaponSounds[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->aWeaponSounds[i] );
			}

			// Precache attachments.
			for ( int i = 0; i < pVisuals->attached_models.Count(); i++ )
			{
				const char *pszModel = pVisuals->attached_models[i].model;
				if ( pszModel[0] != '\0' )
					CBaseEntity::PrecacheModel( pszModel );
			}

			// Precache particles
			// Custom Particles
			const char *pszParticle = pVisuals->custom_particlesystem;
			if ( pszParticle[0] != '\0' )
			{
				PrecacheParticleSystem( pszParticle );
			}
			// Muzzle Flash
			const char *pszMuzzleFlash = pVisuals->muzzle_flash;
			if ( pszMuzzleFlash[0] != '\0' )
			{
				PrecacheParticleSystem( pszMuzzleFlash );
			}
			// Tracer Effect
			const char *pszTracerEffect = pVisuals->tracer_effect;
			if ( pszTracerEffect[0] != '\0' )
			{
				PrecacheParticleSystem( pszTracerEffect );
			}

		}

		// Cache all attrbute names.
		for ( static_attrib_t attrib : pItem->attributes )
		{
			const EconAttributeDefinition *pAttribute = attrib.GetStaticData();

			// Special case for custom_projectile_model attribute.
			if ( pAttribute == pAttribDef_CustomProjectile )
			{
				CBaseEntity::PrecacheModel( STRING( attrib.value.sVal ) );
			}
		}
	}
}

CEconItemDefinition* CEconItemSchema::GetItemDefinition( int id )
{
	if ( id < 0 )
		return NULL;
	CEconItemDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Items, id, itemdef );
	return itemdef;
}

CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Items, i )
	{
		if ( m_Items[i]->index > -1 && !V_stricmp( m_Items[i]->name, name ) )
		{
			return m_Items[i];
		}
	}

	return NULL;
}

EconAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int id )
{
	if (id < 0)
		return NULL;
	EconAttributeDefinition *attribdef = NULL;
	FIND_ELEMENT( m_Attributes, id, attribdef );
	return attribdef;
}

EconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

EconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByClass( const char *classname )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->attribute_class, classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

int CEconItemSchema::GetAttributeIndex( const char *name )
{
	if ( !name )
		return -1;

	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes.Key( i );
		}
	}

	return -1;
}

int CEconItemSchema::GetAttributeType( const char *type ) const
{
	for ( int i=0; i < ARRAYSIZE( g_szAttributeTypes ); ++i )
	{
		if ( !V_stricmp( type, g_szAttributeTypes[ i ] ) )
			return i;
	}

	return -1;
}