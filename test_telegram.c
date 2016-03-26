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
	int i = 0;
	telegram_init("0123456789:qwertyuiopdfghjkl");
	user = telegram_getMe();
	if(!telegram_is_error())	{
		printf("User: id: %i\nusername: %s\n",user->id,user->username);
	}
	else	{
		printf("%s\n",telegram_get_error());
	}
	updates = telegram_getUpdates();
	if(!telegram_is_error()){
		printf("updates: %i\n",updates->length);
		while(i < updates->length)	{
			printf("Mensaje from %s (%i) @ chat: %i\n%s\n",updates->list[i]->item.message->from->username,updates->list[i]->item.message->from->id,updates->list[i]->item.message->chat->id,updates->list[i]->item.message->text);
			i++;
		}
	}
	else	{
		printf("%s\n",telegram_get_error());
	}
	return 0;
}
