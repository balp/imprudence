/** 
 * @file lscript_readlso.cpp
 * @brief classes to read lso file
 *
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include "linden_common.h"

#include "lscript_readlso.h"
#include "lscript_library.h"
#include "lscript_alloc.h"

LLScriptLSOParse::LLScriptLSOParse(FILE *fp)
{
	U8  sizearray[4];
	S32 filesize;
	S32 pos = 0;
	fread(&sizearray, 1, 4, fp);
	filesize = bytestream2integer(sizearray, pos);
	mRawData = new U8[filesize];
	fseek(fp, 0, SEEK_SET);
	fread(mRawData, 1, filesize, fp);

	initOpCodePrinting();
}

LLScriptLSOParse::LLScriptLSOParse(U8 *buffer)
{
	mRawData = buffer;
	initOpCodePrinting();
}

LLScriptLSOParse::~LLScriptLSOParse()
{
	delete [] mRawData;
}

void LLScriptLSOParse::printData(FILE *fp)
{
	


	printNameDesc(fp);

	printRegisters(fp);

	printGlobals(fp);

	printGlobalFunctions(fp);

	printStates(fp);

	printHeap(fp);
}

void LLScriptLSOParse::printNameDesc(FILE *fp)
{
	fprintf(fp, "=============================\n\n");
}

S32 gMajorVersion = 0;

void LLScriptLSOParse::printRegisters(FILE *fp)
{
	// print out registers first
	S32				i;

	fprintf(fp, "=============================\n");
	fprintf(fp, "Registers\n");
	fprintf(fp, "=============================\n");
	S32 version = get_register(mRawData, LREG_VN);
	if (version == LSL2_VERSION1_END_NUMBER)
	{
		gMajorVersion = LSL2_MAJOR_VERSION_ONE;
	}
	else if (version == LSL2_VERSION_NUMBER)
	{
		gMajorVersion = LSL2_MAJOR_VERSION_TWO;
	}
	for (i = LREG_IP; i < LREG_EOF; i++)
	{
		if (i < LREG_NCE)
		{
			fprintf(fp, "%s: 0x%X\n", gLSCRIPTRegisterNames[i], get_register(mRawData, (LSCRIPTRegisters)i));
		}
		else if (gMajorVersion == LSL2_MAJOR_VERSION_TWO)
		{
			U64 data = get_register_u64(mRawData, (LSCRIPTRegisters)i);
			fprintf(fp, "%s: 0x%X%X\n", gLSCRIPTRegisterNames[i], (U32)(data>>32), (U32)(data && 0xFFFFFFFF));
		}
	}
	fprintf(fp, "=============================\n\n");
}

void LLScriptLSOParse::printGlobals(FILE *fp)
{
	// print out registers first
	S32				offset, varoffset;
	S32				ivalue;
	F32				fpvalue;
	LLVector3		vvalue;
	LLQuaternion	qvalue;
	char			name[256];		/*Flawfinder: ignore*/
	U8				type;

	S32 global_v_offset = get_register(mRawData, LREG_GVR);
	S32 global_f_offset = get_register(mRawData, LREG_GFR);

	fprintf(fp, "=============================\n");
	fprintf(fp, "[0x%X] Global Variables\n", global_v_offset);
	fprintf(fp, "=============================\n");


	while (global_v_offset < global_f_offset)
	{

		// get offset to skip past name
		varoffset = global_v_offset;
		offset = bytestream2integer(mRawData, global_v_offset);
		
		// get typeexport
		type = *(mRawData + global_v_offset++);

		// set name
		bytestream2char(name, mRawData, global_v_offset);

		switch(type)
		{
		case LST_INTEGER:
			ivalue = bytestream2integer(mRawData, global_v_offset);
			fprintf(fp, "[0x%X] integer %s = %d\n", varoffset, name, ivalue);
			break;
		case LST_FLOATINGPOINT:
			fpvalue = bytestream2float(mRawData, global_v_offset);
			fprintf(fp, "[0x%X] integer %s = %f\n", varoffset, name, fpvalue);
			break;
		case LST_STRING:
			ivalue = bytestream2integer(mRawData, global_v_offset);
			fprintf(fp, "[0x%X] string %s = 0x%X\n", varoffset, name, ivalue + get_register(mRawData, LREG_HR) - 1);
			break;
		case LST_KEY:
			ivalue = bytestream2integer(mRawData, global_v_offset);
			fprintf(fp, "[0x%X] key %s = 0x%X\n", varoffset, name, ivalue + get_register(mRawData, LREG_HR) - 1);
			break;
		case LST_VECTOR:
			bytestream2vector(vvalue, mRawData, global_v_offset);
			fprintf(fp, "[0x%X] vector %s = < %f, %f, %f >\n", varoffset, name, vvalue.mV[VX], vvalue.mV[VY], vvalue.mV[VZ]);
			break;
		case LST_QUATERNION:
			bytestream2quaternion(qvalue, mRawData, global_v_offset);
			fprintf(fp, "[0x%X] quaternion %s = < %f, %f, %f, %f >\n", varoffset, name, qvalue.mQ[VX], qvalue.mQ[VY], qvalue.mQ[VZ], qvalue.mQ[VS]);
			break;
		case LST_LIST:
			ivalue = bytestream2integer(mRawData, global_v_offset);
			fprintf(fp, "[0x%X] list %s = 0x%X\n", varoffset, name, ivalue + get_register(mRawData, LREG_HR) - 1);
			break;
		default:
			break;
		}
	}

	fprintf(fp, "=============================\n\n");
}

