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

