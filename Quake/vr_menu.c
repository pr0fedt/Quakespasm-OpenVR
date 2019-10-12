#include "quakedef.h"
#include "vr.h"
#include "vr_menu.h"

extern cvar_t vr_enabled;
extern cvar_t vr_crosshair;
extern cvar_t vr_crosshair_depth;
extern cvar_t vr_crosshair_size;
extern cvar_t vr_crosshair_alpha;
extern cvar_t vr_aimmode;
extern cvar_t vr_deadzone;
extern cvar_t vr_world_scale;
extern cvar_t vr_snap_turn;
extern cvar_t vr_turn_speed;

static int	vr_options_cursor = 0;

#define VR_MAX_TURN_SPEED 10.0f

extern void M_DrawSlider (int x, int y, float range);

void VR_Menu_Init()
{
	// VR menu function pointers
	vr_menucmdfn = VR_Menu_f;
	vr_menudrawfn = VR_MenuDraw;
	vr_menukeyfn = VR_MenuKey;
}

static void VR_MenuPlaySound(const char *sound, float fvol)
{
	sfx_t *sfx = S_PrecacheSound( sound );

	if ( sfx ) {
		S_StartSound( cl.viewentity, 0, sfx, vec3_origin, fvol, 1 );
	}
}

static void VR_MenuPrintOptionValue(int cx, int cy, int option)
{
	char value_buffer[16] = {0};
	const char *value_string = NULL;

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif
	switch ( option ) {
		default: break;

		case VR_OPTION_ENABLED:
			M_DrawCheckbox( cx, cy, (int)vr_enabled.value );
			break;
		/*case VR_OPTION_PERFHUD:
			if (vr_perfhud.value == 1) value_string = "Latency Timing";
			else if (vr_perfhud.value == 2) value_string = "Render Timing";
			else if (vr_perfhud.value == 3) value_string = "Perf Headroom";
			else if (vr_perfhud.value == 4) value_string = "Version Info";
			else value_string = "off";
			break;*/
		case VR_OPTION_AIMMODE:
			switch ( (int)vr_aimmode.value ) {
				case VR_AIMMODE_HEAD_MYAW:
					value_string = "HEAD_MYAW";
					break;
				case VR_AIMMODE_HEAD_MYAW_MPITCH:
					value_string = "HEAD_MYAW_MPITCH";
					break;
				case VR_AIMMODE_MOUSE_MYAW:
					value_string = "MOUSE_MYAW";
					break;
				case VR_AIMMODE_MOUSE_MYAW_MPITCH:
					value_string = "MOUSE_MYAW_MPITCH";
					break;
				default:
				case VR_AIMMODE_BLENDED:
					value_string = "BLENDED";
					break;
				case VR_AIMMODE_BLENDED_NOPITCH:
					value_string = "BLENDED_NOPITCH";
					break;
				case VR_AIMMODE_CONTROLLER:
					value_string = "CONTROLLER";
					break;
			}
			break;
		case VR_OPTION_DEADZONE:
			if ( vr_deadzone.value > 0 ) {
				snprintf( value_buffer, sizeof(value_buffer), "%.0f degrees", vr_deadzone.value );
				value_string = value_buffer;
			} else {
				value_string = "off";
			}
			break;
		case VR_OPTION_CROSSHAIR:
			if ( (int)vr_crosshair.value == 2 ) {
				value_string = "line";
			} else if ( (int)vr_crosshair.value == 1 ) {
				value_string = "point";
			} else {
				value_string = "off";
			}
			break;
		case VR_OPTION_CROSSHAIR_DEPTH:
			if ( vr_crosshair_depth.value > 0 ) {
				snprintf( value_buffer, sizeof(value_buffer), "%.0f units", vr_crosshair_depth.value );
				value_string = value_buffer;
			} else {
				value_string = "off";
			}
			break;
		case VR_OPTION_CROSSHAIR_SIZE:
			if ( vr_crosshair_size.value > 0 ) {
				snprintf( value_buffer, sizeof(value_buffer), "%.0f pixels", vr_crosshair_size.value );
				value_string = value_buffer;
			} else {
				value_string = "off";
			}
			break;
		case VR_OPTION_CROSSHAIR_ALPHA:
			M_DrawSlider( cx, cy, vr_crosshair_alpha.value );
			break;
		case VR_OPTION_WORLD_SCALE:
			M_DrawSlider(cx, cy, vr_world_scale.value / 2.0f);
			break;
		case VR_OPTION_MOVEMENT_MODE:
			switch ((int)vr_movement_mode.value)
			{
			case VR_MOVEMENT_MODE_FOLLOW_HAND: value_string = "Follow hand"; break;
			case VR_MOVEMENT_MODE_RAW_INPUT: value_string = "Raw input"; break;
			}
			break;
		case VR_OPTION_SNAP_TURN:
			if (vr_snap_turn.value == 0)
			{
				value_string = "Smooth";
			}
			else
			{
				snprintf(value_buffer, sizeof(value_buffer), "%d Degrees", (int)vr_snap_turn.value);
				value_string = value_buffer;
			}
			break;
		case VR_OPTION_TURN_SPEED:
			M_DrawSlider(cx, cy, vr_turn_speed.value / VR_MAX_TURN_SPEED);
			break;
		case VR_OPTION_MSAA:
			if (vr_msaa.value == 0)
			{
				value_string = "Off";
			}
			else
			{
				snprintf(value_buffer, sizeof(value_buffer), "%d Samples", (int)vr_msaa.value);
				value_string = value_buffer;
			}
	}
#ifdef _MSC_VER
#undef snprintf
#endif
	if ( value_string ) {
		M_Print( cx, cy, value_string );
	}
}

