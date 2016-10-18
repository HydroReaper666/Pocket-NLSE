#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <3ds.h>
#include <sf2d.h>
#include <sftd.h>
#include <sfil.h>

#include "constants.h"
#include "common.h"
#include "ids.h"
#include "log.h"
#include "gfx.h"
#include "menu.h"
#include "dir.h"
#include "ui.h"
#include "kb.h"
#include "actions.h"
#include "acres.h"
#include "backup.h"
#include "menus.h"

#define STACKSIZE (4 * 1024)

static int is_loaded = 0;
static u8* gardenData = NULL;
static u8* gardenPath = NULL;
static Handle sdmc_file_handle = 0;

static int fontheight = 11;

int get_loaded_status(){
	if(is_loaded == 0)
		return 0;
	else
		return 1;
}

//load & save
void load_menu(){
	int menuindex = 0;
	int menucount = 3;

	char headerstr[] = "Load/restore options";
	char* menu_entries[] = {
		"Load garden.dat from file",
		"Dump save from game & create backup",
		"Restore from backup"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				load_garden_file();
				break;
			case 1:
				dump_and_backup_garden();
				break;
			case 2:
				restore_backup();
				break;
		}
	}
}

void save_menu(){
	int menuindex = 0;
	int menucount = 2;

	char headerstr[] = "Save/inject options";
	char* menu_entries[] = {
		"Inject changes into cartridge",
		"Save garden.dat to file",
	};

	if(!gardenData){
		gfx_waitmessage("Please dump or load a file first!");
		return;
	}

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				inject_changes();
				break;
			case 1:
				save_garden_file();
				break;
		}
	}
}

//load menu
void load_garden_file(){
	Result ret;
	u64 size;
	u32 read;
	char* path;

	while(aptMainLoop()){
		path = browse_dir("Load garden.dat");
		if(!strcmp(path, "")){
			return;
		}
		ret = FSUSER_OpenFile(&sdmc_file_handle, sdmc_arch, fsMakePath(PATH_ASCII, path), FS_OPEN_WRITE, 0);
		if(ret){
			gfx_error(ret, __LINE__);
			return;
		}
		FSFILE_GetSize(sdmc_file_handle, &size);

		if(size == GARDEN_SIZE){
			gfx_displaymessage("Loading garden.dat...");
			gardenPath = (u8*)path;
			//load entire file into memory
			gardenData = malloc(size);
			ret = FSFILE_Read(sdmc_file_handle, &read, 0, gardenData, GARDEN_SIZE);
			if(ret){
				gfx_error(ret, __LINE__);
				FSFILE_Close(sdmc_file_handle);
			}
			gfx_waitmessage("Loading garden.dat... Done!");
			is_loaded = 1;
			
			return;
		}
		else{
			gfx_waitmessage("Loading garden.dat... Invalid file!");
			continue;
		}
	}
}

void dump_and_backup_garden(){
	gfx_displaymessage("Dumping save into memory...");
	gardenData = (u8*)game_to_buffer();
	gfx_displaymessage("Backing up to \"backup\"...");
	buffer_to_dir((char*)gardenData, "backup");
	gfx_waitmessage("Dumped save data to memory. Dumped memory to \"Backup\"");

	is_loaded = 1;
}

void restore_backup(){
	gfx_displaymessage("Restoring backup in \"backup\"...");
	gardenData = (u8*)dir_to_buffer("backup");
	buffer_to_game((char*)gardenData);
	gfx_waitmessage("Restoring backup in \"backup\"... Done!");
}

//save menu
void inject_changes(){
	gfx_displaymessage("Injecting changes into game...");
	writeChecksums(gardenData);
	buffer_to_game((char*)gardenData);
	gfx_waitmessage("Injecting changes into game... Done!");

	FSFILE_Close(sdmc_file_handle);
	free(gardenData);
	gardenData = NULL;

	is_loaded = 0;
}

void save_garden_file(){
	Result ret;
	u32 written;
	
	if(sdmc_file_handle == 0){
		if(gardenData)
			gfx_waitmessage("Please inject the save instead!");
		else
			gfx_waitmessage("Please load a file first!");
		return;
	}

	if(gfx_prompt("Overwrite previous file?", NULL))
		return;

	gfx_displaymessage("Saving (this might take a while)...");

	//update checksums so the save file isn't labeled as corrupt
	writeChecksums(gardenData);
	
	ret = FSFILE_Write(sdmc_file_handle, &written, 0, gardenData, GARDEN_SIZE, FS_WRITE_FLUSH);
	if(ret)
		gfx_error(ret, __LINE__);
	ret = FSFILE_Close(sdmc_file_handle);
	if(ret)
		gfx_error(ret, __LINE__);
	free(gardenData);
	gardenData = NULL;

	gfx_waitmessage("Saving (this might take a while)... Done!");
	is_loaded = 0;
}

