/*
 * Luis Alberto 
 * Twitter @albertobsd
 */


#ifndef __TBOT_H_
#define __TBOT_H_

#include"jsmn.h"

enum chat_type	{
	chat_private = 0,
	chat_group = 1,
	chat_supergroup = 2,
	chat_channel = 3
};

typedef struct	{
	char *phone_number;
	char *first_name;
	char *last_name;
	int user_id;
}Contact;

typedef struct 	{
	char *file_id;
	int duration;
	char *mime_type;
	int file_size;
}Voice;

typedef struct {
	char *file_id;
	int width;
	int height;
	int file_size;
} PhotoSize;

typedef struct {
	int length;
	PhotoSize **item;
}Photos;

typedef struct 	{
	char *file_id;
 	PhotoSize *thumb;
	char *file_name;
	char *mime_type;
	int file_size;
}Document;

typedef struct	{
	char *file_id;
	int width;
	int height;
	PhotoSize *thumb;
	int file_size;
}Sticker;

typedef struct	{
	int id;
	enum chat_type type;
	char *title;
	char *username;
	char *first_name;
	char *last_name;
}Chat;

typedef struct	{
	int id;
	char *first_name;
	char *last_name;
	char *username;
}User;

typedef struct	{
	float longitude;
	float latitude;
}Location;

typedef struct	{
	char *file_id;
	int width;
	int height;
	int duration;
	PhotoSize *thumb;
	char *mime_type;
	int file_size;
}Video;

typedef struct Message {
	int message_id;
	User *from;
	int date;
	Chat *chat;
	User *forward_from;
	int forward_date;
	struct Message *reply_to_message;
	char *text;
	void *audio;
	Document *document;
	Photos *photo;
	Sticker *sticker;
	Video *video;
	Voice *voice;
	char *caption;
	Contact *contact;
	Location *location;
	User *new_chat_participant;
	User *left_chat_participant;
	char *new_chat_title;
	Photos *new_chat_photo;
	int delete_chat_photo;
	int group_chat_created;
	int supergroup_chat_created;
	int channel_chat_created;
	int migrate_to_chat_id;
	int migrate_from_chat_id;
}Message;

typedef struct	{
	char *ok;
	char *result;
	int error_code;
	char *description;
}Response;

typedef struct {
	int update_id;
	int type;
	union  {
		Message *message; 
	}item;
}Update;

typedef struct	{
	int length;
	Update **list;
}Updates;

typedef struct	{
	char *file_id;
	int file_size;
	char *file_path;
}File;

typedef struct {
	char *file_id;
	int duration;
	char *performer;
	char *title;
	char *mime_type;
	int file_size;
}Audio;

typedef enum {
	TELEGRAM_ERROR_CURL_= 1,
	TELEGRAM_ERROR_API_RESPONSE = -10,
	TELEGRAM_ERROR_DEVELOPER = -20,
	TELEGRAM_ERROR_UNEXPECTED_TOKEN_TYPE  = -30,
	TELEGRAM_ERROR_UNEXPECTED_TOKEN_VALUE = -40,
	TELEGRAM_ERROR_API_404 = 404
}telegram_error_t;




/*
 * free telegram variables funtions
 */

Response* telegram_free_response(Response *res);
User* telegram_free_user(User *user);
Update* telegram_free_update(Update *update);
Updates* telegram_free_updates(Updates *updates);
Message* telegram_free_message(Message *message);
Document* telegram_free_document(Document *document);
Video* telegram_free_video(Video *video);
Voice* telegram_free_voice(Voice *voice);
Audio* telegram_free_audio(Audio *audio);
Sticker* telegram_free_sticker(Sticker *sticker);
Chat* telegram_free_chat(Chat *chat);
Photos* telegram_free_photos(Photos *photos);
Contact* telegram_free_contact(Contact *contact);
Location* telegram_free_location(Location *location);
PhotoSize* telegram_free_photosize(PhotoSize *photosize);



int telegram_init(char *);
int indexOf(char *str,char **ptr_strings);
int telegram_jsmn_init(jsmn_parser *parser,jsmntok_t **t,char *buffer,int *n);
int telegram_set_error(char *error_str,int error_code);
char * telegram_jsmn_get_token(jsmntok_t token,char *full);
void telegram_dump_token(jsmntok_t token,char *str);
char *telegram_makeurl(char *telegram_method);
char *telegram_get_error();
int telegram_is_error();
char* telegram_build_post(char **variables,char **values);


int telegram_reset_buffer();
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata );
off_t fsize(const char *filename);

/*
 * Telegram API methods
 */

User * telegram_getMe();
Updates * telegram_getUpdates();
Message* telegram_sendMessage(char *postdata);
File* telegram_getFile(char *file_id);
Message* telegram_sendDocument(char *filename,char **variables, char **valores);


/*
 * Telegram parse
 */

User * telegram_parse_user(char *str,int *count);
Response* telegram_parse_response(char *str,int *count);
Updates* telegram_parse_updates(char *str,int *count);
Update* telegram_parse_update(char *str,int *count);
Message* telegram_parse_message(char *str,int *count);
Chat* telegram_parse_chat(char *str,int *count);
PhotoSize* telegram_parse_photosize(char *str,int *count);
Photos* telegram_parse_photos(char *str,int *count);
Sticker* telegram_parse_sticker(char *str,int *count);
Voice* telegram_parse_voice(char *str,int *count);
Location* telegram_parse_location(char *str,int *count);
Contact* telegram_parse_contact(char *str,int *count);
Document* telegram_parse_document(char *str,int *count);
Video* telegram_parse_video(char *str,int *count);
File* telegram_parse_file(char *str,int *count);
char* telegram_downloadFile(File *file,char *name);
char* telegram_process_slash(char *str);
CURL* telegram_curl_init();


#endif /* __TBOT_H_ */
