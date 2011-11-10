/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include "dwarfmanager.h"

void EditLocationExpression (Dwarf_Byte_Ptr data, unsigned int pointer_size, unsigned long length, FileShdrPair & aPair)
{
	unsigned op;
	size_t bytes_read;
	Dwarf_Byte_Ptr end = data + length;

	while (data < end){
		op = *data++;

		switch (op){
		case DW_OP_addr:
			LinearAddr addr = READ_UNALIGNED4(data);
			LinearAddr relocatedAddr = aPair.iXIPFileDetails.Relocate(addr);
			WRITE_UNALIGNED4(data, relocatedAddr);
			data += pointer_size;
			break;
		case DW_OP_deref:
			break;
		case DW_OP_const1u:
		case DW_OP_const1s:
			data++;
			break;
		case DW_OP_const2u:
		case DW_OP_const2s:
			data += 2;
			break;
		case DW_OP_const4u:
		case DW_OP_const4s:
			data += 4;
			break;
		case DW_OP_const8u:
		case DW_OP_const8s:
			data += 8;
			break;
		case DW_OP_constu:
		case DW_OP_consts: 
		{
			ULEB128(data, bytes_read);
			break;
		}
		case DW_OP_dup:
		case DW_OP_drop:
		case DW_OP_over:
			break;
		case DW_OP_pick:
			data++;
			break;
		case DW_OP_swap:
		case DW_OP_rot:
		case DW_OP_xderef:
		case DW_OP_abs:
		case DW_OP_and:
		case DW_OP_div:
		case DW_OP_minus:
		case DW_OP_mod:
		case DW_OP_mul:
		case DW_OP_neg:
		case DW_OP_not:
		case DW_OP_or:
		case DW_OP_plus:
			break;
		case DW_OP_plus_uconst:
		{
			ULEB128(data, bytes_read);
			break;
		}
		case DW_OP_shl:
		case DW_OP_shr:
		case DW_OP_shra:
		case DW_OP_xor:
			break;
		case DW_OP_bra:
			data += 2;
			break;
		case DW_OP_eq:
		case DW_OP_ge:
		case DW_OP_gt:
		case DW_OP_le:
		case DW_OP_lt:
		case DW_OP_ne:
			break;
		case DW_OP_skip:
			data += 2;
			break;

		case DW_OP_lit0:
		case DW_OP_lit1:
		case DW_OP_lit2:
		case DW_OP_lit3:
		case DW_OP_lit4:
		case DW_OP_lit5:
		case DW_OP_lit6:
		case DW_OP_lit7:
		case DW_OP_lit8:
		case DW_OP_lit9:
		case DW_OP_lit10:
		case DW_OP_lit11:
		case DW_OP_lit12:
		case DW_OP_lit13:
		case DW_OP_lit14:
		case DW_OP_lit15:
		case DW_OP_lit16:
		case DW_OP_lit17:
		case DW_OP_lit18:
		case DW_OP_lit19:
		case DW_OP_lit20:
		case DW_OP_lit21:
		case DW_OP_lit22:
		case DW_OP_lit23:
		case DW_OP_lit24:
		case DW_OP_lit25:
		case DW_OP_lit26:
		case DW_OP_lit27:
		case DW_OP_lit28:
		case DW_OP_lit29:
		case DW_OP_lit30:
		case DW_OP_lit31:
			break;

		case DW_OP_reg0:
		case DW_OP_reg1:
		case DW_OP_reg2:
		case DW_OP_reg3:
		case DW_OP_reg4:
		case DW_OP_reg5:
		case DW_OP_reg6:
		case DW_OP_reg7:
		case DW_OP_reg8:
		case DW_OP_reg9:
		case DW_OP_reg10:
		case DW_OP_reg11:
		case DW_OP_reg12:
		case DW_OP_reg13:
		case DW_OP_reg14:
		case DW_OP_reg15:
		case DW_OP_reg16:
		case DW_OP_reg17:
		case DW_OP_reg18:
		case DW_OP_reg19:
		case DW_OP_reg20:
		case DW_OP_reg21:
		case DW_OP_reg22:
		case DW_OP_reg23:
		case DW_OP_reg24:
		case DW_OP_reg25:
		case DW_OP_reg26:
		case DW_OP_reg27:
		case DW_OP_reg28:
		case DW_OP_reg29:
		case DW_OP_reg30:
		case DW_OP_reg31:
			break;

		case DW_OP_breg0:
		case DW_OP_breg1:
		case DW_OP_breg2:
		case DW_OP_breg3:
		case DW_OP_breg4:
		case DW_OP_breg5:
		case DW_OP_breg6:
		case DW_OP_breg7:
		case DW_OP_breg8:
		case DW_OP_breg9:
		case DW_OP_breg10:
		case DW_OP_breg11:
		case DW_OP_breg12:
		case DW_OP_breg13:
		case DW_OP_breg14:
		case DW_OP_breg15:
		case DW_OP_breg16:
		case DW_OP_breg17:
		case DW_OP_breg18:
		case DW_OP_breg19:
		case DW_OP_breg20:
		case DW_OP_breg21:
		case DW_OP_breg22:
		case DW_OP_breg23:
		case DW_OP_breg24:
		case DW_OP_breg25:
		case DW_OP_breg26:
		case DW_OP_breg27:
		case DW_OP_breg28:
		case DW_OP_breg29:
		case DW_OP_breg30:
		case DW_OP_breg31:
		case DW_OP_fbreg:
		{
			ULEB128(data, bytes_read);
			break;
		}
		case DW_OP_bregx:
		{
			ULEB128(data, bytes_read);
			ULEB128(data, bytes_read);
			break;
		}
		case DW_OP_piece:
		{
			ULEB128(data, bytes_read);
			break;
		}
		case DW_OP_deref_size:
		case DW_OP_xderef_size:
			data++;
			break;
		case DW_OP_nop:
		  /* DWARF 3 extensions.  */
		case DW_OP_push_object_address:
		  break;
		case DW_OP_call2:
		  /* XXX: Strictly speaking for 64-bit DWARF3 files
		     this ought to be an 8-byte wide computation.  */
		  data += 2;
		  break;
		case DW_OP_call4:
		  /* XXX: Strictly speaking for 64-bit DWARF3 files
		     this ought to be an 8-byte wide computation.  */
		  data += 4;
		  break;
		case DW_OP_call_ref:
		  /* XXX: Strictly speaking for 64-bit DWARF3 files
		     this ought to be an 8-byte wide computation.  */
		  data += 4;
		  break;
		case DW_OP_form_tls_address:
		case DW_OP_call_frame_cfa:
		  break;
		case DW_OP_bit_piece:
		{
			// Handily the spec doesn't describe the operands - but by analogy with
			// DW_OP_piece we assume these are ULEB128 encoded.
			ULEB128(data, bytes_read);
			ULEB128(data, bytes_read);
			break;
		}
	
		/* GNU extensions.  */
		case DW_OP_GNU_push_tls_address:
		//case DW_OP_GNU_uninit:
			/* FIXME: Is there data associated with this OP ?  */
			break;

		default:
			// bail - can't do anything else sensible here
			cerr << "Warning: Unrecognized opcode " << op << " in Dwarf expression.\n";
		  return;
		}

	}
}