static void VR_MenuKeyOption(int key, int option)
{
#define _sizeofarray(x) ( ( sizeof(x) / sizeof(x[0]) ) )
#define _maxarray(x) ( _sizeofarray(x) - 1 )

	qboolean isLeft = (key == K_LEFTARROW);
	int intValue = 0;
	float floatValue = 0.0f;
	int i = 0;

	int debug[] = { 0, 1, 2, 3, 4 };
	float ipdDiff = 0.2f;
	int position[] = { 0, 1, 2 };
	float multisample[] = { 1.0f, 1.25f, 1.50f, 1.75f, 2.0f };
	int aimmode[] = { 1, 2, 3, 4, 5, 6, 7 };
	int deadzoneDiff = 5;
	int crosshair[] = { 0, 1, 2 };
	int crosshairDepthDiff = 32;
	int crosshairSizeDiff = 1;
	float crosshairAlphaDiff = 0.05f;

	switch ( option ) {
		case VR_OPTION_ENABLED:
			Cvar_SetValue( "vr_enabled", ! (int)vr_enabled.value );
			if ( (int)vr_enabled.value ) {
				VR_MenuPlaySound( "items/r_item2.wav", 0.5 );
			}
			break;
		/*case VR_OPTION_PERFHUD:
			intValue = (int)vr_perfhud.value;
			intValue = CLAMP( debug[0], isLeft ? intValue - 1 : intValue + 1, debug[_maxarray( debug )] );
			Cvar_SetValue( "vr_perfhud", intValue );
			break;*/
		case VR_OPTION_AIMMODE:
			intValue = (int)vr_aimmode.value;
			intValue = CLAMP( aimmode[0], isLeft ? intValue - 1 : intValue + 1, _sizeofarray( aimmode ) );
			intValue -= 1;
			Cvar_SetValue( "vr_aimmode", aimmode[intValue] );
			break;
		case VR_OPTION_DEADZONE:
			intValue = (int)vr_deadzone.value;
			intValue = CLAMP( 0.0f, isLeft ? intValue - deadzoneDiff : intValue + deadzoneDiff, 180.0f );
			Cvar_SetValue( "vr_deadzone", intValue );
			break;
		case VR_OPTION_CROSSHAIR:
			intValue = (int)vr_crosshair.value;
			intValue = CLAMP( crosshair[0], isLeft ? intValue - 1 : intValue + 1, crosshair[_maxarray( crosshair) ] );
			Cvar_SetValue( "vr_crosshair", intValue );
			break;
		case VR_OPTION_CROSSHAIR_DEPTH:
			intValue = (int)vr_crosshair_depth.value;
			intValue = CLAMP( 0.0f, isLeft ? intValue - crosshairDepthDiff : intValue + crosshairDepthDiff, 4096 );
			Cvar_SetValue( "vr_crosshair_depth", intValue );
			break;
		case VR_OPTION_CROSSHAIR_SIZE:
			intValue = (int)vr_crosshair_size.value;
			intValue = CLAMP( 0.0f, isLeft ? intValue - crosshairSizeDiff : intValue + crosshairSizeDiff, 32 );
			Cvar_SetValue( "vr_crosshair_size", intValue );
			break;
		case VR_OPTION_CROSSHAIR_ALPHA:
			floatValue = vr_crosshair_alpha.value;
			floatValue = CLAMP( 0.0f, isLeft ? floatValue - crosshairAlphaDiff : floatValue + crosshairAlphaDiff, 1.0f );
			Cvar_SetValue( "vr_crosshair_alpha", floatValue );
			break;
		case VR_OPTION_WORLD_SCALE:
			floatValue = vr_world_scale.value;
			floatValue = CLAMP(0.0f, isLeft ? floatValue - crosshairAlphaDiff : floatValue + crosshairAlphaDiff, 2.0f);
			Cvar_SetValue("vr_world_scale", floatValue);
			break;
		case VR_OPTION_MOVEMENT_MODE:
			intValue = (int)vr_movement_mode.value;
			intValue = CLAMP(0.0f, isLeft ? intValue - 1 : intValue + 1, VR_MAX_MOVEMENT_MODE);
			Cvar_SetValue("vr_movement_mode", intValue);		
			break;
		case VR_OPTION_SNAP_TURN:
			intValue = (int)vr_snap_turn.value;
			intValue = CLAMP(0.0f, isLeft ? intValue - 45 : intValue + 45, 90.0f);
			Cvar_SetValue("vr_snap_turn", intValue);
			break;
		case VR_OPTION_TURN_SPEED:
			floatValue = vr_turn_speed.value;
			floatValue = CLAMP(0.0f, isLeft ? floatValue - 0.25f : floatValue + 0.25f, VR_MAX_TURN_SPEED);
			Cvar_SetValue("vr_turn_speed", floatValue);
			break;
		case VR_OPTION_MSAA:
			intValue = (int)vr_msaa.value;
			int max;
			glGetIntegerv(GL_MAX_SAMPLES, &max);
			intValue = CLAMP(0, isLeft ? intValue - 1 : intValue + 1, max - 1);
			Cvar_SetValue("vr_msaa", intValue);
			break;
	}

#undef _maxarray
#undef _sizeofarray
}

