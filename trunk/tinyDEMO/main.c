/*
* Copyright (C) 2009 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/
#include "main.h"
#include "common.h"

#include "invite.h"
#include "message.h"
#include "publish.h"
#include "register.h"
#include "subscribe.h"

#include <stdio.h>
#include <string.h>

#define LINE_DELIM "\n \r\n"

/* === global variables === */
context_t* context = tsk_null;

int execute(cmd_type_t cmd, const tsk_options_L_t* options);

/* === entry point === */
int main(int argc, char* argv[])
{
	char buffer[1024];
	tsk_options_L_t* options = tsk_null;
	cmd_type_t cmd;
	tsk_bool_t comment;
	int ret;
	int i, index;
	const char* start = tsk_null, *end = tsk_null;
	char c = '\n';

	/* Copyright */
	printf("Doubango Project\nCopyright (C) 2009 - 2010 Mamadou Diop \n\n");
	/* Initialize Network Layer ==> Mandatory */
	tnet_startup();
	/* Print Usage */
	//cmd_print_help();

	/* create user's context */
	if(!(context = tsk_object_new(context_def_t))){
		TSK_DEBUG_ERROR("Failed to create user's context.");
		goto bail;
	}

	/* reset the buffer */
	memset(buffer, '\0', sizeof(buffer));
	/* initial args */
	for(i=1 /* index zero contains the exe path */, index=0; i<argc; i++){
		memcpy(&buffer[index], argv[i], tsk_strlen(argv[i]));
		index += tsk_strlen(argv[i]);
	}
	
	/* If initial args ==> parse it now */
	if(index){
		TSK_DEBUG_INFO("Initial command-line: %s", buffer);
		goto init_buffer;
	}

	while(gets(buffer)){
init_buffer:
		start = buffer;
		end = start + tsk_strlen(start);
parse_buffer:
		if(!(options = cmd_parse(start, &cmd, &comment)) && cmd == cmd_none){
			if(comment){
				goto nex_line;
			}
			continue;
		}

		/* Load from configuration file? */
		if(cmd == cmd_config_file){
			FILE* file;
			const tsk_option_t* option;
			size_t read = 0;
			if((option = tsk_options_get_option_by_id(options, opt_path)) && !tsk_strnullORempty(option->value)){ /* --path option */
				if((file = fopen(option->value, "r"))){
					memset(buffer, '\0', sizeof(buffer));
					if(!(read = fread(buffer, sizeof(uint8_t), sizeof(buffer), file))){
						TSK_DEBUG_ERROR("[%s] is empty.", option->value);
						fclose(file), file = tsk_null;
						continue;
					}
					fclose(file), file = tsk_null;
					/* repplace all '\' with spaces (easier than handling that in the ragel file) */
					for(i=0; ((size_t)i)<read; i++){
						if(buffer[i] == '\\'){
							buffer[i] = ' ';
						}
					}
					goto init_buffer;
				}
				else{
					TSK_DEBUG_ERROR("Failed to open config-file [%s].", option->value);
				}
				continue;
			}
			else{
				TSK_DEBUG_ERROR("--config-file command must have --path option.");
				continue;
			}
		}
		
		/* execute current command */
		switch(cmd){
			case cmd_exit:
			case cmd_quit:
					TSK_DEBUG_INFO("Exit/Quit");
					goto bail;
			default:
				tsk_safeobj_lock(context);
				ret = execute(cmd, options);
				tsk_safeobj_unlock(context);
				break;
		}

		/* free options */
		TSK_OBJECT_SAFE_FREE(options);

		/* next line */
nex_line:
		if((index = tsk_strindexOf(start, (end - start), "\n")) !=-1){
			start += index;
			while((start < end) && isspace(*start)){
				start ++;
			}
			if((start + 2/*++*/) < end){
				goto parse_buffer; /* next line */
			}
			else{
				continue; /* wait for new commands */
			}
		}
		/*  */
	} /* while(buffer) */


bail:
	
	/* Destroy the user's context */
	TSK_OBJECT_SAFE_FREE(context);
	/* Uninitilize Network Layer */
	tnet_cleanup();

	getchar();

	return 0;
}


int execute(cmd_type_t cmd, const tsk_options_L_t* options)
{
	int ret = 0;

	switch(cmd){
		case cmd_audio:
			{
				TSK_DEBUG_INFO("command=audio");
				break;
			}
		case cmd_config_file:
			{
				TSK_DEBUG_INFO("command=config-file");
				break;
			}
		case cmd_config_session:
			{
				TSK_DEBUG_INFO("command=config-session");
				break;
			}
		case cmd_config_stack:
			{
				TSK_DEBUG_INFO("command=config-satck");
				ret = stack_config(options);
				break;
			}
		case cmd_exit:
			{
				TSK_DEBUG_INFO("command=exit");
				goto bail;
				break;
			}
		case cmd_file:
			{
				TSK_DEBUG_INFO("command=file");
				break;
			}
		case cmd_help:
			{
				TSK_DEBUG_INFO("command=help");
				cmd_print_help();
				break;
			}
		case cmd_message:
			{
				TSK_DEBUG_INFO("command=message");
				break;
			}
		case cmd_publish:
			{
				TSK_DEBUG_INFO("command=publish");
				break;
			}
		case cmd_quit:
			{
				TSK_DEBUG_INFO("command=quit");
				goto bail;
				break;
			}
		case cmd_register:
			{
				TSK_DEBUG_INFO("command=register");
				ret = register_handle_cmd(context, cmd, options);
				break;
			}
		case cmd_run:
			{
				TSK_DEBUG_INFO("command=run");
				ret = stack_run(options);
				break;
			}
		case cmd_sleep:
			{
				const tsk_option_t* option;
				int seconds;
				if((option = tsk_options_get_option_by_id(options, opt_sec)) && !tsk_strnullORempty(option->value)){ /* --sec option */
					seconds = atoi(option->value);
					TSK_DEBUG_ERROR("Sleeping %d seconds");
					tsk_thread_sleep(seconds*1000);
				}
				else{
					TSK_DEBUG_WARN("++sleep need --sec option.");
				}
				break;
			}
		case cmd_sms:
			{
				TSK_DEBUG_INFO("command=sms");
				break;
			}
		case cmd_subscribe:
			{
				TSK_DEBUG_INFO("command=subscribe");
				break;
			}
		case cmd_video:
			{
				TSK_DEBUG_INFO("command=video");
				break;
			}
		default:
			{
				TSK_DEBUG_ERROR("%d not a valid command.", cmd);
				break;
			}
	}

bail:
	return ret;
}