#include "FreeMU.h"
extern u8 MuCtr_StartMoveTowards(Unit*, u8 x, u8 y, u8, u8 flags); //0x8079DDD

static inline bool IsPosInvaild(s8 x, s8 y){
	return( (x<0) & (x>gMapSize.x) & (y<0) & (y>gMapSize.y) );
}

static inline bool IsCharNotOnMap(Unit* unit){
	if(-1==unit->xPos)
		return 1;
	return 0;
}

static inline bool IsCharInvaild(Unit* unit){
	if(0==unit)
		return 1;
	if(0==unit->pCharacterData)
		return 1;
	return 0;
}

/*
 *
 *	In Core
 *
*/
extern void MU_DisplayAsMMS(struct MUProc* proc); 

extern u16 GetCameraCenteredX(int x); 
extern u16 GetCameraAdjustedX(int x); 
extern u16 GetCameraCenteredY(int y); 
extern u16 GetCameraAdjustedY(int y); 
extern const ProcInstruction* gProc_CameraMovement; 

struct CamMoveProc {
    /* 00 */ PROC_HEADER;
	/* 2a */ short dummy; 
    /* 2C */ struct Vec2 to;
    /* 30 */ struct Vec2 from;
    /* 34 */ struct Vec2 watchedCoordinate;
    /* 38 */ s16 calibration;
    /* 3A */ s16 distance;
    /* 3C */ int frame;
    /* 40 */ s8 xCalibrated;
};
struct ProcCmd
{
    short opcode;
    short dataImm;
    const void* dataPtr;
};
extern struct ProcCmd gProcScr_CamMove[];


struct BmSt // Game State Struct
{
    /* 00 */ s8  mainLoopEndedFlag;

    /* 01 */ s8  gameLogicSemaphore;
    /* 02 */ s8  gameGfxSemaphore;

    /* 03 */ u8  _unk04;

    /* 04 */ u8  gameStateBits;

    /* 05 */ u8  _unk05;

    /* 06 */ u16 prevVCount;

    /* 08 */ u32 _unk08;

    /* 0C */ struct Vec2 camera;
    /* 10 */ struct Vec2 cameraPrevious;
    /* 14 */ struct Vec2 playerCursor;
    /* 18 */ struct Vec2 cursorPrevious;
    /* 1C */ struct Vec2 cursorTarget;
    /* 20 */ struct Vec2 playerCursorDisplay;
    /* 24 */ struct Vec2u mapRenderOrigin;
    /* 28 */ struct Vec2 cameraMax;

    /* 2C */ u16 itemUnk2C;
    /* 2E */ u16 itemUnk2E;

    /* 30 */ u16 unk30;
    /* 32 */ s16 unk32;
    /* 34 */ s16 unk34;
    /* 36 */ s8 unk36;
    /* 37 */ s8 unk37;
    /* 38 */ u8 altBlendACa;
    /* 39 */ u8 altBlendACb;
    /* 3A */ u8 altBlendBCa;
    /* 3B */ u8 altBlendBCb;
    /* 3C */ u8 just_resumed;
    /* 3D */ u8 unk3D;
    /* 3E */ u8 unk3E;
    /* 3F */ s8 unk3F;
};
extern struct BmSt gBmSt;


struct MenuRect { s8 x, y, w, h; };

struct MenuDef;
struct MenuItemDef;

struct MenuProc;
struct MenuItemProc;

struct MenuItemProc
{
    /* 00 */ PROC_HEADER;

    /* 2A */ short xTile;
    /* 2C */ short yTile;

    /* 30 */ const struct MenuItemDef* def;

    /* 34 */ struct TextHandle text;

    /* 3C */ s8 itemNumber;
    /* 3D */ u8 availability;
};

struct MenuItemDef
{
    /* 00 */ const char* name;

    /* 04 */ u16 nameMsgId, helpMsgId;
    /* 08 */ u8 color, overrideId;

    /* 0C */ u8(*isAvailable)(const struct MenuItemDef*, int number);

    /* 10 */ int(*onDraw)(struct MenuProc*, struct MenuItemProc*);

