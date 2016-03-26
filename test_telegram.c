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
	int i = 0;
	telegram_init("123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11");
	updates = telegram_getUpdates();
	if(!telegram_is_error()){
		printf("updates: %i\n",updates->length);
		while(i < updates->length)	{
			printf("Mensaje from %s (%i) @ chat: %i\n%s\n",updates->list[i]->item.message->from->username,updates->list[i]->item.message->from->id,updates->list[i]->item.message->chat->id,updates->list[i]->item.message->text);
			i++;
		}
	}
	else	{
		printf("%s",telegram_get_error());
	}
	return 0;
}
