Everything is a DIE

Top level DIE is computation unit(CU) which reprsents a file 
Other files are his siblings 
Files has children which are contents of its file 
Variables, ptr, functions etc


Contents of the .debug_info section:

  Compilation Unit @ offset 0:
   Length:        0x8d (32-bit)
   Version:       5
   Unit Type:     DW_UT_compile (1)
   Abbrev Offset: 0
   Pointer Size:  8
 <0><c>: Abbrev Number: 2 (DW_TAG_compile_unit)
    <d>   DW_AT_producer    : (indirect string, offset: 0x4b): GNU C23 16.1.1 20260625 -mtune=generic -march=x86-64 -g
    <11>   DW_AT_language    : 29       (C11)
    <12>   DW_AT_language_name: 3       (C)
    <13>   DW_AT_language_version: 0x31647      (202311)
    <17>   DW_AT_name        : (indirect line string, offset: 0x22): main.c
    <1b>   DW_AT_comp_dir    : (indirect line string, offset: 0): /home/piggie/Projects/disinsector
    <1f>   DW_AT_low_pc      : 0x1139
    <27>   DW_AT_high_pc     : 0x1a
    <2f>   DW_AT_stmt_list   : 0
 <1><33>: Abbrev Number: 1 (DW_TAG_base_type)
    <34>   DW_AT_byte_size   : 8
    <35>   DW_AT_encoding    : 7        (unsigned)
    <36>   DW_AT_name        : (indirect string, offset: 0): long unsigned int
 <1><3a>: Abbrev Number: 1 (DW_TAG_base_type)
    <3b>   DW_AT_byte_size   : 4
    <3c>   DW_AT_encoding    : 7        (unsigned)
    <3d>   DW_AT_name        : (indirect string, offset: 0x5): unsigned int
 <1><41>: Abbrev Number: 1 (DW_TAG_base_type)
    <42>   DW_AT_byte_size   : 1
    <43>   DW_AT_encoding    : 8        (unsigned char)
    <44>   DW_AT_name        : (indirect string, offset: 0x2f): unsigned char
 <1><48>: Abbrev Number: 1 (DW_TAG_base_type)
    <49>   DW_AT_byte_size   : 2
    <4a>   DW_AT_encoding    : 7        (unsigned)
    <4b>   DW_AT_name        : (indirect string, offset: 0x12): short unsigned int
 <1><4f>: Abbrev Number: 1 (DW_TAG_base_type)
    <50>   DW_AT_byte_size   : 1
    <51>   DW_AT_encoding    : 6        (signed char)
    <52>   DW_AT_name        : (indirect string, offset: 0x31): signed char
 <1><56>: Abbrev Number: 1 (DW_TAG_base_type)
    <57>   DW_AT_byte_size   : 2
    <58>   DW_AT_encoding    : 5        (signed)
    <59>   DW_AT_name        : (indirect string, offset: 0x25): short int
 <1><5d>: Abbrev Number: 3 (DW_TAG_base_type)
    <5e>   DW_AT_byte_size   : 4
    <5f>   DW_AT_encoding    : 5        (signed)
    <60>   DW_AT_name        : int
 <1><64>: Abbrev Number: 1 (DW_TAG_base_type)
    <65>   DW_AT_byte_size   : 8
    <66>   DW_AT_encoding    : 5        (signed)
    <67>   DW_AT_name        : (indirect string, offset: 0x3d): long int
 <1><6b>: Abbrev Number: 1 (DW_TAG_base_type)
    <6c>   DW_AT_byte_size   : 1
    <6d>   DW_AT_encoding    : 6        (signed char)
    <6e>   DW_AT_name        : (indirect string, offset: 0x38): char
 <1><72>: Abbrev Number: 4 (DW_TAG_subprogram)
    <73>   DW_AT_external    : 1
    <73>   DW_AT_name        : (indirect string, offset: 0x46): main
    <77>   DW_AT_decl_file   : 1
    <78>   DW_AT_decl_line   : 3
    <79>   DW_AT_decl_column : 5
    <7a>   DW_AT_prototyped  : 1
    <7a>   DW_AT_type        : <0x5d>
    <7e>   DW_AT_low_pc      : 0x1139
    <86>   DW_AT_high_pc     : 0x1a
    <8e>   DW_AT_frame_base  : 1 byte block: 9c         (DW_OP_call_frame_cfa)
    <90>   DW_AT_call_all_tail_calls: 1
 <1><90>: Abbrev Number: 0