    /* 14 */ u8(*onSelected)(struct MenuProc*, struct MenuItemProc*);
    /* 18 */ u8(*onIdle)(struct MenuProc*, struct MenuItemProc*);

    /* 1C */ int(*onSwitchIn)(struct MenuProc*, struct MenuItemProc*);
    /* 20 */ int(*onSwitchOut)(struct MenuProc*, struct MenuItemProc*);
};

struct MenuDef
{
    /* 00 */ struct MenuRect rect;
    /* 04 */ u8 style;
    /* 08 */ const struct MenuItemDef* menuItems;

    /* 0C */ void(*onInit)(struct MenuProc*);
    /* 10 */ void(*onEnd)(struct MenuProc*);
    /* 14 */ void(*_u14)(struct MenuProc*);
    /* 18 */ u8(*onBPress)(struct MenuProc*, struct MenuItemProc*);
    /* 1C */ u8(*onRPress)(struct MenuProc*);
    /* 20 */ u8(*onHelpBox)(struct MenuProc*, struct MenuItemProc*);
};

extern struct MenuProc* StartSemiCenteredOrphanMenu(const struct MenuDef* def, int xSubject, int xTileLeft, int xTileRight);
extern const struct MenuDef gUnitActionMenuDef;
void FMU_open_um(struct FMUProc* proc){
	StartSemiCenteredOrphanMenu(&gUnitActionMenuDef, gBmSt.cursorTarget.x - gBmSt.camera.x, 1, 0x16);
	return;
}

/*
u16 NewGetCameraCenteredY(int y) {

    int result  = y - DISPLAY_HEIGHT / 2;

    if (result < 0) {
        result = 0;
    }

    if (result > gBmSt.cameraMax.y) {
        result = gBmSt.cameraMax.y;
    }

    return result &~ 0xF;
}
*/


enum {
    CAMERA_MARGIN_LEFT   = 16 * 7, //16 * 3,
    CAMERA_MARGIN_RIGHT  = 16 * 7,//16 * 11,
    CAMERA_MARGIN_TOP    = 16 * 5,//16 * 2,
    CAMERA_MARGIN_BOTTOM = 16 * 5,//16 * 7,
};

u16 NewGetCameraCenteredX(int x) {
    int result = gBmSt.camera.x;

    if (gBmSt.camera.x + CAMERA_MARGIN_LEFT > x) {
        result = x - CAMERA_MARGIN_LEFT < 0
            ? 0
            : x - CAMERA_MARGIN_LEFT;
    }

    if (gBmSt.camera.x + CAMERA_MARGIN_RIGHT < x) {
        result = x - CAMERA_MARGIN_RIGHT > gBmSt.cameraMax.x
            ? gBmSt.cameraMax.x
            : x - CAMERA_MARGIN_RIGHT;
    }

    return result;
}

u16 NewGetCameraCenteredY(int y) {
    int result = gBmSt.camera.y;

    if (gBmSt.camera.y + CAMERA_MARGIN_TOP > y) {
        result = y - CAMERA_MARGIN_TOP < 0
            ? 0
            : y - CAMERA_MARGIN_TOP;
    }

    if (gBmSt.camera.y + CAMERA_MARGIN_BOTTOM < y) {
        result = y - CAMERA_MARGIN_BOTTOM > gBmSt.cameraMax.y
            ? gBmSt.cameraMax.y
            : y - CAMERA_MARGIN_BOTTOM;
    }

    return result;
}


//[202BCBC..202BCBF]!!
s8 VeslyCenterCameraOntoPosition(struct Proc* parent, int x, int y) {
    struct CamMoveProc* proc;
	// camera is SHORT 0x0--p where -- is byte coord and p is number of pixels 

    int xTarget = NewGetCameraCenteredX(x*16);
    int yTarget = NewGetCameraCenteredY(y*16);
	

    if ((xTarget == gBmSt.camera.x) && (yTarget == gBmSt.camera.y)) {
        return 0;
    }
	
    if (ProcFind((const ProcInstruction*)&gProcScr_CamMove)) {
        return 0;
    }

    if (parent) {
        proc = (struct CamMoveProc*)ProcStartBlocking((const ProcInstruction*)&gProcScr_CamMove, parent);
    } else {
        proc = (struct CamMoveProc*)ProcStart((const ProcInstruction*)&gProcScr_CamMove, 3);
    }
    proc->from.x = gBmSt.camera.x;
    proc->from.y = gBmSt.camera.y;

    proc->to.x = xTarget;
    proc->to.y = yTarget;

    proc->watchedCoordinate.x = x;
    proc->watchedCoordinate.y = y;

    return 1;
}

