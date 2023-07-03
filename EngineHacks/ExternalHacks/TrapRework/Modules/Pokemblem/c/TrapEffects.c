#include "gbafe.h"

extern void RemoveLightRune(struct Trap* trap); 

#define returnValue 0x94 

struct FMURam { 
	u8 state : 1; 
	u8 running : 1; 
	u8 dir : 2; 
	u8 silent : 1; 
	u8 use_dir : 1; 
};

extern struct FMURam* FreeMoveRam; 

extern void SetNewFlag(int); 
extern int CheckNewFlag_No_sC(int id);


extern struct Trap* AddTrapExtFix(int x, int y, int type, int ext1, int ext2, int ext3, int ext4, int ext5);  //! FE8U = (0x0802E2E0+1)
extern const void* GiveCoinsEvent; 
extern int CoinsTrapID_Link; 

void AlwaysInitTrap(struct EventTrapData* eTrap) { 
	AddTrapExtFix(eTrap->data[0], eTrap->data[1], eTrap->type, eTrap->data[2], eTrap->data[3], eTrap->data[4], 0, 0); 
} 
void InitIfNewFlagInByte3IsOff(struct EventTrapData* eTrap) { 
	if (!(CheckNewFlag_No_sC(eTrap->data[3]))) { 
		AddTrapExtFix(eTrap->data[0], eTrap->data[1], eTrap->type, eTrap->data[2], eTrap->data[3], eTrap->data[4], 0, 0); 
	}
} 


	
struct Trap* NewGetAdjacentTrap(struct Unit* unit, int trapID_low, int trapID_high) { 
	struct Trap* trap = 0; 
	int x = unit->xPos; 
	int y = unit->yPos; 
	
	if (FreeMoveRam->use_dir) { 
		//struct FMUProc* proc = (FMUProc*)ProcFind(FreeMovementControlProc);
		int dir = FreeMoveRam->dir; 
		if (dir==MU_FACING_LEFT)      x--;
		else if (dir==MU_FACING_RIGHT)x++;
		else if (dir==MU_FACING_DOWN) y++;
		else if (dir==MU_FACING_UP) y--;
		trap = GetTrapAt(x, y);
		if ((trap) && (trap->type >= trapID_low) && (trap->type <= trapID_high)) 
			return trap;
		else 
			return 0; 
	} 
	
	//for (int i = 0; i < 4; i++) { 
	//	if (i == 0) x++; 
	//	if (i == 1) x-= 2; 
	//	if (i == 2) { x++; y++; } 
	//	if (i == 3) y-= 2; 
	//	trap = GetTrapAt(x, y);
	//	if ((trap->type >= trapID_low) && (trap->type <= trapID_high)) 
	//		return trap;
	//} 
	return 0; 
} 

struct Trap* NewGetAdjacentTrapID(struct Unit* unit, int trapID) {
	return NewGetAdjacentTrap(unit, trapID, trapID); 
} 


struct CoinsTrap { 
	/* 00 */ u8 xPosition;
	/* 01 */ u8 yPosition;
	/* 02 */ u8 type;
	/* 03 */ u8 flag;
	/* 04 */ u16 gold; 
};



int NewObtainCoinsUsability(int trapID) { 
	struct CoinsTrap* trap = (struct CoinsTrap*)NewGetAdjacentTrapID(gActiveUnit, CoinsTrapID_Link); 
	if (trap) { 
		if (!CheckNewFlag_No_sC(trap->flag)) { 
			if (!(gActiveUnit->state & US_CANTOING)) {
				return true;  
			}
		} 
	}
	return 3; // false 
} 

int NewObtainCoinsUsability0x15(void) { 
	return NewObtainCoinsUsability(CoinsTrapID_Link); 
}


int TrapEffectCleanup(void) { 
	gActionData.unitActionType = 0x10; // from wait routine 
	SMS_UpdateFromGameData(); // needed while in FMU mode 
	return returnValue; //  play beep sound & end menu on next frame & clear menu graphics

} 


