#include <errno.h>
#include <limits.h>

#include <iostream>

using namespace std;

#ifndef COMMON_H
#define COMMON_H
	#include "../include/common.h"
#endif

#include "../include/rom_core.h"
#include "../include/dump_core.h"

#ifndef SEARCHING_H
#define SEARCHING_H
	#include "../include/searching.h"
#endif

#include "../include/input.h"
#include "../include/output.h"
#include "../include/translate.h"

/*!
*	Helper function to clean up memory on program exit.
*
*	rom:	A pointer to the rom file being used.
*	dump:	A pointer the dump file being used.
*/
void cleanup( rom_file *rom, dump_file *dump )
{
	if( rom != NULL && rom->rom_buffer != NULL )
	{
		free( rom->rom_buffer );
	}

	if( dump != NULL )
	{
		if( dump->rom_buffer != NULL )
		{
			free( dump->rom_buffer );
		}

		if( dump->translated_buffer != NULL )
		{
			free( dump->translated_buffer );
		}
	}
}

int main( int argc, char** argv ) 
{
	rom_file rom 								= { 0 };
	passed_options options 						= { 0 };

	dump_file dump 								= { 0 };

	if( 1 == handle_input( &rom, &dump, &options, argc, argv ) )
		return 1;

	if( rom.rom_path != NULL )
	{
		if( get_rom_length( &rom ) == -1 )
		{
			//No need to cleanup here as we haven't allocated anything yet
			fprintf( stderr, "Error getting rom length.\n" );
			return -1;
		}

		rom.rom_buffer = (unsigned char*) malloc( rom.rom_length + 1 );

		dump_rom_into_buffer( &rom );

		if( options.dump_rom_flag ) 
		{
			print_buffer_contents_f( &rom, 0 );

			cleanup( &rom, &dump );
			return 0;
		}

		if( options.relative_search_text != NULL )
		{
			match_info matches = { 0 };

			if( -1 == relative_search( &rom, &matches, options.relative_search_text, options.fuzz_value) )
			{
				fprintf( stderr, "Error occured while searching.\n" );

				cleanup( &rom, &dump );
				return -1;
			}

			print_match_list( &rom, &matches, strlen( options.relative_search_text ) );

			if( matches.amount_of_matches > 0 )
			{
				size_t file_path_len =						 0;
				
				
				string inLine="";


				printf("Attempt to generate a translation file? (Yes, type file name. No, type nothing):\n");
				cin>> inLine;
				char *translate_file_path = new char[inLine.length() + 1];
        		strcpy(translate_file_path, inLine.c_str());
        		
				//Strip newline characters from input name
				//https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input/28462221#28462221
				translate_file_path[ strcspn( translate_file_path, "\r\n" ) ] = 0;

				if( strlen( translate_file_path ) > 0 )
				{
					int translation_return_info = 0;

					if( (translation_return_info = generate_translation_set_from_matches( &rom, translate_file_path, &matches, 
						options.relative_search_text )) < 0 )
					{
						if( translation_return_info == -1 )
						{
							fprintf( stderr, "Error while generating the translation file.\n" );

							if( translate_file_path != NULL )
							{
								free( translate_file_path );
							}
							cleanup( &rom, &dump );
							return -1;
						}
						else if( translation_return_info == -2 )
						{
							printf( "Ambiguous data set provided. Please select a data set to use: \n" );

							size_t match_number_length			= 0;

							char *temp_end_for_conversion;
							errno = 0;

							int match_set_to_use				= 0;
							
							string inLine2="";

							cin>> inLine2;
							char *match_number_selected = new char[inLine2.length() + 1];
        					strcpy(match_number_selected, inLine2.c_str());

							long temp_value = strtol( match_number_selected, &temp_end_for_conversion, 10);
							if( temp_end_for_conversion != match_number_selected && errno != ERANGE && 
								(temp_value >= INT_MIN || temp_value <= INT_MAX))
							{
								match_set_to_use = ( int ) temp_value;
							}

							if( match_set_to_use > 0 )
							{
								matches.location_matches[ 0 ] = matches.location_matches[ match_set_to_use - 1 ];
								matches.amount_of_matches = 1;
							}

							generate_translation_set_from_matches( &rom, translate_file_path, &matches, options.relative_search_text );

							if( match_number_selected != NULL )
							{
								free( match_number_selected );
							}
						}
					}
				}

				if( translate_file_path != NULL )
				{
					free( translate_file_path );
				}
			}

			cleanup( &rom, &dump );
			return 0;
		}

		if( options.translation_file_arg != NULL )
		{
			if( -1 == read_translation_file( options.translation_file_arg ) )
			{
				fprintf( stderr, "Error while reading the translation file.\n" );

				cleanup( &rom, &dump );
				return -1;
			}

			if( -1 == create_translated_rom( &rom ) )
			{
				fprintf( stderr, "Error while creating the translated rom.\n" );

				cleanup( &rom, &dump );
				return -1;
			}

			cleanup( &rom, &dump );
			return 0;
		}		
	}
	else if( dump.dump_path != NULL )
	{
		if( -1 == get_dump_amount_of_lines( &dump ) )
		{
			fprintf( stderr, "Error while parsing the dump file.\n" );

			cleanup( &rom, &dump );
			return -1;
		}

		dump.rom_buffer 				= (unsigned char*) malloc( dump.rom_length );
		dump.translated_buffer 			= (unsigned char*) malloc( dump.translated_length );

		if( -1 == read_dump_file( &dump ) )
		{
			fprintf( stderr, "Error while reading the dump file.\n" );

			cleanup( &rom, &dump );
			return -1;
		}

		if( options.translation_file_arg != NULL )
		{
			if( -1 == read_translation_file( options.translation_file_arg ) )
			{
				fprintf( stderr, "Error while reading the translation file.\n" );

				cleanup( &rom, &dump );
				return -1;
			}

			if( -1 == write_translated_dump( &dump ) )
			{
				fprintf( stderr, "Error while writing the new rom.\n" );

				cleanup( &rom, &dump );
				return -1;
			}
		}
		else if( options.rom_string_break != NULL )
		{
			if( options.strings_file_arg != NULL )
			{
				if( -1 == read_and_translate_dump_strings( &dump, options.strings_file_arg, options.start_address, 
					options.end_address, options.rom_string_break ))
				{
					fprintf( stderr, "Error while writing strings back to the dump.\n" );

					cleanup( &rom, &dump );
					return -1;
				}

				reprint_dump_contents( &dump, dump.rom_length );
			}
			else
			{
				if( -1 == write_dump_strings( &dump, options.start_address, options.end_address, options.rom_string_break ) )
				{
					fprintf( stderr, "Error while extracting the dump's strings.\n" );

					cleanup( &rom, &dump );
					return -1;
				}
			}
		}

		cleanup( &rom, &dump );
		return 0;
	}

	cleanup( &rom, &dump );
	return 0;
}