#define  MU_SUBPIXEL_PRECISION 4
void pFMU_MainLoop(struct FMUProc* proc){
	
	
	struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
	if (muProc) { 
		if (!(ProcFind((const ProcInstruction*)gProc_CameraMovement))) { 
			//EnsureCameraOntoPosition((Proc*)proc,((muProc->xSubPosition)/16) >> 4, ((muProc->ySubPosition)/16) >> 4);
			int right = (muProc->facingId == MU_FACING_RIGHT); 
			int left = (muProc->facingId == MU_FACING_LEFT); 
			int up = (muProc->facingId == MU_FACING_UP);
			int down = (muProc->facingId == MU_FACING_DOWN);
			if (right) right = 1; 
			if (left) left = 1; 
			if (up) up = 1; 
			if (down) down = 1; 
			
			VeslyCenterCameraOntoPosition((Proc*)proc,((((muProc->xSubPosition)/16) >> 4) + right - left), ((((muProc->ySubPosition)/16) >> 4) + down - up));
			//EnsureCameraOntoPosition((Proc*)proc,gActiveUnit->xPos, gActiveUnit->yPos);
			//VeslyCenterCameraOntoPosition((Proc*)proc, gActiveUnit->xPos, gActiveUnit->yPos);
			
			//gGameState.cameraRealPos.x = GetCameraCenteredX(muProc->xSubPosition >> MU_SUBPIXEL_PRECISION) + (muProc->xSubOffset & 0xF);
			//gGameState.cameraRealPos.y = GetCameraCenteredY(muProc->ySubPosition >> MU_SUBPIXEL_PRECISION) + (muProc->ySubOffset & 0xF);
			
			//muProc->boolAttractCamera = true; 
			//#define adjustBorder 4
			//int right = (muProc->facingId == MU_FACING_RIGHT) * adjustBorder; 
			//int left = (muProc->facingId == MU_FACING_LEFT) * adjustBorder; 
			//int up = (muProc->facingId == MU_FACING_UP) * adjustBorder; 
			//int down = (muProc->facingId == MU_FACING_DOWN) * adjustBorder; 
			//gGameState.cameraRealPos.x = GetCameraAdjustedX((muProc->xSubPosition >> MU_SUBPIXEL_PRECISION) + right - left);
			//gGameState.cameraRealPos.y = GetCameraAdjustedY((muProc->ySubPosition >> MU_SUBPIXEL_PRECISION) + down - up);
			//muProc->boolAttractCamera = false; 
			//CenterCameraOntoPosition((Proc*)proc,((muProc->xSubPosition)/16) >> 4, ((muProc->ySubPosition)/16) >> 4);
			
			//gGameState.cameraRealPos.x = GetCameraAdjustedX(15); 
			//gGameState.cameraRealPos.y = GetCameraAdjustedY(12); 
		}
	}
	/*
	struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
	
	if (muProc) { 
		if (muProc->stateId >= MU_STATE_MOVEMENT) { 
		return; 
		}
	} 
	MU_EndAll();
	*/
	
	if(MU_Exists()){
		
		return;
	}
	pFMU_MoveUnit(proc);
	//if(pFMU_MoveUnit(proc) == yield) {
		//return; 
	//}
	//ProcGoto((Proc*)proc,0x1);
	return;
}


int pFMU_HanleContinueMove(struct FMUProc* proc){
	return yield;
}


extern void PlayerPhase_Suspend(void); 
void FMU_ClearActionAndSave(struct FMUProc* proc) { 
	PlayerPhase_Suspend(); 
} 

