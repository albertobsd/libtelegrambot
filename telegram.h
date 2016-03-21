#include<string.h>
#include<stdlib.h>
#include<curl/curl.h>
#include"jsmn.h"


enum chat_type	{
	chat_private = 0,
	chat_group = 1,
	chat_supergroup = 2,
	chat_channel = 3
};


struct PhotoSize	{
	char *file_id;
	int width;
	int height;
	int file_size;
};

struct Update	{
	int update_id;
	int type;
};

struct User	{
	int id;
	char *first_name;
	char *last_name;
	char *username;
};

struct Chat	{
	int id;
	enum chat_type type;
	char *title;
	char *username;
	char *first_name;
	char last_name;
};

struct Audio	{
	char *file_id;
	int duration;
	char *performer;
	char *title;
	char *mime_type;
	int file_size;
};

struct Document	{
	char *file_id;
 	struct PhotoSize *thumb;
	char *file_name;
	char *mime_type;
	int file_size;
};

struct Sticker	{
	char *file_id;
	int width;
	int height;
	struct PhotoSize *thumb;
	int file_size;
};

struct Video	{
	char *file_id;
	int width;
	int height;
	int duration;
	struct PhotoSize *thumb;
	char *mime_type;
	int file_size;
};

struct Message {
	int message_id;
	struct User *from;
	int date;
	struct Chat *chat;
	struct User *forward_from;
	int forward_date;
	struct Message *reply_to_message;
	char *text;
	struct Audio *audio;
	struct Document *document;
	struct PhotoSize **photo;
	struct Sticker *sticker;
	struct Video *video;
	struct Voice *voice;
	char *caption;
	struct Contact *contact;
	struct Location *location;
	struct User *new_chat_participant;
	struct User *left_chat_participant;
	char *new_chat_title;
	struct PhotoSize **new_chat_photo;
	int delete_chat_photo;
	int group_chat_created;
	int supergroup_chat_created;
	int channel_chat_created;
	int migrate_to_chat_id;
	int migrate_from_chat_id;
};

struct Voice	{
	char *file_id;
	int duration;
	char *mime_type;
	int file_size;
};

struct Contact	{
	char *phone_number;
	char *first_name;
	char *last_name;
	int user_id;
};

struct Location	{
	float longitude;
	float latitude;
};

struct UserProfilePhotos	{
	int total_count;
	struct PhotoSize **photos;
};

struct File	{
	char *file_id;
	int file_size;
	char *file_path;
};

struct ReplyKeyboardMarkup	{
	char **keyboard;
	int resize_keyboard;
	int one_time_keyboard;
	int selective;
};

struct ReplyKeyboardHide	{
	int hide_keyboard;
	int selective;
};

struct ForceReply	{
	int force_reply;
	int selective;
};

typedef enum {
	TELEGRAM_ERROR_API_RESPONSE = -10,
	TELEGRAM_ERROR_DEVELOPER = -20,
	TELEGRAM_ERROR_API_404 = 404,
	TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE  = -30,
	TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE = -40
}telegram_error_t;

char *response_str[2][7] = {
			{NULL,"ok","true","result",NULL,NULL,NULL},
			{NULL,"ok","false","error_code",NULL,"description",NULL}};
int response_table[2][7] ={{1,3,0,3,1,-1,-1},{1,3,0,3,0,3,3}};
int response_states[2][7] ={{2,2,1,2,3,-1,-1},{2,2,1,2,4,2,5}};
char *user_str[] = {"id","first_name","last_name","username",NULL};

int telegram_init(char *);
struct User * telegram_getMe();
char *telegram_makeurl(char *);
size_t write_callback(void *, size_t , size_t , void *);
char * jsmn_getTokenStr(jsmntok_t);
int telegram_parse_result();
struct User * telegram_parse_user();
int telegram_reset_buffer();
int telegram_setError(char *,int);
int indexOf(char *,char **);
void telegram_dump_user(struct User *);
void telegram_dump_token(jsmntok_t token);

char *telegram_token;
char *telegram_baseurl = "https://api.telegram.org/bot";
char *telegram_buffer;
int telegram_buffer_size = 1024;
int telegram_buffer_offset = 0;
int telegram_error = 0;
int telegram_error_code = 0;
char *telegram_error_buffer;

jsmntok_t *tokens;
jsmn_parser parser;

CURL *curl;
CURLcode res;

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
	url = calloc(l+10,sizeof(char));
	if(url != NULL)	{
		snprintf(url,l+1,"%s%s%s",telegram_baseurl,telegram_token,telegram_method);
	}
	else	{
		fprintf(stderr,"calloc error\n");
		exit(1);
	}
	return url;
}