void LLScriptLSOParse::printGlobalFunctions(FILE *fp)
{
	// print out registers first
	S32				i, offset;
//	LLVector3		vvalue;		unused
//	LLQuaternion	qvalue;		unused
	char			name[256];		/*Flawfinder: ignore*/
	U8				type;

	offset = get_register(mRawData, LREG_GFR);
	S32 start_of_state = get_register(mRawData, LREG_SR);
	if (start_of_state == offset)
		return;

	S32 global_f_offset = get_register(mRawData, LREG_GFR);

	fprintf(fp, "=============================\n");
	fprintf(fp, "[0x%X] Global Functions\n", global_f_offset);
	fprintf(fp, "=============================\n");


	S32 num_functions = bytestream2integer(mRawData, offset);
	S32 orig_function_offset;
	S32 function_offset;
	S32 next_function_offset = 0;
	S32 function_number = 0;
	S32 opcode_start;
	S32 opcode_end;

	for (i = 0; i < num_functions; i++)
	{
		// jump to function
		// if this is the first function
		if (i == 0)
		{
			if (i < num_functions - 1)
			{
				function_offset = bytestream2integer(mRawData, offset);
				next_function_offset = bytestream2integer(mRawData, offset);
				function_offset += global_f_offset;
				opcode_end = next_function_offset + global_f_offset;
			}
			else
			{
				function_offset = bytestream2integer(mRawData, offset);
				function_offset += global_f_offset;
				opcode_end = get_register(mRawData, LREG_SR);
			}
		}
		else if (i < num_functions - 1)
		{
			function_offset = next_function_offset;
			next_function_offset = bytestream2integer(mRawData, offset);
			function_offset += global_f_offset;
			opcode_end = next_function_offset + global_f_offset;
		}
		else
		{
			function_offset = next_function_offset;
			function_offset += global_f_offset;
			opcode_end = get_register(mRawData, LREG_SR);
		}
		orig_function_offset = function_offset;
		// where do the opcodes start
		opcode_start = bytestream2integer(mRawData, function_offset);
		opcode_start += orig_function_offset;
		bytestream2char(name, mRawData, function_offset);
		// get return type
		type = *(mRawData + function_offset++);
		fprintf(fp, "[Function #%d] [0x%X] %s\n", function_number, orig_function_offset, name);
		fprintf(fp, "\tReturn Type: %s\n", LSCRIPTTypeNames[type]);
		type = *(mRawData + function_offset++);
		S32 params;
		params = 0;
		S32 pcount = 0;
		while (type)
		{
			bytestream2char(name, mRawData, function_offset);
			fprintf(fp, "\tParameter #%d: %s %s\n", pcount++, LSCRIPTTypeNames[type], name);
			type = *(mRawData + function_offset++);
		}
		fprintf(fp, "\t\tOpCodes: 0x%X - 0x%X\n", opcode_start, opcode_end);
		printOpCodeRange(fp, opcode_start, opcode_end, 2);
		function_number++;
	}

	fprintf(fp, "=============================\n\n");
}