extern const struct SMSData NewStandingMapSpriteTable[];
void pFMU_UpdateSMS(struct FMUProc* proc){
  u32 tileIndex = (proc->FMUnit->pMapSpriteHandle->oam2Base & 0x3FF) - 0x80;
  u8 smsID = proc->FMUnit->pClassData->SMSId;
  u16 size = NewStandingMapSpriteTable[smsID].size;
  u8 width =  size < 2 ? 16 : 32;
  u8 height = size > 0 ? 32 : 16;
  u32 srcOffs[3] = {0, 0, 0};
  int frame = GetGameClock() % 72;
  
  // Do nothing if no different-direction facing idle sprites exist.
  if (FMU_idleSMSGfxTable[smsID] == NULL)
    return;
  
  // Decompress sms gfx.
  if (proc->smsFacing==2)
    Decompress(NewStandingMapSpriteTable[smsID].pGraphics, gGenericBuffer);    // Downward facing sms.
  else {
    Decompress(FMU_idleSMSGfxTable[smsID], gGenericBuffer);                 // Other direction-facing sms.
    srcOffs[0] = proc->smsFacing==3 ? proc->smsFacing-1 : proc->smsFacing;  // Up-facing sprite comes immediately after right.
  }
  
  // Move sms gfx into smsbuffer.
  srcOffs[0] = (srcOffs[0] << (7 + size)) * 3;
  srcOffs[1] = srcOffs[0] + (0x80 << (size << 2));
  srcOffs[2] = srcOffs[1] + (0x80 << (size << 2));
  CopyTileGfxForObj((void*)gGenericBuffer+srcOffs[0], (void*)gSMSGfxBuffer_Frame1+(tileIndex<<5), width>>3, height>>3);
  CopyTileGfxForObj((void*)gGenericBuffer+srcOffs[0], (void*)gSMSGfxBuffer_Frame2+(tileIndex<<5), width>>3, height>>3);
  CopyTileGfxForObj((void*)gGenericBuffer+srcOffs[0], (void*)gSMSGfxBuffer_Frame3+(tileIndex<<5), width>>3, height>>3);
  
  // Overwrite VRAM with new SMS next frame. Timings taken from 0x8026F2C, SyncUnitSpriteSheet.
  if (frame < 31)
    RegisterTileGraphics(gSMSGfxBuffer_Frame1, (void*)0x06011000, sizeof(gSMSGfxBuffer_Frame1));
  else if (frame < 35)
    RegisterTileGraphics(gSMSGfxBuffer_Frame2, (void*)0x06011000, sizeof(gSMSGfxBuffer_Frame2));
  else if (frame < 67)
    RegisterTileGraphics(gSMSGfxBuffer_Frame3, (void*)0x06011000, sizeof(gSMSGfxBuffer_Frame3));
  else
    RegisterTileGraphics(gSMSGfxBuffer_Frame2, (void*)0x06011000, sizeof(gSMSGfxBuffer_Frame2));
  return;
}

// This replaces MuCtr_OnEnd.
// Adapts different-facing SMS during free movement.
void pFMUCtr_OnEnd(Proc* proc){
  struct FMUProc* FMUproc = (FMUProc*)ProcFind(FreeMovementControlProc);
  
  MuCtr_OnEnd(proc);
  
  // Determine facing direction and update sms.
  if (FMUproc!=NULL && GetFreeMovementState())
    pFMU_UpdateSMS(FMUproc);

  return;
}