int telegram_init(char *token)	{
	int size;
	size = strlen(token);
	telegram_token = calloc(size,sizeof(char));
	if(telegram_token == NULL)	{
		fprintf(stderr, "realloc() failed\n");
		exit(1);
	}
	telegram_buffer = calloc(telegram_buffer_size,sizeof(char));
	if(telegram_buffer == NULL)	{
		fprintf(stderr, "realloc() failed\n");
		exit(1);
	}
	memcpy(telegram_token,token,size);
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
		return 1;
	}
	else	{
		return 0;
	}
}

int telegram_end()	{
	memset(telegram_token,0,strlen(telegram_token));
	free(telegram_buffer);
	free(telegram_token);
	curl_global_cleanup();
	return 0;
}

struct User * telegram_getMe()	{
	struct User *user = NULL;
	char *url = telegram_makeurl("/getMe");
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	free(url);
	if(res != CURLE_OK)	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	}
	jsmn_init(&parser);
	if(telegram_parse_result() == 0){
		user = telegram_parse_user();
		//telegram_dump_user(user);
	}
	telegram_reset_buffer();
	return user;
}

struct User * telegram_getUpdates()	{
	struct User *user = NULL;
	char *url = telegram_makeurl("/getUpdates");
	//printf("%s\n",url);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	free(url);
	if(res != CURLE_OK)	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));
	}
	jsmn_init(&parser);
	telegram_parse_result();
	telegram_reset_buffer();
	return user;
}

int telegram_reset_buffer()	{
	memset(telegram_buffer,0,telegram_buffer_size);
	telegram_buffer_offset = 0;
	return 0;
};