void LLScriptLSOParse::printStates(FILE *fp)
{
	// print out registers first
	S32				i, offset;
	U32 			j, k;
//	LLVector3		vvalue;		unused
//	LLQuaternion	qvalue;		unused
	char			name[256];		/*Flawfinder: ignore*/

	S32 state_offset = get_register(mRawData, LREG_SR);

	fprintf(fp, "=============================\n");
	fprintf(fp, "[0x%X] States\n", state_offset);
	fprintf(fp, "=============================\n");

	offset = state_offset;
	S32 num_states = bytestream2integer(mRawData, offset);
	S32 state_info_offset;
	S32 event_jump_table;
	U64 event_handlers;
	S32 event_offset;
	S32 original_event_offset;
	S32 opcode_start;
	S32 worst_case_opcode_end;
	S32 opcode_end;
	S32 stack_size;
	S32 read_ahead;
	S32 first_jump = 0;

	for (i = 0; i < num_states; i++)
	{
		state_info_offset = bytestream2integer(mRawData, offset);
		if (gMajorVersion == LSL2_MAJOR_VERSION_TWO)
			event_handlers = bytestream2u64(mRawData, offset);
		else
			event_handlers = bytestream2integer(mRawData, offset);
		if (!first_jump)
		{
			first_jump = state_info_offset;
		}
		read_ahead = offset;
		if (offset < first_jump + state_offset)
		{
			worst_case_opcode_end = bytestream2integer(mRawData, read_ahead) + state_offset;
		}
		else
		{
			worst_case_opcode_end = get_register(mRawData, LREG_HR);
		}
		state_info_offset += state_offset;
		fprintf(fp, "[0x%X] ", state_info_offset);
		state_info_offset += LSCRIPTDataSize[LST_INTEGER];
		bytestream2char(name, mRawData, state_info_offset);
		fprintf(fp, "%s\n", name);

		event_jump_table = state_info_offset;

		// run run through the handlers
		for (j = LSTT_STATE_BEGIN; j < LSTT_STATE_END; j++)
		{
			if (event_handlers & LSCRIPTStateBitField[j])
			{
				event_offset = bytestream2integer(mRawData, state_info_offset);
				stack_size = bytestream2integer(mRawData, state_info_offset);

				read_ahead = event_jump_table;

				S32 temp_end;
				S32 dummy;

				opcode_end = worst_case_opcode_end;

				for (k = LSTT_STATE_BEGIN; k < LSTT_STATE_END; k++)
				{
					if (event_handlers & LSCRIPTStateBitField[k])
					{
						temp_end = bytestream2integer(mRawData, read_ahead);
						dummy = bytestream2integer(mRawData, read_ahead);
						if (  (temp_end < opcode_end)
							&&(temp_end > event_offset))
						{
							opcode_end = temp_end;
						}
					}
				}

				if (event_offset)
				{
					event_offset += event_jump_table;
					if (opcode_end < worst_case_opcode_end)
						opcode_end += event_jump_table;
					original_event_offset = event_offset;

					fprintf(fp, "\t[0x%X] ", event_offset);

					opcode_start = bytestream2integer(mRawData, event_offset);
					opcode_start += original_event_offset;

					switch(j)
					{
					case LSTT_STATE_ENTRY:	// LSTT_STATE_ENTRY
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_STATE_EXIT:	// LSTT_STATE_EXIT
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_TOUCH_START:	// LSTT_TOUCH_START
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_TOUCH:	// LSTT_TOUCH
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_TOUCH_END:	// LSTT_TOUCH_END
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_COLLISION_START:	// LSTT_COLLISION_START
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_COLLISION:	// LSTT_COLLISION
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_COLLISION_END:	// LSTT_COLLISION_END
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_LAND_COLLISION_START:	// LSTT_LAND_COLLISION_START
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_LAND_COLLISION:	// LSTT_LAND_COLLISION
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_LAND_COLLISION_END:	// LSTT_LAND_COLLISION_END
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_INVENTORY:	// LSTT_INVENTORY
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						break;
					case LSTT_ATTACH:	// LSTT_ATTACH
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						break;
					case LSTT_DATASERVER:	// LSTT_DATASERVER
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						break;
					case LSTT_TIMER:	// LSTT_TIMER
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_MOVING_START:	// LSTT_MOVING_START
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_MOVING_END:	// LSTT_MOVING_END
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_CHAT:	// LSTT_CHAT
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						break;
					case LSTT_OBJECT_REZ:	// LSTT_OBJECT_REZ
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						break;
					case LSTT_REMOTE_DATA:	// LSTT_REMOTE_DATA
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						break;
					case LSTT_REZ:	// LSTT_REZ
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_SENSOR:	// LSTT_SENSOR
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						break;
					case LSTT_NO_SENSOR:	// LSTT_NO_SENSOR
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_CONTROL:	// LSTT_CONTROL
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						break;
					case LSTT_LINK_MESSAGE:	// LSTT_LINK_MESSAGE
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						break;
					case LSTT_MONEY:	// LSTT_MONEY
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						break;
					case LSTT_EMAIL:	// LSTT_EMAIL
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						break;
					case LSTT_AT_TARGET:	// LSTT_AT_TARGET
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tvector %s\n", name);
						break;
					case LSTT_NOT_AT_TARGET:	// LSTT_NOT_AT_TARGET
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_AT_ROT_TARGET:	// LSTT_AT_ROT_TARGET
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tquaternion %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tquaternion %s\n", name);
						break;
					case LSTT_NOT_AT_ROT_TARGET:	// LSTT_NOT_AT_TARGET
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						break;
					case LSTT_RTPERMISSIONS:	// LSTT_RTPERMISSIONS
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						break;
					case LSTT_HTTP_RESPONSE:	// LSTT_REMOTE_DATA ?!?!?!
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "%s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tkey %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tinteger %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tlist %s\n", name);
						bytestream2char(name, mRawData, event_offset);
						fprintf(fp, "\t\tstring %s\n", name);
						break;
					default:
						break;
					}
					fprintf(fp, "\t\tStack Size: %d\n", stack_size);
					fprintf(fp, "\t\t\tOpCodes: 0x%X - 0x%X\n", opcode_start, opcode_end);
					printOpCodeRange(fp, opcode_start, opcode_end, 3);
				}
			}
		}
	}
	fprintf(fp, "=============================\n\n");
}