#define bufferFrames 3
int pFMU_MoveUnit(struct FMUProc* proc){	//Label 1
	u16 iKeyCur = gKeyState.heldKeys;
	if (gKeyState.ABLRPressed) 
		iKeyCur = 0; // do not move if ABLR were pressed 
	if (!(gKeyState.heldKeys)) { 
		if (gKeyState.lastPressKeys && (gKeyState.timeSinceNonStartSelect <= bufferFrames)) { 
			iKeyCur = iKeyCur | gKeyState.lastPressKeys; 
			proc->bufferPress = 0; 
		} // use latest button press within x frames 
	} 
	s8 x = gActiveUnit->xPos;
	s8 y = gActiveUnit->yPos;
	u8 facingCur = proc->smsFacing;
  
  
	u8 mD[10]; //moveDirections[10]; 
	mD[0] = MU_COMMAND_HALT; // default to do no movement 

	
	if(0xF0&iKeyCur){
		if(iKeyCur&0x10) {
      x++;
      proc->smsFacing = MU_FACING_RIGHT;
	  mD[0] = MU_COMMAND_MOVE_RIGHT;
    }
		else if(iKeyCur&0x20) {
      x--;
      proc->smsFacing = MU_FACING_LEFT;
	  mD[0] = MU_COMMAND_MOVE_LEFT;
    }
		else if(iKeyCur&0x40) {
      y--;
      proc->smsFacing = MU_FACING_UP;
	  mD[0] = MU_COMMAND_MOVE_UP;
    }
		else if(iKeyCur&0x80) {
      y++;
      proc->smsFacing = MU_FACING_DOWN;
	  mD[0] = MU_COMMAND_MOVE_DOWN;
    }
	}
	 
    if (facingCur != proc->smsFacing) { 
        pFMU_UpdateSMS(proc);
	} 
	else { 
		
		if( !IsPosInvaild(x,y) ){
			proc->xTo = x;
			proc->yTo = y;
		}
			
		if( FMU_CanUnitBeOnPos(gActiveUnit, x, y) ){
			if( !IsPosInvaild(x,y) ) { 
				
				MuCtr_StartMoveTowards(gActiveUnit, x, y, 0x10, 0x0);
				struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
				MU_DisableAttractCamera(muProc);
				//gGameState.cameraRealPos.x+= 16; 

				
				/*
				mD[1] = MU_COMMAND_CAMERA_ON; 
				mD[2] = MU_COMMAND_HALT;
				struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
				if (!muProc) { 
					muProc = MU_Create(gActiveUnit);
				} 
				MU_SetFacing(muProc, proc->smsFacing);
				MU_DisplayAsMMS(muProc);
				HideUnitSMS(gActiveUnit);
				MU_EnableAttractCamera(muProc);
				MU_StartMoveScript(muProc, &mD[0]); 
				gActiveUnit->xPos = x; 
				gActiveUnit->yPos = y; 
				//MuCtr_StartMoveTowards(gActiveUnit, x, y, 0x10, 0x0);
				//struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
				*/
				return yield; 
			} 
		}
		else {
			ProcGoto((Proc*)proc,0x2);
			return yield; 
		}
			
	} 
	return no_yield;
}




int pFMU_HandleKeyMisc(struct FMUProc* proc){	//Label 2
	u16 iKeyCur = gKeyState.heldKeys;
	if (!(gKeyState.heldKeys)) { 
		if (gKeyState.lastPressKeys && (gKeyState.timeSinceNonStartSelect <= bufferFrames)) { 
			iKeyCur = iKeyCur | gKeyState.lastPressKeys; 
			proc->bufferPress = 0; 
		} // use latest button press within x frames 
	} 
	if(1&iKeyCur){ 			//Press A
		ProcGoto((Proc*)proc,0x4); 
		return yield;
		}			
	if(2&iKeyCur){ 			//Press B
		ProcGoto((Proc*)proc,0x5); 
		return yield;
		}	
	if(2&(iKeyCur>>0x8)){ 	//Press L
		ProcGoto((Proc*)proc,0x6); 
		return yield;
		}
	if(1&(iKeyCur>>0x8)){ 	//Press R
		ProcGoto((Proc*)proc,0x7); 
		return yield;
		}
	if(4&iKeyCur){ 			//Press Select
		ProcGoto((Proc*)proc,0x8); 
		return yield;
		}
	if(8&iKeyCur){ 			//Press Start
		ProcGoto((Proc*)proc,0x9); 
		return yield;
		}
	return no_yield;
}

int pFMU_HandleSave(struct FMUProc* proc){	//KeyPress Default
	if( TimerDelay < ++proc->uTimer ){
		ProcGoto((Proc*)proc,0xE);
		proc->uTimer=0;
		return yield;
	}
	return no_yield;
}


void pFMU_PressA(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (1&iKeyCur) )
		return;
	
	ButtonFunc* it = &FMU_FunctionList_OnPressA[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}


