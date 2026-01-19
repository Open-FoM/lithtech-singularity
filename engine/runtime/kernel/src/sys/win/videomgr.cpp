
#include "bdefs.h"
#include "videomgr.h"

#include "ltjs_ffmpeg_video_mgr_impl.h"


extern int32 g_CV_VideoDebug;

void VideoMgr::UpdateVideos()
{
	uint32 nRunning = 0;
	for(MPOS pos=m_Videos; pos; ) 
	{
		VideoInst *pVideo = m_Videos.GetNext(pos);

		pVideo->Update();
			
		if(pVideo->GetVideoStatus() == LT_OK)
			++nRunning;
	}

	if( g_CV_VideoDebug ) 
	{
		dsi_ConsolePrint("%d videos, %d active", m_Videos.GetSize(), nRunning);
	}
}


void VideoMgr::OnRenderInit()
{
	MPOS pos;

	for( pos = m_Videos; pos; )
		m_Videos.GetNext(pos)->OnRenderInit();
}


void VideoMgr::OnRenderTerm()
{
	MPOS pos;

	for( pos = m_Videos; pos; )
		m_Videos.GetNext(pos)->OnRenderTerm();
}




// -------------------------------------------------------------------------------- //
// The starting point...
//
//	Create the appropriate videomgr 
//
// 
//
// -------------------------------------------------------------------------------- //
VideoMgr* CreateVideoMgr( const char *pszName )
{
	const auto want_ffmpeg = (::stricmp(pszName, "FFMPEG") == 0);

	if (want_ffmpeg)
	{
#ifdef LTJS_USE_FFMPEG_VIDEO_MGR
		auto video_mgr_uptr = std::make_unique<ltjs::FfmpegVideoMgr>();

		if (video_mgr_uptr)
		{
			if (video_mgr_uptr->initialize() == LT_OK)
			{
				return video_mgr_uptr.release();
			}
		}
#endif // LTJS_USE_FFMPEG_VIDEO_MGR
	}
  
	return LTNULL;
}
