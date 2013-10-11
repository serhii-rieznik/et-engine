#pragma once

namespace et
{
	namespace gui
	{
		enum State
		{
			State_Default,
			State_Hovered,
			State_Pressed,
			State_Selected,
			State_SelectedHovered,
			State_SelectedPressed,
			State_max
		};

		enum AnimationFlags
		{
			AnimationFlag_None = 0x0,
			AnimationFlag_Fade = 0x01,
			AnimationFlag_FromLeft = 0x02,
			AnimationFlag_FromRight = 0x04,
			AnimationFlag_FromTop = 0x08,
			AnimationFlag_FromBottom = 0x10
		};

		enum Flags
		{
			Flag_RequiresKeyboard = 0x0001,
			Flag_Dragable = 0x0002,
			Flag_TransparentForPointer = 0x0004,
			Flag_RenderTopmost = 0x0008,
			Flag_HandlesChildEvents = 0x0010,
			Flag_ClipToBounds = 0x0020,
			Flag_HandlesChildLayout = 0x0040
		};

		enum AnimatedPropery
		{
			AnimatedProperty_None,
			AnimatedProperty_Angle,
			AnimatedProperty_Scale,
			AnimatedProperty_Color,
			AnimatedProperty_Frame,
			AnimatedProperty_max
		};

		enum RenderLayer 
		{
			RenderLayer_Layer0,
			RenderLayer_Layer1,
			RenderLayer_max
		};

		enum Alignment
		{
			Alignment_Near,
			Alignment_Center,
			Alignment_Far,
			Alignment_max,
		};

		enum LayoutMode
		{
			LayoutMode_Absolute,
			LayoutMode_RelativeToParent,
			LayoutMode_RelativeToContext,
			LayoutMode_WrapContent
		};

		enum LayoutMask
		{
			LayoutMask_None = 0x00,
			LayoutMask_Position = 0x01,
			LayoutMask_Size = 0x02,
			LayoutMask_Pivot = 0x04,

			LayoutMask_PositionPivot = LayoutMask_Position | LayoutMask_Pivot,
			LayoutMask_Frame = LayoutMask_Position | LayoutMask_Size,
			LayoutMask_All = LayoutMask_Frame | LayoutMask_Pivot
		};
	}
}