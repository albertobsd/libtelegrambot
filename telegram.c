/*
 * Luis Alberto 
 * Twitter @albertobsd
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>
#include<sys/stat.h>
#include<errno.h>
#include"telegram.h"
#include"jsmn.h"


char *telegram_buffer;
int telegram_buffer_size = 1024;
int telegram_buffer_offset = 0;
char *telegram_baseurl = "https://api.telegram.org/bot";
char *telegram_fileurl = "https://api.telegram.org/file/bot";
char *telegram_token;
int telegram_error = 0;
int telegram_error_code = 0;
char *telegram_error_buffer;

char *message_names[] = {"message_id","from","date","chat",
"forward_from","forward_date","reply_to_message",
"text","audio","document","photo","sticker","video",
"voice","caption","contact","location",
"new_chat_participant","left_chat_participant",
"new_chat_title","new_chat_photo","delete_chat_photo",
"group_chat_created","supergroup_chat_created","channel_chat_created","migrate_to_chat_id","migrate_from_chat_id",NULL};
char *contact_names[] = {"phone_number","first_name","last_name","user_id",NULL};
char *chat_names[] = {"id","type","title","username","first_name","last_name",NULL};
char *chat_type_names[] = {"private","group","supergroup","channel",NULL};
char *sticker_names[] = {"file_id","width","height","thumb","file_size",NULL};
char *photosize_names[] = {"file_id","width","height","file_size",NULL};
char *voice_names[] = {"file_id","duration","mime_type","file_size",NULL};
char *location_names[] = {"longitude","latitude",NULL};
char *document_names[] = {"file_id","thumb","file_name","mime_type","file_size",NULL};
char *video_names[] = {"file_id","width","height","duration","thumb","mime_type","file_size",NULL};
char *response_names[] = {"ok","description","result","error_code",NULL};
char *user_names[] = {"id","first_name","last_name","username",NULL};
char *update_names[] = {"update_id","message","inline_query","chosen_inline_result",NULL};
char *file_names[] = {"file_id","file_size","file_path",NULL};

int message_states[3][5] = {{4,1,4,4,4},{4,4,4,2,4},{4,1,1,1,1}};
int parse_array_states[2][5] = {{4,4,1,4,4},{4,1,4,4,4}};


Response *telegram_parse_response(char *str,int *count)	{
	Response *response;
	int r,n =1,c,i =0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	response = calloc(2,sizeof(Response));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+2,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
				switch(indexOf(variable,response_names))	{
					case 0:
						response->ok = value;
					break;
					case 1:
						response->description = value;
						telegram_error_buffer = calloc(strlen(value)+2,sizeof(char));
						memcpy(telegram_error_buffer,value,strlen(value)*sizeof(char));
						entrar = 0;
					break;
					case 2:
						response->result = value;
						entrar = 0;
					break;
					case 3:
						response->error_code = strtol(value,NULL,10);
						telegram_error = 1;
						telegram_error_code =  response->error_code;
						free(value);
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 3:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
			default:
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return response;
}


int telegram_jsmn_init(jsmn_parser *parser,jsmntok_t **t,char *buffer,int *n)	{
	int r;
	jsmntok_t *toks;
	jsmn_init(parser);
	toks = calloc(n[0]+2,sizeof(jsmntok_t));
	if(toks != NULL)	{
		r = jsmn_parse(parser, buffer, strlen(buffer), toks, n[0]);
		while (r == JSMN_ERROR_NOMEM)	{
			n[0]*=2;
			toks = realloc(toks, sizeof(jsmntok_t) * (n[0] + 2));
			r = jsmn_parse(parser, buffer, strlen(buffer), toks, n[0]);
		}
		if (r == JSMN_ERROR_INVAL)	{
			telegram_set_error("jsmn_parse: invalid JSON string",JSMN_ERROR_INVAL);
		}
		if (r == JSMN_ERROR_PART)	{
			telegram_set_error("jsmn_parse: truncated JSON string",JSMN_ERROR_PART);
		}
		t[0] = toks;
	}
	else	{
		telegram_set_error("calloc: error",0xDEAD);
	}
	return r;
}

int telegram_set_error(char *error_str,int error_code)	{
	telegram_error_buffer = error_str;
	telegram_error_code = error_code;
	telegram_error = 1;
	return 0;
}

Response* telegram_free_response(Response *res)	{
	if(res)	{
		if(res->ok)	{
			free(res->ok);
		}
		if(res->result)	{
			free(res->result);
		}
		if(res->description)	{
			free(res->description);
		}
		memset(res,0,sizeof(Response));
		free(res);
	}
	return NULL;
}

User* telegram_free_user(User *user)	{
	if(user)	{
		if(user->username != NULL)	{
			free(user->username);
		}
		if(user->first_name != NULL)	{
			free(user->first_name);
		}
		if(user->last_name != NULL)	{
			free(user->last_name);
		}
		memset(user,0,sizeof(User));
		free(user);
	}
	return NULL;
}

int indexOf(char *str,char **ptr_strings)	{
	int i = 0,entrar = 1;
	while(entrar && ptr_strings[i] != NULL )	{
		if(strcmp(ptr_strings[i],str) == 0)	{
			entrar = 0;
			i--;
		}
		i++;
	}
	if(entrar)	{
		i = -1;
	}
	return i;
}

char * telegram_jsmn_get_token(jsmntok_t token,char *full)	{
	char *str;
	int size = token.end - token.start;
	str = calloc(size+2,sizeof(char));
	memcpy(str,full+token.start,size);
	return str;
}

void telegram_dump_token(jsmntok_t token,char *str)	{
	char *temp;
	temp = telegram_jsmn_get_token(token,str);
	printf("Token: %s\n",temp);
	free(temp);
	printf("type: %i\n",token.type);
	printf("start: %i\n",token.start);
	printf("end: %i\n",token.end);
	printf("size: %i\n",token.size);
#ifdef JSMN_PARENT_LINKS	
	printf("size: %i\n",token.parent);
#endif
}

User* telegram_getMe()	{
	CURL *curl;
	CURLcode res;
	Response *response = NULL;
	User *user = NULL;
	char *url = NULL;
	int i;
	url = telegram_makeurl("/getMe");
	curl = telegram_curl_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	free(url);
	if(res == CURLE_OK)	{
		response = telegram_parse_response(telegram_buffer,&i);
		if(!telegram_error)	{
			user = telegram_parse_user(response->result,&i);
		}
		telegram_free_response(response);
		telegram_reset_buffer();
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("curl: curl_easy_perform()",-1000);
	}
	return user;
}

File* telegram_getFile(char *file_id)	{
	CURL *curl;
	CURLcode res;
	Response *response = NULL;
	File *file = NULL;
	char *url = NULL;
	char *postdata = NULL;
	char *variables[] = {"file_id",NULL};
	char *values[] = {file_id,NULL};
	int i;
	if(file_id){
		curl = telegram_curl_init();
		url = telegram_makeurl("/getFile");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		postdata = telegram_build_post(variables,values);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
		res = curl_easy_perform(curl);
		free(url);
		if(res == CURLE_OK)	{
			response = telegram_parse_response(telegram_buffer,&i);
			if(!telegram_error)	{	
				file = telegram_parse_file(response->result,&i);
			}
			telegram_free_response(response);
			telegram_reset_buffer();
		}
		else	{
			telegram_set_error("curl_easy_perform() failed!",-1000);	
		}
		free(postdata);
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("NULL file_id",-10);
	}
	return file;
}

char *telegram_makeurl(char *telegram_method)	{
	char *url;
	int l;
	int size_base;
	int size_token;
	int size_method;
	size_base = strlen(telegram_baseurl);
	size_token = strlen(telegram_token);
	size_method = strlen(telegram_method);
	l = size_base + size_token + size_method;
	url = calloc(l+2,sizeof(char));
	if(url != NULL)	{
		snprintf(url,l+1,"%s%s%s",telegram_baseurl,telegram_token,telegram_method);
	}
	else	{
		fprintf(stderr,"calloc error\n");
		exit(1);
	}
	return url;
}

User* telegram_parse_user(char *str,int *count)	{
	User *user;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	char *token,*token1;
	jsmntok_t *toks;
	jsmn_parser parser;
	user = calloc(2,sizeof(User));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
				switch(indexOf(variable,user_names))	{
					case 0:
						user->id = strtol(value,NULL,10);
						free(value);
					break;
					case 1:
						user->first_name = value;
					break;
					case 2:
						user->last_name = value;
					break;
					case 3:
						user->username = value;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 3:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0] += r;
	return user;
}

int telegram_reset_buffer()	{
	memset(telegram_buffer,0,telegram_buffer_size);
	telegram_buffer_offset = 0;
	return 0;
};

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata )	{
	size_t new_len = telegram_buffer_offset + size*nmemb;
	if(new_len >= telegram_buffer_size)	{
		do	{
			telegram_buffer_size*=2;
		}while(new_len >= telegram_buffer_size);
		telegram_buffer = realloc(telegram_buffer, (telegram_buffer_size +2)*sizeof(char));
		if (telegram_buffer == NULL) {
			fprintf(stderr, "realloc() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	memcpy(telegram_buffer + telegram_buffer_offset, ptr, size*nmemb);
	telegram_buffer[new_len] = '\0';
	telegram_buffer_offset+= size*nmemb;
	return size*nmemb;
}

off_t fsize(const char *filename) {
	struct stat st;
	if (stat(filename, &st) == 0)
		return st.st_size;
	fprintf(stderr, "Cannot determine size of %s: %s\n",filename, strerror(errno));
	return -1;
}

Updates * telegram_getUpdates()	{
	CURL *curl;
	CURLcode res;
	Response *response = NULL;
	Updates *updates = NULL;
	char *url = telegram_makeurl("/getUpdates");
	int i;
	curl = telegram_curl_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	free(url);
	if(res == CURLE_OK)	{
		response = telegram_parse_response(telegram_buffer,&i);
		if(!telegram_error)	{
			if(response && response->result){
				updates = telegram_parse_updates(response->result,&i);
			}
		}
		telegram_free_response(response);
		telegram_reset_buffer();
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("curl: curl_easy_perform()",-1000);
	}
	return updates;
}

Updates * telegram_parse_updates(char *str,int *count)	{
	Updates *updates;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *value;
	updates = calloc(2,sizeof(Updates));
	updates->list = calloc(2,sizeof(Update*));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+2,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = parse_array_states[state][toks[i].type];
				i++;
			break;
			case 1:
				value = telegram_jsmn_get_token(toks[i],str);
				state = parse_array_states[state][toks[i].type];
				updates->list[updates->length] = telegram_parse_update(value,&i);
				updates->length++;
				updates->list = realloc(updates->list,(updates->length+2)*sizeof(Update *));
			break;
			case 4:
				telegram_dump_token(toks[i],str);
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r; 
	return updates;
}

Update * telegram_parse_update(char *str,int *count)	{
	Update *update = NULL;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	update = calloc(2,sizeof(Update));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				switch(indexOf(variable,update_names))	{
					case 0:
						update->update_id = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 1:
						update->item.message = telegram_parse_message(value,&i);
						free(value);
					break;
					case 2:
					case 3:
						telegram_set_error("Fixme",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						free(value);
						entrar = 0;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 3:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
			default:
			break;
		}

	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+= r;	
	return update;
}

Message* telegram_parse_message(char *str,int *count)	{
	Message *message;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	message = calloc(2,sizeof(Message));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,message_names);
				switch(c)	{
					case 0: //message_id
						message->message_id = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 1: //from
						message->from = telegram_parse_user(value,&i);
						free(value);
					break; 
					case 2: //date
						message->date = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 3:	//chat
						message->chat= telegram_parse_chat(value,&i);
						free(value);
					break;
					case 4:	//forward_from
						message->forward_from = telegram_parse_user(value,&i);
						free(value);
					break;
					case 5:	//forward_date
						message->forward_date = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 6:	//reply_to_message
						message->reply_to_message = telegram_parse_message(value,&i);
						free(value);
					break;
					case 7:	//text
						message->text = value;
						i++;
					break;
					case 8:	//audio
						printf("error with token %s\n",value);
						entrar = 0;
						free(value);
						i++;
					break;
					case 9:	//document
						message->document = telegram_parse_document(value,&i);
						free(value);
					break;
					case 10: //photo
						message->photo = telegram_parse_photos(value,&i);
						free(value);
					break;
					case 11:	

						message->sticker = telegram_parse_sticker(value,&i);
						free(value);
					break;
					case 12:	//video
						message->video = telegram_parse_video(value,&i);
					break;
					case 13:	//voice
						message->voice = telegram_parse_voice(value,&i);
						free(value);
					break;
					case 14:	//Cption
						message->caption = value;
						i++;
					break;
					case 15:	//contact
						message->contact = telegram_parse_contact(value,&i);
						free(value);
					break;
					case 16:	//location
						message->location = telegram_parse_location(value,&i);
						free(value);
					break;
					case 17:	//new_chat_participant
						message->new_chat_participant = telegram_parse_user(value,&i);
						free(value);
					break;
					case 18:	//left_chat_participant
						message->left_chat_participant = telegram_parse_user(value,&i);
						free(value);
					break;
					case 19:	//new_chat_title
						message->new_chat_title = value;
						i++;
					break;
					case 20:	//new_chat_photo
						message->new_chat_photo = telegram_parse_photos(value,&i);
						free(value);
					break;
					case 21:	//delete_chat_photo
						message->delete_chat_photo = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 22:	//group_chat_created
						message->group_chat_created = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 23:	//supergroup_chat_created
						message->supergroup_chat_created = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 24:	//channel_chat_created
						message->channel_chat_created = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 25:	//migrate_to_chat_id
						message->migrate_to_chat_id = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 26:	//migrate_from_chat_id
						message->migrate_from_chat_id = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
			default:
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return message;
}

Chat* telegram_parse_chat(char *str,int  *count)	{
	Chat *chat = NULL;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	chat = calloc(2,sizeof(Chat));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
				switch(indexOf(variable,chat_names))	{
					case 0: 
						chat->id = strtol(value,NULL,10);
						free(value);
					break;
					case 1:
						chat->type = indexOf(variable,chat_type_names);
					break; 
					case 2: 
						chat->title = value;
					break;
					case 3: 
						chat->username = value;
					break;
					case 4: 
						chat->first_name = value;
					break;
					case 5: 
						chat->last_name = value;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;

		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
 	return chat;	
}

Sticker* telegram_parse_sticker(char *str,int *count)	{
	Sticker *sticker = NULL;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	sticker = calloc(2,sizeof(Sticker));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				switch(indexOf(variable,sticker_names))	{
					case 0: 
						sticker->file_id = value;
						i++;
					break;
					case 1:
						sticker->width = strtol(value,NULL,10);
						free(value);
						i++;
					break; 
					case 2: 
						sticker->height = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 3: 
						sticker->thumb = telegram_parse_photosize(value,&i);
						free(value);
					break;
					case 4:
						sticker->file_size = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;

		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return sticker;
}

PhotoSize* telegram_parse_photosize(char *str,int *count)	{
	PhotoSize *photosize = NULL;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	photosize = calloc(2,sizeof(PhotoSize));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,photosize_names);
				switch(c)	{
					case 0: 
						photosize->file_id = value;
						i++;
					break;
					case 1:
						photosize->width = strtol(value,NULL,10);
						free(value);
						i++;
					break; 
					case 2: 
						photosize->height = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 3:
						photosize->file_size = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return photosize;
}

Photos* telegram_parse_photos(char *str,int *count)	{
	Photos *photos = NULL;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *value;
	photos = calloc(2,sizeof(Photos));
	photos->length = 0;
	photos->item = calloc(2,sizeof(PhotoSize *));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = parse_array_states[state][toks[i].type];
				i++;
			break;
			case 1:
				value = telegram_jsmn_get_token(toks[i],str);
				state = parse_array_states[state][toks[i].type];
				photos->item[photos->length] = telegram_parse_photosize(value,&i);
				photos->length++;
				photos->item = realloc(photos->item,(photos->length+2) * sizeof(PhotoSize*));
			break;		
			case 4:
				telegram_dump_token(toks[i],str);
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+= r;
	return photos;

}

Voice* telegram_parse_voice(char *str,int *count)	{
	Voice *voice;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	voice = calloc(2,sizeof(Voice));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,voice_names);
				i++;
				switch(c)	{
					case 0: 
						voice->file_id = value;
					break;
					case 1:
						voice->duration = strtol(value,NULL,10);
						free(value);
					break; 
					case 2: 
						voice->mime_type = value;
;
					case 3:
						voice->file_size = strtol(value,NULL,10);
						free(value);
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return voice;
}

Location* telegram_parse_location(char *str,int *count)	{
	Location *location;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	location = calloc(2,sizeof(Location));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,location_names);
				i++;
				switch(c)	{
					case 0: 
						location->longitude = strtof(value,NULL);
					break;
					case 1:
						location->latitude = strtof(value,NULL);
					break; 
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(value);
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return location;
}

Contact* telegram_parse_contact(char *str,int *count)	{
	Contact *contact;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	contact = calloc(2,sizeof(Contact));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,contact_names);
				i++;
				switch(c)	{
					case 0: 
						contact->phone_number = value;
					break;
					case 1:
						contact->first_name = value;
					break;
					case 2:
						contact->last_name = value;
					break;

					case 3:
						contact->user_id = strtol(value,NULL,10);
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return contact;
}


Document* telegram_parse_document(char *str,int *count)	{
	Document *document;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	document = calloc(2,sizeof(Document));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,document_names);
				switch(c)	{
					case 0: 
						document->file_id = value;
						i++;
					break;
					case 1:
						document->thumb = telegram_parse_photosize(value,&i);
						free(value);
					break;
					case 2:
						document->file_name = value;
						i++;
					break;

					case 3:
						document->mime_type = value;
						i++;
					break;

					case 4:
						document->file_size = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return document;
}

Video * telegram_parse_video(char *str,int *count)	{
	Video *video;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	video = calloc(2,sizeof(Video));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,video_names);
				switch(c)	{
					case 0: 
						video->file_id = value;
						i++;
					break;
					case 1:
						video->width = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 2:
						video->height = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 3:
						video->duration = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 4:
						video->thumb = telegram_parse_photosize(value,&i);
						free(value);
					break;
					case 5:
						video->mime_type = value;
						free(value);
						i++;
					break;
					case 6:
						video->file_size = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return video;
}

int telegram_init(char *token)	{
	int size;
	size = strlen(token);
	telegram_token = calloc(size,sizeof(char));
	if(telegram_token == NULL)	{
		fprintf(stderr, "calloc() failed\n");
		exit(1);
	}
	telegram_buffer = calloc(telegram_buffer_size,sizeof(char));
	if(telegram_buffer == NULL)	{
		fprintf(stderr, "calloc() failed\n");
		exit(1);
	}
	memcpy(telegram_token,token,size);
	return 0;
}

char *telegram_get_error()	{
	telegram_error = 0;
	return telegram_error_buffer;
}

int telegram_is_error()	{
	return telegram_error;
}

char* telegram_build_post(char **variables,char **values)	{
	int i = 0,l = 0,size= 0,offset = 0;
	char *buffer = NULL;
	//printf("%X %X\n",variables,values);
	if(variables && values)	{
		while( variables[l] && values[l])	{
			//printf("%x %x\n",variables[l],values[l]);
			size += (strlen(variables[l])+ strlen(values[l])+2);
			l++;
		}
	}
	//printf("size = %i\nl = %i\n",size,l);
	buffer = calloc(size+1,sizeof(char));
	if(buffer)	{
		while(i<l)	{
			if(i==0)	{
				sprintf(buffer+offset,"%s=%s",variables[i],values[i]);
				offset+= (1 +strlen(variables[i]) +strlen(values[i]));
			}
			else	{
				sprintf(buffer+offset,"&%s=%s",variables[i],values[i]);
				offset+= (1 +strlen(variables[i]) +strlen(values[i]));
			}
			i++;
		}
	}
	else	{
		fprintf(stderr,"error callo()");
		exit(0);
	}
	return buffer;
}

File* telegram_parse_file(char *str,int *count)	{
	File *file;
	int r,n =1,c,i = 0;
	unsigned int state = 0,entrar = 1;
	char *variable,*value;
	file = calloc(2,sizeof(File));
	jsmntok_t *toks;
	jsmn_parser parser;
	toks = calloc(n+1,sizeof(jsmntok_t));
	r = telegram_jsmn_init(&parser,&toks,str,&n);
	while(entrar && i < r)	{
		switch(state)	{
			case 0:
				state = message_states[state][toks[i].type];
				i++;
			break;
			case 1:
				variable = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				i++;
			break;		
			case 2:
				value = telegram_jsmn_get_token(toks[i],str);
				state = message_states[state][toks[i].type];
				c = indexOf(variable,file_names);
				switch(c)	{
					case 0: 
						file->file_id = value;
						i++;
					break;
					case 1:
						file->file_size = strtol(value,NULL,10);
						free(value);
						i++;
					break;
					case 2:
						file->file_path = value;
						free(value);
						i++;
					break;
					default:
						telegram_set_error("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
						entrar = 0;
					break;
				}
				free(variable);
			break;
			case 4:
				telegram_set_error("UNEXPECTED TOKEN TYPE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
				entrar = 0;
			break;
		}
	}
	memset(toks,0,n*sizeof(jsmntok_t));
	free(toks);
	count[0]+=r;
	return file;
	
}

char* telegra_makeDownloadURL(File *file)	{
	char *url = NULL;
	char *temp = NULL;
	int l;
	int size_base;
	int size_token;
	int size_path;
	size_base = strlen(telegram_fileurl);
	size_token = strlen(telegram_token);
	size_path = strlen(file->file_path);
	l = size_base + size_token + size_path;
	temp = calloc(l+10,sizeof(char));
	if(temp)	{
		snprintf(temp,l+3,"%s%s/%s",telegram_fileurl,telegram_token,file->file_path);
		url = telegram_process_slash(temp);
		free(temp);
	}
	else	{
		fprintf(stderr,"calloc error\n");
		exit(1);
	}
	return url;
}

char* telegram_downloadFile(File *file,char *name)	{
	CURL *curl;
	CURLcode res;
	char *toreturn = NULL;
	FILE *toLocalFile = NULL;
	char **test = NULL;
	char *url = telegra_makeDownloadURL(file);
	curl = telegram_curl_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	free(url);
	if(res == CURLE_OK)	{
		if(file->file_size == telegram_buffer_offset)	{
			toLocalFile = fopen(name,"w");
			if(toLocalFile)	{
				if(fwrite(telegram_buffer,sizeof(char),telegram_buffer_offset,toLocalFile) == telegram_buffer_offset)	{
					toreturn = name;
				}			
				else	{
					telegram_set_error("stdio: fwrite()",-1000);	
				}
				fclose(toLocalFile);
			}
			else	{
				telegram_set_error("stdio: fopen()",-1000);	
			}
			telegram_reset_buffer();
		}
		else	{
			printf("Diff %i %i\n",file->file_size,telegram_buffer_offset);
		}
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("curl: curl_easy_perform()",-1000);
	}
	return toreturn;
}

char* telegram_process_slash(char *str)	{
	int i = 0,j =0;
	char *temp = NULL;
	if(str)	{
		temp = calloc(strlen(str)+2,sizeof(char));
		while(str[j] != 0)	{ // '\0'
			if(str[j] == '\\')
				j++;
			temp[i] = str[j];
			i++;
			j++;
		}
	}	
	else	{
		telegram_set_error("NULL funtion argument",-10);	
	}
	return temp;
}

CURL* telegram_curl_init()	{
	CURL *curl  = NULL;
	//printf("CURL INIT\n");
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
	}
	return curl;
}

Updates* telegram_free_updates(Updates *updates)	{
	if(updates)	{
		int i = 0;
		while(i < updates->length)	{
			if(updates->list[i])
				telegram_free_update(updates->list[i]);
			i++;
		}
		memset(updates,0,sizeof(Updates));
		free(updates);
	}
	return NULL;
}

Update* telegram_free_update(Update *update)	{
	if(update)	{
		switch(update->type)	{
			case 0:
				telegram_free_message(update->item.message);
			break;
		}
		memset(update,0,sizeof(Update));
		free(update);
	}
	return NULL;
}

Message* telegram_free_message(Message *message)	{
	if(message)	{
		if(message->from)	{
			telegram_free_user(message->from);
		}
		if(message->chat)	{
			telegram_free_chat(message->chat);
		}
		if(message->forward_from)	{
			telegram_free_user(message->forward_from);
		}
		if(message->reply_to_message)	{
			telegram_free_message(message->reply_to_message);
		}
		if(message->audio)	{
			telegram_free_audio(message->audio);
		}
		if(message->document)	{
			telegram_free_document(message->document);
		}
		if(message->photo)	{
			telegram_free_photos(message->photo);
		}
		if(message->sticker)	{
			telegram_free_sticker(message->sticker);
		}
		if(message->video)	{
			telegram_free_video(message->video);
		}
		if(message->voice)	{
			telegram_free_voice(message->voice);
		}
		if(message->contact)	{
			telegram_free_contact(message->contact);
		}
		if(message->location)	{
			telegram_free_location(message->location);
		}
		if(message->new_chat_participant)	{
			telegram_free_user(message->new_chat_participant);
		}
		if(message->left_chat_participant)	{
			telegram_free_user(message->left_chat_participant);
		}
		if(message->new_chat_photo)	{
			telegram_free_photos(message->new_chat_photo);
		}
		memset(message,0,sizeof(Message));
		free(message);
	}
	return NULL;
}

Location* telegram_free_location(Location *location)	{
	if(location)	{
		memset(location,0,sizeof(Location));
		free(location);
	}
	return NULL;
}

Voice* telegram_free_voice(Voice *voice)	{
	if(voice)	{
		if(voice->file_id)	{
			free(voice->file_id);
		}
		if(voice->mime_type)	{
			free(voice->mime_type);
		}
		memset(voice,0,sizeof(Voice));
		free(voice);
	}
	return NULL;
}

Contact* telegram_free_contact(Contact *contact)	{
	if(contact)	{
		if(contact->phone_number)	{
			free(contact->phone_number);
		}
		if(contact->first_name)	{
			free(contact->first_name);
		}
		if(contact->last_name)	{
			free(contact->last_name);
		}
		memset(contact,0,sizeof(Contact));
		free(contact);
	}
	return NULL;
}

Document* telegram_free_document(Document *document)	{
	if(document)	{
		if(document->file_id)	{
			free(document->file_id);
		}
		if(document->mime_type)	{
			free(document->mime_type);
		}
		if(document->file_name)	{
			free(document->file_name);
		}
		if(document->thumb)	{
			telegram_free_photosize(document->thumb);
		}
		memset(document,0,sizeof(Document));
		free(document);
	}
	return NULL;
}

Chat* telegram_free_chat(Chat *chat)	{
	if(chat)	{
		if(chat->title)	{
			free(chat->title);
		}
		if(chat->username)	{
			free(chat->username);
		}
		if(chat->first_name)	{
			free(chat->first_name);
		}
		if(chat->last_name)	{
			free(chat->last_name);
		}
		memset(chat,0,sizeof(Chat));
		free(chat);
	}
	return NULL;
}

Audio* telegram_free_audio(Audio *audio)	{
	if(audio)	{
		if(audio->file_id)	{
			free(audio->file_id);
		}
		if(audio->performer)	{
			free(audio->performer);
		}
		if(audio->title)	{
			free(audio->title);
		}
		if(audio->mime_type)	{
			free(audio->mime_type);
		}
		memset(audio,0,sizeof(Audio));
		free(audio);
	}
	return NULL;
}

Sticker* telegram_free_sticker(Sticker *sticker)	{
	if(sticker)	{
		if(sticker->file_id)	{
			free(sticker->file_id);
		}
		if(sticker->thumb)	{
			telegram_free_photosize(sticker->thumb);
		}
		memset(sticker,0,sizeof(Sticker));
		free(sticker);
	}
	return NULL;
}

PhotoSize* telegram_free_photosize(PhotoSize *photosize)	{
	if(photosize)	{
		if(photosize->file_id)	{
			free(photosize->file_id);
		}
		memset(photosize,0,sizeof(PhotoSize));
		free(photosize);
	}
	return NULL;
}

Photos* telegram_free_photos(Photos *photos)	{
	int i = 0;
	if(photos)	{
		while(i < photos->length)	{
			if(photos->item[i])
				telegram_free_photosize(photos->item[i]);
			i++;
		}
		free(photos->item);
		memset(photos,0,sizeof(Photos));
		free(photos);
	}
	return NULL;
}

Video* telegram_free_video(Video *video)	{
	if(video)	{
		if(video->file_id)	{
			free(video->file_id);
		}
		if(video->mime_type)	{
			free(video->mime_type);
		}
		if(video->thumb)	{
			telegram_free_photosize(video->thumb);
		}
		memset(video,0,sizeof(Video));
		free(video);
	}
	return NULL;
}

Message* telegram_sendMessage(char *postdata)	{
	CURL *curl = NULL;
	CURLcode res;
	Response *response = NULL;
	Message *message = NULL;
	int i;
	char *url = NULL;
	url = telegram_makeurl("/sendMessage");
	curl = telegram_curl_init();
	curl_easy_setopt(curl, CURLOPT_URL , url);
	curl_easy_setopt(curl,  CURLOPT_POSTFIELDS , postdata);
	res = curl_easy_perform(curl);
	free(url);
	if(res == CURLE_OK)	{
		response = telegram_parse_response(telegram_buffer,&i);
		if(!telegram_error)	{
			message = telegram_parse_message(response->result,&i);
		}
		telegram_free_response(response);
		telegram_reset_buffer();
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("curl: curl_easy_perform()",-1000);
	}
	return message;		
}

Message* telegram_sendDocument(char *filename,char **variables, char **valores)	{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
	Response *response = NULL;
	Message *message = NULL;
	char *url = NULL;
	int i;
	if(filename){
		url = telegram_makeurl("/sendDocument");
		curl = telegram_curl_init();
		i = 0;
		while(variables[i] != NULL && valores[i] != NULL)	{
			curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, variables[i],CURLFORM_COPYCONTENTS, valores[i],CURLFORM_END);
			i++;
		}
		curl_formadd(&formpost,&lastptr,CURLFORM_COPYNAME, "document",CURLFORM_FILE, filename,CURLFORM_END);
		headerlist = curl_slist_append(headerlist, buf);
		curl_easy_setopt(curl, CURLOPT_URL , url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		res = curl_easy_perform(curl);
		if(res == CURLE_OK)	{
			response = telegram_parse_response(telegram_buffer,&i);
			if(!telegram_error)	{
				message = telegram_parse_message(response->result,&i);
			}
			else	{
				telegram_set_error(response->description,1);
			}
			free(url);
			curl_easy_cleanup(curl);
			curl_formfree(formpost);
			curl_slist_free_all (headerlist);
		}
		else	{	
			telegram_set_error("curl",1);
		}

	}
	else	{
		telegram_set_error("NULL argument",1);
	}
	return 	message;
}

Message* sendLocation(char postdata)	{
	CURL *curl = NULL;
	CURLcode res;
	Response *response = NULL;
	Message *message = NULL;
	int i;
	char *url = NULL;
	url = telegram_makeurl("/sendLocation");
	curl = telegram_curl_init();
	curl_easy_setopt(curl, CURLOPT_URL , url);
	curl_easy_setopt(curl,  CURLOPT_POSTFIELDS , postdata);
	res = curl_easy_perform(curl);
	free(url);
	if(res == CURLE_OK)	{
		response = telegram_parse_response(telegram_buffer,&i);
		if(!telegram_error)	{
			message = telegram_parse_message(response->result,&i);
		}
		telegram_free_response(response);
		telegram_reset_buffer();
		curl_easy_cleanup(curl);
	}
	else	{
		telegram_set_error("curl: curl_easy_perform()",-1000);
	}
	return message;		

}