int telegram_parse_result()	{
	int r;
	int code;
	unsigned int n =1,i = 0,state,entrar;
	char *token;
	tokens = calloc(n,sizeof(jsmntok_t));
	r = jsmn_parse(&parser, telegram_buffer, telegram_buffer_offset, tokens, n);
	while (r == JSMN_ERROR_NOMEM)	{
		n*=2;
		tokens = realloc(tokens, sizeof(jsmntok_t) * n);
		r = jsmn_parse(&parser, telegram_buffer, telegram_buffer_offset, tokens, n);
	}
	if (r == JSMN_ERROR_INVAL)	{
		telegram_setError("jsmn_parse: invalid JSON string",JSMN_ERROR_INVAL);
	}
	if (r == JSMN_ERROR_PART)	{
		telegram_setError("jsmn_parse: truncated JSON string",JSMN_ERROR_INVAL);
	}
	state = 0;
	entrar = 1;
	while(entrar && i< 7)	{
		switch(response_states[state][i])	{
			case 1:	//if token string mismach change of state to 1
				if(tokens[i].type == response_table[state][i]){
					token = jsmn_getTokenStr(tokens[i]);
					if(strcmp(token,response_str[state][i]) != 0)	{
						state = 1;
					}				
					free(token);
				}
				else	{
					telegram_setError("Unexpect token type",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
					entrar = 0;
				}
			break;
			case 2:	//Compare token.type and string compare
				if(tokens[i].type == response_table[state][i]){
					if(response_str[state][i] != NULL)	{
						token = jsmn_getTokenStr(tokens[i]);
						if(strcmp(token,response_str[state][i]) != 0)	{
							printf("Token mismatch: %s != %s\n",token,response_str[state][i]);
							telegram_setError("Unexpect token value",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
							entrar = 0;
						}				
						free(token);
					}
				}
				else	{
					telegram_setError("Unexpect token type",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
					entrar = 0;
				}
			break;
			case 3: //Return Object Token in telegram buffer, end while
				if(tokens[i].type == response_table[state][i]){
					token = jsmn_getTokenStr(tokens[i]);
					memset(telegram_buffer,0,telegram_buffer_size);
					memcpy(telegram_buffer,token,strlen(token));			
					free(token);
					telegram_buffer_offset = strlen(token);
					telegram_error = 0;
					entrar = 0;
				}
				else	{
					telegram_setError("Unexpect token type",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
					entrar = 0;
				}
			break;
			case 4: //mov token int value to error code
				if(tokens[i].type == response_table[state][i]){
					token = jsmn_getTokenStr(tokens[i]);
					telegram_error_code = strtol(token,NULL,10);
					free(token);
					telegram_error = 1;
				}
				else	{
					telegram_setError("Unexpect token type",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
					entrar = 0;
				}
			break;
			case 5: //mov token string value to error ptr
				if(tokens[i].type == response_table[state][i]){
					token = jsmn_getTokenStr(tokens[i]);
					telegram_setError(token,telegram_error_code);
					entrar = 0;
				}
				else	{
					telegram_setError("Unexpect token type",TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE);
					entrar = 0;
				}
			break;
		}
		i++;
	}
	memset(tokens,0,n*sizeof(jsmntok_t));
	free(tokens);
	return telegram_error;
}

struct User * telegram_parse_user()	{
	struct User *user;
	int r,i,n = 1;
	char *token,*token1;
	jsmn_init(&parser);
	user = calloc(1,sizeof(struct User));
	tokens = calloc(n,sizeof(jsmntok_t));
	if(user == NULL)	{
		telegram_setError("calloc: error",0xDEAD);
	}
	if(tokens == NULL)	{
		telegram_setError("calloc: error",0xDEAD);
	}
	r = jsmn_parse(&parser, telegram_buffer, telegram_buffer_offset, tokens, n);
	while (r == JSMN_ERROR_NOMEM)	{
		//telegram_setError("offset",telegram_buffer_offset);
		n*=2;
		tokens = realloc(tokens, sizeof(jsmntok_t) * n);
		r = jsmn_parse(&parser, telegram_buffer, telegram_buffer_offset, tokens, n);
	}
	if (r == JSMN_ERROR_INVAL)	{
		telegram_setError("jsmn_parse: invalid JSON string",JSMN_ERROR_INVAL);
	}
	if (r == JSMN_ERROR_PART)	{
		telegram_setError("jsmn_parse: truncated JSON string",JSMN_ERROR_INVAL);
	}
	if(tokens == NULL)	{
		telegram_setError("calloc: error",0xDEAD);
	}
	i = 1;
	while(tokens[i].end != 0 && tokens[i].size != 0)	{
		//telegram_setError("While in",0xDEAD);
		//telegram_dump_token(tokens[i]);
		token = jsmn_getTokenStr(tokens[i]);
		i++;
		//printf("Token: %s\n",token);
		switch(indexOf(token,user_str))	{
			case 0:	//Is ID
				//telegram_setError("Debug",0);
				token1 = jsmn_getTokenStr(tokens[i]);
				user->id = strtol(token1,NULL,10);
				free(token1);
			break;
			case 1:	//fistname
				//telegram_setError("Debug",1);
				token1 = jsmn_getTokenStr(tokens[i]);
				user->first_name = token1;
			break;
			case 2:	//lastname
				//telegram_setError("Debug",2);
				token1 = jsmn_getTokenStr(tokens[i]);
				user->last_name = token1;
			break;
			case 3:	//username
				//telegram_setError("Debug",3);
				token1 = jsmn_getTokenStr(tokens[i]);
				user->username = token1;
			break;
			default:
				telegram_setError("UNEXPECTED TOKEN VALUE",TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE);
			break;
		}
		i++;
		free(token);
	}
	//telegram_dump_user(user);
	return user;
}

int indexOf(char *str,char **ptr_strings)	{
	int i = 0,entrar = 1;
	while(entrar && ptr_strings[i] != NULL )	{
		//printf("Buscando %s en %s\n",str,ptr_strings[i]);
		if(strcmp(ptr_strings[i],str) == 0)	{
			//printf("encontrado en index: %i\n",i);
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

int telegram_setError(char *error_str,int error_code)	{
	printf("%i : %s\n",error_code,error_str);
	telegram_error_buffer = error_str;
	telegram_error_code = error_code;
	telegram_error = 1;
	return 0;
}

char * jsmn_getTokenStr(jsmntok_t token)	{
	char *str;
	int size = token.end - token.start;
	str = calloc(size+1,sizeof(char));
	memcpy(str,telegram_buffer+token.start,size);
	return str;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata )	{
	//printf("offset actual %i\n",telegram_buffer_offset);
	size_t new_len = telegram_buffer_offset + size*nmemb;
	if(new_len >= telegram_buffer_size)	{
		do	{
			telegram_buffer_size*=2;
		}while(new_len >= telegram_buffer_size);
		telegram_buffer = realloc(telegram_buffer, telegram_buffer_size +1);
		if (telegram_buffer == NULL) {
			fprintf(stderr, "realloc() failed\n");
			exit(EXIT_FAILURE);
		}
	}
	memcpy(telegram_buffer + telegram_buffer_offset, ptr, size*nmemb);
	telegram_buffer[new_len] = '\0';
	telegram_buffer_offset+= size*nmemb;
	//printf("offset actual %i\n",telegram_buffer_offset);
	//printf("buffer actual %i\n",telegram_buffer_size);
	return size*nmemb;
}

void telegram_dump_user(struct User *user)	{
	printf("id: %i\n",user->id);
	printf("first_name: %s\n",user->first_name);
	printf("last_name: %s\n",user->last_name);
	printf("username: %s\n",user->username);
	
}

void telegram_dump_token(jsmntok_t token)	{
	printf("type: %i\n",token.type);
	printf("start: %i\n",token.start);
	printf("end: %i\n",token.end);
	printf("size: %i\n",token.size);
#ifdef JSMN_PARENT_LINKS	
	printf("size: %i\n",token.parent);
#endif

}
