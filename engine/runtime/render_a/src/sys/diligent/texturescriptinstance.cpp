#include "bdefs.h"
#include "ltbasedefs.h"
#include "texturescriptinstance.h"
#include "renderstruct.h"
#include "viewparams.h"
#include "diligent_state.h"

#include <cstdarg>
#include <cstring>

namespace
{
float diligent_get_frame_time()
{
	if (g_diligent_state.scene_desc)
	{
		return g_diligent_state.scene_desc->m_FrameTime;
	}

	return 0.0f;
}

uint32 diligent_get_frame_code()
{
	if (g_diligent_state.render_struct && g_diligent_state.render_struct->GetObjectFrameCode)
	{
		return g_diligent_state.render_struct->GetObjectFrameCode();
	}

	return g_diligent_state.object_frame_code;
}
} // namespace

CTextureScriptInstance::CTextureScriptInstance() :
	m_bFirstUpdate(true),
	m_fOldTime(0.0f),
	m_nOldFrameCode(0),
	m_nRefCount(0)
{
}

CTextureScriptInstance::~CTextureScriptInstance()
{
}

//sets up a texture transform as a unique evaluator
bool CTextureScriptInstance::SetupStage(uint32 nStage, uint32 nVarID, ETextureScriptChannel eChannel, ITextureScriptEvaluator* pEvaluator)
{
	//check range
	if(nStage >= NUM_STAGES)
		return false;

	//don't let them set up a null channel
	if(eChannel == TSChannel_Null)
		return true;

	//setup the stage, but make sure it isn't currently setup
	ASSERT(m_Stages[nStage].m_bValid == false);
	ASSERT(pEvaluator);

	//set up the stage
	m_Stages[nStage].m_nID			= nVarID;
	m_Stages[nStage].m_eChannel		= eChannel;
	m_Stages[nStage].m_pEvaluator	= pEvaluator;
	m_Stages[nStage].m_bValid		= true;
	m_Stages[nStage].m_pOverride	= NULL;

	return true;
}

//sets up a texture transform as a reference to another transform (it will piggy back the
//transform from there)
bool CTextureScriptInstance::SetupStageAsReference(uint32 nStage, ETextureScriptChannel eChannel, uint32 nReferTo)
{
	//check range
	if((nStage >= NUM_STAGES) || (nReferTo >= NUM_STAGES))
		return false;

	//don't let them set up a null channel
	if(eChannel == TSChannel_Null)
		return true;

	//setup the stage, but make sure it isn't currently setup
	ASSERT(m_Stages[nStage].m_bValid == false);

	//set up the stage
	m_Stages[nStage].m_eChannel		= eChannel;
	m_Stages[nStage].m_bValid		= true;
	m_Stages[nStage].m_pOverride	= &m_Stages[nReferTo];

	return true;
}