//menus

void map_menu(){
	int menuindex = 0;
	int menucount = 4;

	char headerstr[] = "Map options";
	char* menu_entries[] = {
		"Grass options",
		"Unbury fossils (not yet working!)",
		"Water flowers (not yet working!)",
		"Map tile editor (not yet working!)"
	};
	
	if(gardenData == NULL){
			while(aptMainLoop()){
				hidScanInput();

				if(hidKeysDown() & KEY_A)
					break;
				
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
					ui_frame();
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
					sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
				sf2d_end_frame();
				if(is3dsx){
					sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
					sf2d_end_frame();
				}
				sf2d_swapbuffers();
			}
		return;
	}

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				grass_menu();
				break;
			case 1:
				unbury_fossils();
				break;
			case 2:
				water_flowers();
				break;
			case 3:
				//map_tile_editor();
				break;
		}
	}
}

void player_select(){
	int i;
	int player_offset;
	int menuindex = 0;
	int menucount = 4;

	char headerstr[] = "Select a player";
	char* menu_entries[4];
	
	if(gardenData == NULL){
			while(aptMainLoop()){
				hidScanInput();

				if(hidKeysDown() & KEY_A)
					break;
				
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
					ui_frame();
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
					sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
				sf2d_end_frame();
				if(is3dsx){
					sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
					sf2d_end_frame();
				}
				sf2d_swapbuffers();
			}
		return;
	}

	for(i = 0; i < 4; i++){
		menu_entries[i] = calloc(11, 1);
		player_offset = (OFFSET_PLAYERS+SIZE_PLAYER*i);
		menu_entries[i] = (char*)get_ustring(gardenData, player_offset+0x55a8, 10);
		if(get_ustring(gardenData, (OFFSET_PLAYERS+SIZE_PLAYER*(i+1))+0x55a8, 1)[0] == '\0')
			break;
	}
	menucount = i+1;


	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		player_menu(menuindex);
	}

	for(i = 0; i < menucount; i++){
		free(menu_entries[i]);
	}
}

void villager_select(){
	int i, j;
	int villager_offset;
	u16 id; //villager ID
	u32 status;
	bool is_boxed[10];
	int menuindex = 0;
	int menucount = 0;

	char headerstr[] = "Select a villager";
	char* menu_entries[10]; //max of 10 villagers per town
	
	if(gardenData == NULL){
			while(aptMainLoop()){
				hidScanInput();

				if(hidKeysDown() & KEY_A)
					break;
				
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
					ui_frame();
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
					sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
				sf2d_end_frame();
				if(is3dsx){
					sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
					sf2d_end_frame();
				}
				sf2d_swapbuffers();
			}
		return;
	}

	while(aptMainLoop()){
		for(i = 0; i < 10; i++){
			villager_offset = OFFSET_VILLAGERS+SIZE_VILLAGER*i;
			id = readByte2(gardenData, villager_offset);
			status = readByte4(gardenData, villager_offset+OFFSET_VILLAGER_STATUS);
			if(id > 0x14c || id == 0xffff){
				if(id == 0xffff)
					menu_entries[i] = "(Empty slot!)";
				else{
					menucount = i+1;
					break;
				}
			}
			if(status == (status | 0x00000001))
				is_boxed[i] = true;
			else
				is_boxed[i] = false;

			if(id < 0x14c && i == 9)
				menucount = i+1;

			for(j = 0; j < 333; j++){
				if(id == villagers[j].id){
					if(is_boxed[i]){
						menu_entries[i] = calloc(strlen(villagers[j].name)+9, 1);
						strcat(menu_entries[i], villagers[j].name);
						strcat(menu_entries[i], " (boxed)");
					}
					else
						menu_entries[i] = villagers[j].name;
				}
			}
		}

		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		villager_menu(menuindex);
	}

	for(i = 0; i < 10; i++){
		if(is_boxed[i])
			free(menu_entries[i]);
	}
}

