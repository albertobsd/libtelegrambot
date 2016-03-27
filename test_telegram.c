#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>
#include<sys/stat.h>
#include<errno.h>
#include"telegram.h"
#include"jsmn.h"

int main()	{
	Updates *updates;
	User *user;
	File *file;
	int i = 0;
	char *filename;
	telegram_init("1234567:fghjkl45678xcvbkcvbnzsxdcfvgbh");
	user = telegram_getMe();
	if(!telegram_is_error())	{
		printf("User: id: %i\nusername: %s\n",user->id,user->username);
		telegram_free_user(user);
	}
	else	{
		printf("%s\n",telegram_get_error());
	}
	updates = telegram_getUpdates();
	if(!telegram_is_error()){
		printf("updates: %i\n",updates->length);
		while(i < updates->length)	{
			if(updates->list[i]->item.message->document)	{
				printf("Document exits file_id : %s\n",updates->list[i]->item.message->document->file_id);
				file = telegram_getFile(updates->list[i]->item.message->document->file_id);
				if(!telegram_is_error())	{
					filename = telegram_downloadFile(file,updates->list[i]->item.message->document->file_name);
					printf("file name : %s\n",filename);
				}
				else	{
					printf("%s\n",telegram_get_error());
				}
			}
			i++;
		}
		telegram_free_updates(updates);
	}
	else	{
		printf("%s\n",telegram_get_error());
	}
	return 0;
}