void LLScriptLSOParse::printHeap(FILE *fp)
{
	// print out registers first

	S32 heap_offset = get_register(mRawData, LREG_HR);
	S32 heap_pointer = get_register(mRawData, LREG_HP);
	fprintf(fp, "=============================\n");
	fprintf(fp, "[0x%X - 0x%X] Heap\n", heap_offset, heap_pointer);
	fprintf(fp, "=============================\n");

	lsa_fprint_heap(mRawData, fp);
	
	fprintf(fp, "=============================\n\n");
}

void lso_print_tabs(FILE *fp, S32 tabs)
{
	S32 i;
	for (i = 0; i < tabs; i++)
	{
		fprintf(fp, "\t");
	}
}

void LLScriptLSOParse::printOpCodes(FILE *fp, S32 &offset, S32 tabs)
{
	U8 opcode = *(mRawData + offset);
	mPrintOpCodes[opcode](fp, mRawData, offset, tabs);
}

void LLScriptLSOParse::printOpCodeRange(FILE *fp, S32 start, S32 end, S32 tabs)
{
	while (start < end)
	{
		printOpCodes(fp, start, tabs);
	}
}

void LLScriptLSOParse::initOpCodePrinting()
{
	S32 i;
	for (i = 0; i < 256; i++)
	{
		mPrintOpCodes[i] = print_noop;
	}
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_NOOP]] = print_noop;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POP]] = print_pop;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPS]] = print_pops;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPL]] = print_popl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPV]] = print_popv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPQ]] = print_popq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPARG]] = print_poparg;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPIP]] = print_popip;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPBP]] = print_popbp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPSP]] = print_popsp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_POPSLR]] = print_popslr;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DUP]] = print_dup;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DUPS]] = print_dups;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DUPL]] = print_dupl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DUPV]] = print_dupv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DUPQ]] = print_dupq;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STORE]] = print_store;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STORES]] = print_stores;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREL]] = print_storel;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREV]] = print_storev;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREQ]] = print_storeq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREG]] = print_storeg;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREGS]] = print_storegs;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREGL]] = print_storegl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREGV]] = print_storegv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STOREGQ]] = print_storegq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADP]] = print_loadp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADSP]] = print_loadsp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADLP]] = print_loadlp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADVP]] = print_loadvp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADQP]] = print_loadqp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADGP]] = print_loadgp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADGSP]] = print_loadgsp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADGLP]] = print_loadglp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADGVP]] = print_loadgvp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LOADGQP]] = print_loadgqp;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSH]] = print_push;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHS]] = print_pushs;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHL]] = print_pushl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHV]] = print_pushv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHQ]] = print_pushq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHG]] = print_pushg;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHGS]] = print_pushgs;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHGL]] = print_pushgl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHGV]] = print_pushgv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHGQ]] = print_pushgq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHIP]] = print_puship;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHSP]] = print_pushsp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHBP]] = print_pushbp;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGB]] = print_pushargb;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGI]] = print_pushargi;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGF]] = print_pushargf;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGS]] = print_pushargs;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGV]] = print_pushargv;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGQ]] = print_pushargq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHE]] = print_pushe;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHEV]] = print_pushev;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHEQ]] = print_pusheq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PUSHARGE]] = print_pusharge;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_ADD]] = print_add;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_SUB]] = print_sub;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_MUL]] = print_mul;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_DIV]] = print_div;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_MOD]] = print_mod;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_EQ]] = print_eq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_NEQ]] = print_neq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LEQ]] = print_leq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_GEQ]] = print_geq;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_LESS]] = print_less;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_GREATER]] = print_greater;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BITAND]] = print_bitand;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BITOR]] = print_bitor;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BITXOR]] = print_bitxor;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BOOLAND]] = print_booland;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BOOLOR]] = print_boolor;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_SHL]] = print_shl;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_SHR]] = print_shr;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_NEG]] = print_neg;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BITNOT]] = print_bitnot;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_BOOLNOT]] = print_boolnot;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_JUMP]] = print_jump;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_JUMPIF]] = print_jumpif;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_JUMPNIF]] = print_jumpnif;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STATE]] = print_state;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_CALL]] = print_call;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_RETURN]] = print_return;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_CAST]] = print_cast;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STACKTOS]] = print_stacktos;
	mPrintOpCodes[LSCRIPTOpCodes[LOPC_STACKTOL]] = print_stacktol;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_PRINT]] = print_print;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_CALLLIB]] = print_calllib;

	mPrintOpCodes[LSCRIPTOpCodes[LOPC_CALLLIB_TWO_BYTE]] = print_calllib_two_byte;
}

