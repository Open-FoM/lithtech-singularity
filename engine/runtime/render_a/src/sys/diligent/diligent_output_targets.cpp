#include "diligent_state.h"

void diligent_SetOutputTargets(Diligent::ITextureView* render_target, Diligent::ITextureView* depth_target)
{
	g_diligent_state.output_render_target_override = render_target;
	g_diligent_state.output_depth_target_override = depth_target;
}