int NewObtainCoinsEffect(void) { 

	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, CoinsTrapID_Link); 
	if (trap) { 
	SetNewFlag(trap->data[0]); 
	short gold = trap->data[1] | trap->data[2]<<8; 
	gEventSlot[3] = gold; 
	
	CallMapEventEngine(&GiveCoinsEvent, EV_RUN_CUTSCENE);
	RemoveLightRune((struct Trap*)trap); // also fixes the terrain 
	}
	
	
	//int result = ObtainCoinsEffect(); 

	return TrapEffectCleanup(); 
} 

extern int BerryTrapID_Link; 
extern const void* PickBerryEvent; 
extern const void* NoBerriesEvent; 

int NewBerryTreeUsability(void) { 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, BerryTrapID_Link); 
	if (trap) 
		return true; 
	return 3; 
} 

int NewPickBerryTreeEffect(void) { 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, BerryTrapID_Link); 
	int berries = 0; 
	if (trap) berries = trap->data[0];
	if (berries) { 
		trap->data[0] = 0; // no more berries 
		gEventSlot[4] = berries; 
		CallMapEventEngine(&PickBerryEvent, EV_RUN_CUTSCENE);
		gActionData.unitActionType = 0x10; // only use up turn if we got a berry 
	} 
	else {
		CallMapEventEngine(&NoBerriesEvent, EV_RUN_CUTSCENE);
		// set cantoing? 
	}
	return returnValue; 
} 

int DisplayTextEffect(struct Trap* trap); 
extern int SignTrapID_Link; 
extern int Sign2TrapID_Link; 
extern int BlankExamineID_Link; 
extern int BlankTalkID_Link; 
extern int TutSignID_Link; 
extern int HelpMsgFlagOffset_Link; 
extern const void* TutTextEvent; 

int DisplayTextUsability(int id);

int DisplayTextUsability0x50(void) { 
	return DisplayTextUsability(SignTrapID_Link); 
} 
int DisplayTextUsability0x51(void) { 
	return DisplayTextUsability(Sign2TrapID_Link); 
} 
int DisplayTextUsability0x52(void) { 
	return DisplayTextUsability(BlankExamineID_Link); 
} 
int DisplayTextUsability0x53(void) { 
	return DisplayTextUsability(BlankTalkID_Link); 
} 
int DisplayTextUsability0x54(void) { 
	return DisplayTextUsability(TutSignID_Link); 
} 

int DisplayTextUsability(int id) { 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id);
	if (trap) { 
		if (!(CheckNewFlag_No_sC(trap->data[0]))) { 
		return true; 
		} 
	} 
	return 3; 
} 

int DisplayTextEffect0x50(void) { 
	int id1 = SignTrapID_Link; 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id1); 
	if (trap) SetNewFlag(trap->data[0]); // maybe should use a defined offset for the flag 
	//SetNewFlag(trap->data[0]<<3 | (HelpMsgFlagOffset_Link)); 
	return DisplayTextEffect(trap); 
} 

int DisplayTextEffect0x51(void) { 
	int id1 = Sign2TrapID_Link; 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id1); 
	if (trap) SetNewFlag(trap->data[0]); 
	return DisplayTextEffect(trap); 
} 

int DisplayTextEffect0x52(void) { 
	int id1 = BlankExamineID_Link; 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id1); 
	if (trap) SetNewFlag(trap->data[0]); 
	return DisplayTextEffect(trap); 
} 

int DisplayTextEffect0x53(void) { 
	int id1 = BlankTalkID_Link; 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id1); 
	if (trap) SetNewFlag(trap->data[0]); 
	return DisplayTextEffect(trap); 
} 

// sets a different flag 
int DisplayTextEffect0x54(void) { 
	int id1 = TutSignID_Link; 
	struct Trap* trap = NewGetAdjacentTrapID(gActiveUnit, id1); 
	if (trap) SetNewFlag(trap->data[0]<<3 | (HelpMsgFlagOffset_Link)); 
	return DisplayTextEffect(trap); 
} 

int DisplayTextEffect(struct Trap* trap) { 
	if (trap) { 
	SetNewFlag(trap->data[0]);
	int textID = trap->data[1] | trap->data[2]<<8; 
	gEventSlot[2] = textID; 
		
	if (textID) { 
		CallMapEventEngine(&TutTextEvent, EV_RUN_CUTSCENE);
		gActionData.unitActionType = 0x10; // only use up turn if we got a berry 
	} 
	} 
	return returnValue; 
} 