//called to update the matrix transform. If force is not set, it will
//attempt to determine if the matrix is dirty, and not set it if it isn't
bool CTextureScriptInstance::Evaluate(bool bForce)
{
	//determine the current time
	float fTime = m_fOldTime + diligent_get_frame_time();

	//determine what items are dirty
	const uint32 frame_code = diligent_get_frame_code();
	bool bFrameDirty = (frame_code != m_nOldFrameCode);

	for(uint32 nCurrStage = 0; nCurrStage < NUM_STAGES; nCurrStage++)
	{
		//get the stage
		CTextureScriptInstanceStage* pStage = &m_Stages[nCurrStage];

		//ignore if invalid or is overridden
		if(!pStage->m_bValid || pStage->m_pOverride)
			continue;

		//we need to see if we need to evaluate the matrix
		uint32 nFlags = pStage->m_pEvaluator->GetFlags();

		//evaluate if the appropriate data is dirty, or if we are forcing it
		bool bEvaluate = bForce || m_bFirstUpdate;

		//see if the framecode is dirty
		if(	((nFlags & ITextureScriptEvaluator::FLAG_DIRTYONFRAME) ||
			 (nFlags & ITextureScriptEvaluator::FLAG_WORLDSPACE)) && bFrameDirty)
		{
			bEvaluate = true;
		}

		//get the variables
		float* pVars = CTextureScriptVarMgr::GetSingleton().GetVars(pStage->m_nID);

		if(pVars == NULL)
			continue;

		//see if the variables are dirty
		if( (nFlags & ITextureScriptEvaluator::FLAG_DIRTYONVAR) &&
			(memcmp(pVars, pStage->m_fOldVars, sizeof(float) * CTextureScriptVarMgr::NUM_VARS) != 0))
		{
			bEvaluate = true;
		}

		//bail if we don't need to evaluate it
		if(!bEvaluate)
			continue;

		//ok, we need to actually evaluate it, so fill out the parameters for the evaluation
		CTextureScriptEvaluateVars Vars;
		Vars.m_fTime		= fTime;
		Vars.m_fElapsed		= fTime - m_fOldTime;
		Vars.m_fUserVars	= pVars;

		//now let the evaluator evaluate
		pStage->m_pEvaluator->Evaluate(Vars, pStage->m_mTransform);

		//transform the matrix to be in the appropriate space
		if(nFlags & ITextureScriptEvaluator::FLAG_WORLDSPACE)
		{
			if(pStage->m_pEvaluator->GetInputType() == ITextureScriptEvaluator::INPUT_POS)
			{
				//do a full matrix multiply
				pStage->m_mTransform = pStage->m_mTransform * g_diligent_state.view_params.m_mInvView;
			}
			else
			{
				//since we are not doing the position we only want to do an orientation
				//transform
				LTMatrix mTrans;
				mTrans.Init(	g_diligent_state.view_params.m_Right.x, g_diligent_state.view_params.m_Up.x, g_diligent_state.view_params.m_Forward.x, 0.0f,
								g_diligent_state.view_params.m_Right.y, g_diligent_state.view_params.m_Up.y, g_diligent_state.view_params.m_Forward.y, 0.0f,
								g_diligent_state.view_params.m_Right.z, g_diligent_state.view_params.m_Up.z, g_diligent_state.view_params.m_Forward.z, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f);
				pStage->m_mTransform = pStage->m_mTransform * mTrans;
			}
		}

		//update our old stuff for proper dirty detection
		memcpy(pStage->m_fOldVars, pVars, sizeof(float) * CTextureScriptVarMgr::NUM_VARS);
	}

	//update our other info for dirty detection
	if( m_bFirstUpdate || (m_nOldFrameCode != frame_code) )
	{
	m_fOldTime			= fTime;
	m_nOldFrameCode		= frame_code;
	}
	m_bFirstUpdate		= false;

	//success
	return true;
}

//Installs the matrix into the specified channel with the appropriate flags.
//If evaluate is true it will (unforcefully) evaluate the matrix
bool CTextureScriptInstance::Install(uint32 nNumChannels, ...)
{
	va_list vl;
	va_start(vl, nNumChannels);
	for (uint32 nCurrChannel = 0; nCurrChannel < nNumChannels; ++nCurrChannel)
	{
		static_cast<void>(va_arg(vl, ETextureScriptChannel));
	}
	va_end(vl);

	return Evaluate();
}

//disables all transforms that were set
bool CTextureScriptInstance::Uninstall(uint32 nNumChannels, ...)
{
	return true;
}

bool CTextureScriptInstance::GetStageData(ETextureScriptChannel eChannel, TextureScriptStageData& out, bool bForce)
{
	out = TextureScriptStageData{};

	if (eChannel == TSChannel_Null)
	{
		return false;
	}

	if (!Evaluate(bForce))
	{
		return false;
	}

	for (uint32 nCurrStage = 0; nCurrStage < NUM_STAGES; ++nCurrStage)
	{
		CTextureScriptInstanceStage* pStage = &m_Stages[nCurrStage];
		if (!pStage->m_bValid || pStage->m_eChannel != eChannel)
		{
			continue;
		}

		CTextureScriptInstanceStage* pOverride = pStage->m_pOverride ? pStage->m_pOverride : pStage;
		if (!pOverride->m_bValid || !pOverride->m_pEvaluator)
		{
			return false;
		}

		out.transform = pOverride->m_mTransform;
		out.flags = pOverride->m_pEvaluator->GetFlags();
		out.input_type = pOverride->m_pEvaluator->GetInputType();
		out.valid = true;
		return true;
	}

	return false;
}