void print_noop(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tNOOP\n", offset++);
}

void print_pop(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOP\n", offset++);
}

void print_pops(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPS\n", offset++);
}

void print_popl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPL\n", offset++);
}

void print_popv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPV\n", offset++);
}

void print_popq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPQ\n", offset++);
}

void print_poparg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPARG ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_popip(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPIP\n", offset++);
}

void print_popbp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPBP\n", offset++);
}

void print_popsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPSP\n", offset++);
}

void print_popslr(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPOPSLR\n", offset++);
}

void print_dup(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDUP\n", offset++);
}

void print_dups(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDUPS\n", offset++);
}

void print_dupl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDUPL\n", offset++);
}

void print_dupv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDUPV\n", offset++);
}

void print_dupq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDUPQ\n", offset++);
}

void print_store(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTORE $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_stores(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTORES $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_storel(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREL $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_storev(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREV $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_storeq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREQ $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_storeg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREG ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_storegs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGS ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_storegl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGL ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_storegv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGV ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_storegq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGQ ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_loadp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREP $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_loadsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREPS $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_loadlp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREPL $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_loadvp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREVP $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_loadqp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREQP $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_loadgp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_loadgsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGSP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_loadglp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGLP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_loadgvp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGVP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_loadgqp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTOREGQP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg + get_register(buffer, LREG_GVR));
}

void print_push(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSH $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHS $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHL $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHV $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHQ $BP + ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHG ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "0x%X\n", arg + get_register(buffer, LREG_GVR));
}

void print_pushgs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHGS ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "0x%X\n", arg + get_register(buffer, LREG_GVR));
}

void print_pushgl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHGL ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "0x%X\n", arg + get_register(buffer, LREG_GVR));
}

void print_pushgv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHGV ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "0x%X\n", arg + get_register(buffer, LREG_GVR));
}

void print_pushgq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHGQ ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "0x%X\n", arg + get_register(buffer, LREG_GVR));
}