void misc_menu(){
	int menuindex = 0;
	int menucount = 1;

	char headerstr[] = "Misc options";
	char* menu_entries[] = {
		"Unlock all PWPs"
	};
	
	if(gardenData == NULL){
			while(aptMainLoop()){
				hidScanInput();

				if(hidKeysDown() & KEY_A)
					break;
				
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
					ui_frame();
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
					sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
				sf2d_end_frame();
				if(is3dsx){
					sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
					sf2d_end_frame();
				}
				sf2d_swapbuffers();
			}
		return;
	}

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				unlock_all_pwps();
				break;
		}
	}
}


//map menu
void grass_menu(){
	int menuindex = 0;
	int menucount = 2;

	char headerstr[] = "Select an option";
	char* menu_entries[2] = {
		"Revive grass",
		"Strip grass"
	};
	
	if(gardenData == NULL){
			while(aptMainLoop()){
				hidScanInput();

				if(hidKeysDown() & KEY_A)
					break;
				
				sf2d_start_frame(GFX_TOP, GFX_LEFT);
					ui_frame();
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
					sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
				sf2d_end_frame();
				if(is3dsx){
					sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
					sf2d_end_frame();
				}
				sf2d_swapbuffers();
			}
		return;
	}

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			if(menuindex == 0)
				sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Reviving grass...");
			else
				sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Stripping grass...");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
		if(menuindex == 0)
			set_grass(gardenData, 0xFF);
		else
			set_grass(gardenData, 0x00);
		
		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;
			
			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				if(menuindex == 0)
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Reviving grass... Done!");
				else
					sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Stripping grass... Done!");
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}

void unbury_fossils(){
	//TODO
}

void water_flowers(){
	//TODO
}

void map_tile_editor(){
	//TODO
	int i, j;
	u8 acres[7*6];

	//get acre bytes into acres
	j = 0;
	for(i = 0; i < 7*6*2; i++){
		if(i == 7*6*2-1){
			acres[j] = readByte1(gardenData, OFFSET_MAP_ACRES+i-1);
			j++;
		}
		else if(i % 2 == 0){
			acres[j] = readByte1(gardenData, OFFSET_MAP_ACRES+i-2);
			j++;
		}
	}

	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sf2d_draw_rectangle(88-3, 36-3, 223+6, 179+6, COLOR_BLACK);
			sf2d_draw_rectangle(88, 36, 223, 179, RGBA8(0x77, 0x77, 0x77, 0xFF));
			for(i = 0; i < 6; i++){
				for(j = 0; j < 7; j++){
					if(i == 0)
						sf2d_draw_texture_scale(acre_textures[acres[(i*7)+(j+1)]], 88+j*32, 36, 0.49, 0.49);
					else
						sf2d_draw_texture_scale(acre_textures[acres[(i*7)+(j+1)]], 88+j*32, 24+i*32, 0.49, 0.49);
					sf2d_draw_rectangle(88+32, 36+20, 31, 31, RGBA8(0x00, 0x00, 0xFF, 0x03));
				}
			}
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}


//player select & player menu
void player_menu(int player){
	int menuindex = 0;
	int menucount = 7;

	char headerstr[] = "Select an option";
	char* menu_entries[] = {
		"Bank options",
		"Change gender (here be dragons!)",
		"Change tan shade",
		"Change hair style",
		"Change hair color",
		"Change eye color",
		"Change face type"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				bank_menu(player);
				break;
			case 1:
				change_gender(player);
				break;
			case 2:
				change_tan(player);
				break;
			case 3:
				change_hair_style(player);
				break;
			case 4:
				change_hair_color(player);
				break;
			case 5:
				change_eye_color(player);
				break;
			case 6:
				change_face(player);
				break;
		}
	}
}

void bank_menu(int player){
	int menuindex = 0;
	int menucount = 4;

	char headerstr[] = "Select an option";
	char* menu_entries[] = {
		"Max bank",
		"Wipe bank",
		"Set bank balance",
		"View bank IDs"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				max_bank(player);
				break;
			case 1:
				wipe_bank(player);
				break;
			case 2:
				select_bank_balance(player);
				break;
			case 3:
				view_bank_ids(player);
				break;
		}
	}
}

void select_bank_balance(int player){
	int menuindex = 0;
	int menucount = 10;

	char headerstr[] = "Select an option";
	char* menu_entries[] = {
		"10,000 bells",
		"25,000 bells",
		"50,000 bells",
		"100,000 bells",
		"250,000 bells",
		"500,000 bells",
		"1,000,000 bells",
		"5,000,000 bells",
		"10,000,000 bells",
		"25,000,000 bells"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
	
		set_bank_balance(player, menuindex);
	}
}