void pFMU_PressB(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (2&iKeyCur) )
		return;
	
	ButtonFunc* it = &FMU_FunctionList_OnPressB[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}

void pFMU_PressL(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (2&iKeyCur>>0x8) )
		return;
	
	ButtonFunc* it = &FMU_FunctionList_OnPressL[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}

void pFMU_PressR(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (1&iKeyCur>>0x8) )
		return;
	
	ButtonFunc* it = &FMU_FunctionList_OnPressR[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}

void pFMU_PressStart(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (8&iKeyCur) )
		return;
	
	ButtonFunc* it = &FMU_FunctionList_OnPressStart[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}

void pFMU_PressSelect(struct FMUProc* proc){
	u16 iKeyCur = gKeyState.heldKeys;
	if( 0 == (4&iKeyCur) ) 			//Press Select
		return;

	ButtonFunc* it = &FMU_FunctionList_OnPressSelect[0];
	while(*it)
		if( (*it++)(proc) )
			return;
	return;
}



/*
void pFMU_MainLoop(struct FMUProc* proc){

	struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
	
	if (muProc) { 
		if (muProc->pMUConfig->commands[muProc->pMUConfig->currentCommand] != MU_COMMAND_HALT && muProc->stateId > MU_STATE_NONACTIVE) { 
			return; // MU_COMMAND_HALT is the terminator 
		} 
	} 
	else { 
		struct MUProc* muProc = MU_Create(gActiveUnit);
		MU_DisplayAsMMS(muProc);
		HideUnitSMS(gActiveUnit);
	} 
	

	ProcGoto((Proc*)proc,0x1);
	return;
}


void pFMU_MoveUnit(struct FMUProc* proc){	//Label 1
	u16 iKeyCur = gKeyState.heldKeys;
	s8 x = gActiveUnit->xPos;
	s8 y = gActiveUnit->yPos;
	
	//proc->xCur = x;
	//proc->xCur = y;
	//proc->xTo  = x;
	//proc->xTo  = y;
	u8 mD[10]; //moveDirections[10]; 
	mD[0] = MU_COMMAND_HALT; // default to do no movement 
	struct MUProc* muProc = MU_GetByUnit(gActiveUnit);
	
	
	if(0xF0&iKeyCur){
		if(iKeyCur&0x10) 		{ x++; if (muProc->facingId != MU_FACING_RIGHT) mD[0] = MU_COMMAND_FACE_RIGHT; else mD[0] = MU_COMMAND_MOVE_RIGHT; } 
		else if(iKeyCur&0x20)	{ x--; if (muProc->facingId != MU_FACING_LEFT) mD[0] = MU_COMMAND_FACE_LEFT; else mD[0] = MU_COMMAND_MOVE_LEFT; } 
		else if(iKeyCur&0x40)	{ y--; if (muProc->facingId != MU_FACING_UP) mD[0] = MU_COMMAND_FACE_UP; else mD[0] = MU_COMMAND_MOVE_UP; } 
		else if(iKeyCur&0x80)	{ y++; if (muProc->facingId != MU_FACING_DOWN) mD[0] = MU_COMMAND_FACE_DOWN; else mD[0] = MU_COMMAND_MOVE_DOWN; } 
	}
	mD[1] = MU_COMMAND_HALT; // halt 
	if (mD[0] > MU_COMMAND_HALT) {
		MU_StartMoveScript(muProc, &mD[0]); // always change directions freely 
		proc->facingId = mD[0] - 6; 
	}
	else { 
		if( !IsPosInvaild(x,y) ){
			proc->xTo = x;
			proc->yTo = y;
		}
			
		if( FMU_CanUnitBeOnPos(gActiveUnit, x, y) ){
			if( !IsPosInvaild(x,y) ) { } 
				MU_StartMoveScript(muProc, &mD[0]); 
				gActiveUnit->xPos = x; 
				gActiveUnit->yPos = y; 
				//MuCtr_StartMoveTowards(gActiveUnit, x, y, 0x10);
		}
	} 
	
	if (mD[0] == MU_COMMAND_HALT) { 
			ProcGoto((Proc*)proc,0x2);
	}
	return;
}

*/