void print_puship(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHIP\n", offset++);
}

void print_pushbp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHBP\n", offset++);
}

void print_pushsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHSP\n", offset++);
}

void print_pushargb(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGB ", offset++);
	arg = *(buffer + offset++);
	fprintf(fp, "%d\n", (U32)arg);
}

void print_pushargi(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGI ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_pushargf(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	F32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGF ", offset++);
	arg = bytestream2float(buffer, offset);
	fprintf(fp, "%f\n", arg);
}

void print_pushargs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	char arg[1024];		/*Flawfinder: ignore*/
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGS ", offset++);
	bytestream2char(arg, buffer, offset);
	fprintf(fp, "%s\n", arg);
}

void print_pushargv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	LLVector3 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGV ", offset++);
	bytestream2vector(arg, buffer, offset);
	fprintf(fp, "< %f, %f, %f >\n", arg.mV[VX], arg.mV[VY], arg.mV[VZ]);
}

void print_pushargq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	LLQuaternion arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGV ", offset++);
	bytestream2quaternion(arg, buffer, offset);
	fprintf(fp, "< %f, %f, %f, %f >\n", arg.mQ[VX], arg.mQ[VY], arg.mQ[VZ], arg.mQ[VS]);
}

void print_pushe(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHE\n", offset++);
}

void print_pushev(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHEV\n", offset++);
}

void print_pusheq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHEQ\n", offset++);
}

void print_pusharge(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPUSHARGE ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}


void print_add(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tADD ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_sub(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSUB ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_mul(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tMUL ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_div(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tDIV ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_mod(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tMOD ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_eq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tEQ ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_neq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tNEQ ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_leq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tLEQ ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_geq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tGEQ ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_less(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tLESS ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_greater(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tGREATER ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}


void print_bitand(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBITAND\n", offset++);
}

void print_bitor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBITOR\n", offset++);
}

void print_bitxor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBITXOR\n", offset++);
}

void print_booland(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBOOLAND\n", offset++);
}

void print_boolor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBOOLOR\n", offset++);
}

void print_shl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSHL\n", offset++);
}

void print_shr(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSHR\n", offset++);
}


void print_neg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 type;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tNEG ", offset++);
	type = *(buffer + offset++);
	fprintf(fp, "%s\n", LSCRIPTTypeNames[type]);
}

void print_bitnot(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBITNOT\n", offset++);
}

void print_boolnot(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tBOOLNOT\n", offset++);
}

void print_jump(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tJUMP ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_jumpif(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	U8 type;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tJUMPIF ", offset++);
	type = *(buffer + offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%s, %d\n", LSCRIPTTypeNames[type], arg);
}

void print_jumpnif(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	U8 type;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tJUMPNIF ", offset++);
	type = *(buffer + offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%s, %d\n", LSCRIPTTypeNames[type], arg);
}

void print_state(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTATE ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_call(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tCALL ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_return(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tRETURN\n", offset++);
}

void print_cast(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 types;
	U8 type1;
	U8 type2;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tCAST ", offset++);
	types = *(buffer + offset++);
	type1 = types >> 4;
	type2 = types & 0xf;
	fprintf(fp, "%s, %s\n", LSCRIPTTypeNames[type1], LSCRIPTTypeNames[type2]);
}

void print_stacktos(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTACKTOS ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_stacktol(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	S32 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tSTACKTOL ", offset++);
	arg = bytestream2integer(buffer, offset);
	fprintf(fp, "%d\n", arg);
}

void print_print(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tPRINT ", offset++);
	U8 type = *(buffer + offset++);
	fprintf(fp, "%s\n", LSCRIPTTypeNames[type]);
}

void print_calllib(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U8 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tCALLLIB ", offset++);
	arg = *(buffer + offset++);
	fprintf(fp, "%d (%s)\n", (U32)arg, gScriptLibrary.mFunctions[arg]->mName);
}


void print_calllib_two_byte(FILE *fp, U8 *buffer, S32 &offset, S32 tabs)
{
	U16 arg;
	lso_print_tabs(fp, tabs);
	fprintf(fp, "[0x%X]\tCALLLIB_TWO_BYTE ", offset++);
	arg = bytestream2u16(buffer, offset);
	fprintf(fp, "%d (%s)\n", (U32)arg, gScriptLibrary.mFunctions[arg]->mName);
}