static void VR_MenuKey(int key)
{
	switch ( key ) {
		case K_ESCAPE:
			VID_SyncCvars(); // sync cvars before leaving menu. FIXME: there are other ways to leave menu
			S_LocalSound( "misc/menu1.wav" );
			M_Menu_Options_f();
			break;

		case K_UPARROW:
			S_LocalSound( "misc/menu1.wav" );
			vr_options_cursor--;
			if ( vr_options_cursor < 0 ) {
				vr_options_cursor = VR_OPTION_MAX - 1;
			}
			break;

		case K_DOWNARROW:
			S_LocalSound( "misc/menu1.wav" );
			vr_options_cursor++;
			if ( vr_options_cursor >= VR_OPTION_MAX ) {
				vr_options_cursor = 0;
			}
			break;

		case K_LEFTARROW:
			S_LocalSound ("misc/menu3.wav");
			VR_MenuKeyOption( key, vr_options_cursor );
			break;

		case K_RIGHTARROW:
			S_LocalSound ("misc/menu3.wav");
			VR_MenuKeyOption( key, vr_options_cursor );
			break;

		case K_ENTER:
			m_entersound = true;
			VR_MenuKeyOption( key, vr_options_cursor );
			break;

		default: break;
	}
}

static void VR_MenuDraw (void)
{
	int i, y;
	qpic_t *p;
	const char *title;

	y = 4;

	// plaque
	p = Draw_CachePic ("gfx/qplaque.lmp");
	M_DrawTransPic (16, y, p);


	// customize header
	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-p->width)/2, y, p);

	y += 28;

	// title
	title = "VR/HMD OPTIONS";
	M_PrintWhite ((320-8*strlen(title))/2, y, title);

	y += 16;

	for ( i = 0; i < VR_OPTION_MAX; i++ ) {
		switch ( i ) {
			case VR_OPTION_ENABLED:
				M_Print( 16, y, "            VR Enabled" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			/*case VR_OPTION_PERFHUD:
				M_Print( 16, y, "             Debug HMD" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;*/
			case VR_OPTION_AIMMODE:
				y += 4; // separation
				M_Print( 16, y, "              Aim Mode" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_DEADZONE:
				M_Print( 16, y, "              Deadzone" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_CROSSHAIR:
				M_Print( 16, y, "             Crosshair" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_CROSSHAIR_DEPTH:
				M_Print( 16, y, "       Crosshair Depth" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_CROSSHAIR_SIZE:
				M_Print( 16, y, "        Crosshair Size" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_CROSSHAIR_ALPHA:
				M_Print( 16, y, "       Crosshair Alpha" );
				VR_MenuPrintOptionValue( 220, y, i );
				break;
			case VR_OPTION_WORLD_SCALE:
				M_Print(16, y, "       World Scale");
				VR_MenuPrintOptionValue(220, y, i);
				break;
			case VR_OPTION_MOVEMENT_MODE:
				M_Print(16, y, "       Movement mode");
				VR_MenuPrintOptionValue(220, y, i);
				break;
			case VR_OPTION_SNAP_TURN:
				M_Print(16, y, "       Turn");
				VR_MenuPrintOptionValue(220, y, i);
				break;
			case VR_OPTION_TURN_SPEED:
				M_Print(16, y, "       Turn Speed");
				VR_MenuPrintOptionValue(220, y, i);
				break;
			case VR_OPTION_MSAA:
				M_Print(16, y, "       MSAA");
				VR_MenuPrintOptionValue(220, y, i);
				break;

			default: break;
		}

		// draw the blinking cursor
		if ( vr_options_cursor == i ) {
			M_DrawCharacter( 200, y, 12 + ((int)(realtime*4)&1) );
		}

		y += 8;
	}
}

void VR_Menu_f (void)
{
	const char *sound = "items/r_item1.wav";

	IN_Deactivate(modestate == MS_WINDOWED);
	key_dest = key_menu;
	m_state = m_vr;
	m_entersound = true;

	VR_MenuPlaySound( sound, 0.5 );
}