void set_bank_balance(int player, int menuindex){
	int i;
	char* str = NULL;
	
	int bank_offset = (OFFSET_PLAYERS+SIZE_PLAYER*player)+0x6b6c;
	int bank_ids_10k[] = {0xac, 0x18, 0x8f, 0x9d, 0x5b, 0x11, 0x14, 0xaa};
	int bank_ids_25k[] = {0xe6, 0xcb, 0x78, 0xf4, 0x04, 0xa7, 0x17, 0xd7};
	int bank_ids_50k[] = {0x8d, 0x47, 0xe7, 0x12, 0x4c, 0xee, 0x0b, 0x87};
	int bank_ids_100k[] = {0xc2, 0xa6, 0xf1, 0x18, 0x4f, 0x71, 0x18, 0x2b};
	int bank_ids_250k[] = {0x98, 0xd2, 0xce, 0xf1, 0x67, 0xa8, 0x00, 0xe3};
	int bank_ids_500k[] = {0x11, 0xb3, 0x06, 0xe4, 0x46, OFFSET_PLAYERS, 0x01, 0x68};
	int bank_ids_1m[] = {0xf2, 0x78, 0x52, 0x7e, 0xb5, 0x2e, 0x08, 0xf4};
	int bank_ids_5m[] = {0x3d, 0xa2, 0x18, 0x94, 0xb6, 0x46, 0x06, 0x45};
	int bank_ids_10m[] = {0x1f, 0x4f, 0xfb, 0x62, 0xf5, 0x72, 0x05, 0x85};
	int bank_ids_25m[] = {0x69, 0xa7, 0x62, 0x09, 0x04, 0x3e, 0x00, 0x35};
	//int bank_ids_50m[] = {};
	//int bank_ids_100m[] = {};
	//int bank_ids_250m[] = {};
	//int bank_ids_500m[] = {};

	switch(menuindex){
		case 0:
			str = "10,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_10k[i]);
			break;
		case 1:
			str = "25,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_25k[i]);
			break;
		case 2:
			str = "50,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_50k[i]);
			break;
		case 3:
			str = "100,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_100k[i]);
			break;
		case 4:
			str = "250,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_250k[i]);
			break;
		case 5:
			str = "500,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_500k[i]);
			break;
		case 6:
			str = "1,000,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_1m[i]);
			break;
		case 7:
			str = "5,000,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_5m[i]);
			break;
		case 8:
			str = "10,000,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_10m[i]);
			break;
		case 9:
			str = "25,000,000";
			for(i = 0; i < 8; i++)
				storeByte(gardenData, bank_offset+i, bank_ids_25m[i]);
			break;
	}

	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Setting bank balance to %s... Done!", str);
			sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void max_bank(int player){
	int i;
	//original IDs
	int bank_ids[] = {0x78,0x56,0xf9,0x8c,0x36,0x86,0x11,0x0d};
	int bank_offset = (OFFSET_PLAYERS+SIZE_PLAYER*player)+0x6b6c;

	sf2d_start_frame(GFX_TOP, GFX_LEFT);
		ui_frame();
		sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Maxing bank... ");
	sf2d_end_frame();
	if(is3dsx){
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		sf2d_end_frame();
	}
	sf2d_swapbuffers();

	for(i = 0; i < 8; i++){
		storeByte(gardenData, bank_offset+i, bank_ids[i]);
	}
	
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Maxing bank... Done!");
			sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void wipe_bank(int player){
	int i;
	//original IDs
	int bank_ids[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	int bank_offset = (OFFSET_PLAYERS+SIZE_PLAYER*player)+0x6b6c;

	sf2d_start_frame(GFX_TOP, GFX_LEFT);
		ui_frame();
		sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Wiping bank... ");
	sf2d_end_frame();
	if(is3dsx){
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		sf2d_end_frame();
	}
	sf2d_swapbuffers();

	for(i = 0; i < 8; i++){
		storeByte(gardenData, bank_offset+i, bank_ids[i]);
	}
	
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Wiping bank... Done!");
			sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void view_bank_ids(int player){
	int i;
	//original IDs
	int bank_ids[8];
	int bank_offset = (OFFSET_PLAYERS+SIZE_PLAYER*player)+0x6b6c;

	for(i = 0; i < 8; i++){
		bank_ids[i] = readByte1(gardenData, bank_offset+i);
	}
	
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			for(i = 0; i < 8; i++){
				sftd_draw_textf(font, 0, fontheight*(2+i), COLOR_WHITE, fontheight, "%d: 0x%02x", i, bank_ids[i]);
			}
			sftd_draw_text(font, 0, fontheight*11, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void change_gender(int player){
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 2;
	char* str = NULL;

	char headerstr[] = "Select a gender";
	char* menu_entries[] = {
		"Male",
		"Female"
	};

	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;
		else if(hidKeysDown() & KEY_B)
			return;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Warning! This will make your villagers forget who you");
			sftd_draw_text(font, 0, fontheight*3, COLOR_WHITE, fontheight, "are! Press A if you still wish to continue, otherwise,");
			sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "press B.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		switch(menuindex){
			case 0:
				storeByte(gardenData, player_offset+0x55ba, 0x00);
				break;
			case 1:
				storeByte(gardenData, player_offset+0x55ba, 0x01);
				break;
		}

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				if(menuindex == 0)
					str = "Changing gender to male... ";
				else
					str = "Changing gender to female... ";
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "%s Done!", str);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}

void change_tan(int player){
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 16;

	char headerstr[] = "Select tan value";
	char* menu_entries[16] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;
		
		storeByte(gardenData, player_offset+0x8, (u8)menuindex);

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Changing tan value to %d... Done!", menuindex);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}

void change_hair_style(int player){
	int i;
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 34;

	char headerstr[] = "Select a hair style";
	char* menu_entries[34]; //0-16 Male, 17-34 Female

	for(i = 0; i < 17; i++){
		menu_entries[i] = calloc(8, 1); //Male + ' ' + 2 chars + \0 = 8
		snprintf(menu_entries[i], 8, "Male %d", i);
		menu_entries[i+17] = calloc(10, 1);
		snprintf(menu_entries[i+17], 10, "Female %d", i);
	}
	menu_entries[16] = calloc(17, 1);
	snprintf(menu_entries[16], 16, "Male (beadhead)");
	menu_entries[16+17] = calloc(19, 1);
	snprintf(menu_entries[16+17], 18, "Female (beadhead)");

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		storeByte(gardenData, player_offset+0x04, (u8)menuindex);

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Changing hair style to %s... Done!", menu_entries[menuindex]);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
	for(i = 0; i < 34; i++)
		free(menu_entries[i]);
}

void change_hair_color(int player){
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 16;

	char headerstr[] = "Select a hair color";
	char* menu_entries[] = {
		"Dark brown",
		"Light brown",
		"Orange",
		"Light blue",
		"Gold",
		"Light green",
		"Pink",
		"White",
		"Black",
		"Auburn",
		"Red",
		"Dark blue",
		"Blonde",
		"Dark green",
		"Light purple",
		"Ash brown"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		storeByte(gardenData, player_offset+0x05, (u8)menuindex);

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Changing hair color to %s... Done!", menu_entries[menuindex]);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}

void change_eye_color(int player){
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 8;

	char headerstr[] = "Select a eye color";
	char* menu_entries[] = {"0", "1", "2", "3", "4", "5", "6", "7"};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		storeByte(gardenData, player_offset+0x07, (u8)menuindex);

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Changing eye color to %d... Done!", menuindex);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}

void change_face(int player){
	int player_offset = OFFSET_PLAYERS+SIZE_PLAYER*player;
	int menuindex = 0;
	int menucount = 12;

	char headerstr[] = "Select a face type";
	char* menu_entries[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		storeByte(gardenData, player_offset+0x06, (u8)menuindex);

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Changing face type to %d... Done!", menuindex);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_TOP);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}
}


//villager select & villager menu
void villager_menu(int villager){
	int menuindex = 0;
	int menucount = 3;

	char headerstr[] = "Select an option";
	char* menu_entries[] = {
		"Toggle whether boxed",
		"Reset villager to defaults (not yet working!)",
		"Set villager (not yet working!)"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		switch(menuindex){
			case 0:
				toggle_boxed(villager);
				break;
			case 1:
				reset_villager(villager);
				break;
			case 2:
				set_villager(villager);
				break;
		}
	}
}

void box_menu(int villager){
	//legacy menu, here for future reference
	int menuindex = 0;
	int menucount;
	if(debug == 2)
		menucount = 3;
	else
		menucount = 2;

	char headerstr[] = "Select an option";
	char* menu_entries[] = {
		"Box villager",
		"Unbox villager",
		"Display status IDs"
	};

	while(aptMainLoop()){
		display_menu(menu_entries, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		switch(menuindex){
			case 0:
				//box_villager(villager);
				break;
			case 1:
				//unbox_villager(villager);
				break;
			case 2:
				display_status_id(villager);
				break;
		}
	}
}

void toggle_boxed(int villager){
	int villager_offset = OFFSET_VILLAGERS+SIZE_VILLAGER*villager;
	u32 status = readByte4(gardenData, villager_offset+OFFSET_VILLAGER_STATUS);
	char* str;
	
	if(status == (status | 0x00000001)){ //check if last byte is odd (boxed)
		str = "Unboxing";
		status = (status & ~0x00000001); //unbox villager
	}
	else{
		str = "Boxing";
		status = (status | 0x00000001); //box villager
	}

	storeByte(gardenData, villager_offset+OFFSET_VILLAGER_STATUS, (status & 0x000000ff));

	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "%s villager... Done!", str);
			sftd_draw_text(font, 0, fontheight*3, COLOR_WHITE, fontheight, "Press the A key to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void reset_villager(int villager){
	villager = villager;
	//TODO
}

void set_villager(int villager){
	//TODO
	u16 id = 0;
	int villager_offset = OFFSET_VILLAGERS+SIZE_VILLAGER*villager;
	id = id;
	villager_offset = villager_offset;
}


//misc menu
void unlock_all_pwps(){
	unsigned int i;
	char* str;

	if(gardenData == NULL){
		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Please load a file first!");
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A key to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
		return;
	}
	u8 pwp_ids[]={
		0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
		0xff,0xff,0xff,0xff,0x2a,0xd6,0xe4,0x58
	};

	for(i = 0; i < sizeof(pwp_ids)/sizeof(u8); i++){
		storeByte(gardenData, 0x04d9c8+i, pwp_ids[i]);
	}
	
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			str = "Unlocking all PWPs... ";
			sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "%s Done!", str);
			sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}


//list test
void list_test(){
	unsigned int i;
	int menuindex = 0;
	int menucount;

	char headerstr[] = "Number list";
	char* list[27];

	for(i = 0; i < sizeof(list)/sizeof(list[0]); i++){
		list[i] = calloc(3, 1);
		sprintf(list[i], "%d", i);
	}

	while(aptMainLoop()){
		menucount = sizeof(list)/sizeof(list[0]);

		display_menu(list, menucount, &menuindex, headerstr);

		if(menuindex == -1)
			break;

		while(aptMainLoop()){
			hidScanInput();

			if(hidKeysDown() & KEY_A)
				break;

			sf2d_start_frame(GFX_TOP, GFX_LEFT);
				ui_frame();
				sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "menuindex: %d", menuindex);
				sftd_draw_text(font, 0, fontheight*4, COLOR_WHITE, fontheight, "Press the A button to continue.");
			sf2d_end_frame();
			if(is3dsx){
				sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
				sf2d_end_frame();
			}
			sf2d_swapbuffers();
		}
	}

	for(i = 0; i < sizeof(list)/sizeof(list[0]); i++){
		free(list[i]);
	}
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_text(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Press the A button to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}

void view_apt_id(){
	u64 id;
	APT_GetProgramID(&id);

	if(id == 0x000400000cce2e00)
		sf2d_set_clear_color(RGBA8(0x00, 0xFF, 0x00, 0xFF)); //green
	else
		sf2d_set_clear_color(RGBA8(0xFF, 0x00, 0x00, 0xFF)); //red

	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "APT ID: %llx", id);
			sftd_draw_text(font, 0, fontheight*3, COLOR_WHITE, fontheight, "Press the A key to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
	
	sf2d_set_clear_color(RGBA8(0x00, 0x2e, 0x00, 0xFF));
}

void display_status_id(int villager){
	int villager_offset = OFFSET_VILLAGERS+SIZE_VILLAGER*villager;
	u32 status_id = readByte4(gardenData, villager_offset+OFFSET_VILLAGER_STATUS);
	while(aptMainLoop()){
		hidScanInput();

		if(hidKeysDown() & KEY_A)
			break;

		sf2d_start_frame(GFX_TOP, GFX_LEFT);
			ui_frame();
			sftd_draw_textf(font, 0, fontheight*2, COLOR_WHITE, fontheight, "Status ID: 0x%x", status_id);
			sftd_draw_text(font, 0, fontheight*3, COLOR_WHITE, fontheight, "Press the A key to continue.");
		sf2d_end_frame();
		if(is3dsx){
			sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
			sf2d_end_frame();
		}
		sf2d_swapbuffers();
	}
}
