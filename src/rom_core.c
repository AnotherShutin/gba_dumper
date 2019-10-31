#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <fstream>
#include <string>

#include <iostream>

using namespace std;

#include "../include/rom_core.h"
#include "../include/utils/utils.h"

/*!
*	Given a path to a rom, determine its size.
*
*	rom:	A pointer to a rom_file with its rom_path member set to a valid path.
*
*	Fills rom->rom_length on success and returns length, returns -1 on invalid args or issue opening the rom.
*/
unsigned long get_rom_length( rom_file *rom )
{
	FILE *rom_file						= NULL;

	if( rom == NULL || rom->rom_path == NULL )
		return -1;

	rom_file = fopen( rom->rom_path, "rb" );
	if( rom_file == NULL )
		return -1;

	fseek( rom_file, 0, SEEK_END );
	rom->rom_length = ftell( rom_file );
	fseek( rom_file, 0, SEEK_SET );

	fclose( rom_file );

	return rom->rom_length;
}

/*!
*	Given a path to a rom, read the rom into an allocated buffer.
*
*	rom:	A pointer to a rom_file with an allocated rom_buffer member.
*
*	Returns 0 on success, -1 on invalid arguments or issue opening the rom.
*/
int dump_rom_into_buffer( rom_file *rom )
{
	FILE *rom_file						= NULL;

	if( rom == NULL || rom->rom_path == NULL || rom->rom_buffer == NULL )
		return -1;

	rom_file = fopen( rom->rom_path, "rb" );
	if( rom_file == NULL )
		return -1;

	fread( rom->rom_buffer, rom->rom_length, 1, rom_file );

	fclose( rom_file );

	return 0;
}

/*!
*	Given a path, create a translation file from the current byte->readable hash.
*
*	filename:	Path to the translation file.
*
*	Returns 0 on success, -1 on invalid arguments or issue opening the translation file.
*/
int create_translation_file( char *filename )
{
	FILE *translation_file 				= NULL;

	byte_to_readable_set *s 			= NULL;

	if( filename == NULL )
		return -1;

	translation_file = fopen( filename, "w" );
	if( translation_file == NULL )
		return -1;

	//This should ideally be done with HASH_ITER, but the HASH_ITER macro has issues handling
	//the value returned from get_byte_to_readable_hash. 
	for( s = get_byte_to_readable_hash(); s != NULL; s = s->hh.next ) 
	{
        print_buffer_as_bytes( translation_file, s->byte_value, 2 );
        print_character_translation( translation_file, s->readable);
    }

    fclose( translation_file );

	return 0;
}

/*!
*	Given a path, fill in the byte->readable and readable->byte hashes with information from the translation file.
*
*	filename:	Path to the translation file.
*
*	Returns 0 on success, -1 on invalid arguments or issue opening/parsing the translation file.
*/
int read_translation_file( char *filename )
{

	char readable 						= 0;
	string cur_line2	 = "";

	unsigned char byte_hex[ 2 ]			= { 0 };
	unsigned char byte_literal[ 4 ]		= { 0 };

	size_t cur_len 						= 0;

	ifstream dump_file(filename);

	while( getline( dump_file, cur_line2 ) )
	{
		char *cur_line = new char[cur_line2.length() + 1];
        strcpy(cur_line, cur_line2.c_str());
		sscanf( cur_line, "%s : %c", byte_literal, &readable );

		if( -1 == byte_literal_to_hex_value( byte_hex, (char*)byte_literal, 4 ) )
		{
			delete_hashes( );
			return -1;
		}

		if( readable == 0 )
		{
			readable = ' ';
		}

		add_element_to_hashes( byte_hex, readable );

		readable = 0;
	}

	dump_file.close();

	return 0;
}

/*!
*	Given a pointer to a rom file, create a translated dump of the rom. Requires the byte->readable hash to be filled.
*
*	Deletes the byte->readable and readable->byte hashes after successful translation.
*
*	rom:	A pointer to a rom_file with an allocated rom_buffer member.
*
*	Returns 0 on success, -1 on invalid arguments.
*/
int create_translated_rom( rom_file *rom )
{
	if( rom == NULL || get_byte_to_readable_hash() == NULL )
		return -1;

	print_buffer_contents_f( rom, rom->rom_length );

	delete_hashes( );

	return 0;
}
