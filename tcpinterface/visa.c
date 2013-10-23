#include <stdio.h>
#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <time.h>

int ipcconnect(int);

char stationid[7]="456789";

fldformat* loadNetFormat(void)
{
	return load_format((char*)"../parser/formats/fields_visa");
}

field* parseNetMsg(char *buf, unsigned int length, fldformat *frm)
{
	field *message;

	if(!buf)
	{
		printf("Error: no buf\n");
		return NULL;
	}

	if(!frm)
	{
		printf("Error: no frm\n");
		return NULL;
	}
	
	printf("\nMessage received, length %d\n", length);

	message=parse_message(buf, length, frm);   //TODO: For Visa, parse header (frm->fld[0]) and message(frm->fld[1]) separately to handle reject header properly.

	if(!message)
		printf("Error: Unable to parse\n");

	return message;
}

unsigned int buildNetMsg(char *buf, unsigned int maxlength, field *message, fldformat *frm)
{
	if(!buf)
	{
		printf("Error: no buf\n");
		return 0;
	}

	if(!frm)
	{
		printf("Error: no frm\n");
		return 0;
	}

	if(!message)
	{
		printf("Error: no message\n");
		return 0;
	}

	strcpy(add_field(add_field(message, 0), 4)->data, "0000");

	sprintf(message->fld[0]->fld[4]->data, "%04X", get_length(message) - frm->lengthLength);

	return build_message(buf, maxlength, message);
	
}

int translateNetToSwitch(isomessage *visamsg, field *fullmessage)
{
	field *header;
	field *message;

	struct tm datetime, current;
	time_t now;

	if(!fullmessage)
	{
		printf("Error: No message\n");
		return 1;
	}

	if(!visamsg)
	{
		printf("Error: no visamsg\n");
		return 1;
	}

	if(!fullmessage->fld[0])
	{
		printf("Error: No message header found\n");
		return 1;
	}

	if(!fullmessage->fld[1])
	{
		printf("Error: No message body found\n");
		return 1;
	}

	header=fullmessage->fld[0];
	message=fullmessage->fld[1];

	visamsg->Clear();

	visamsg->set_sourceinterface("visa");
	visamsg->set_sourcestationid(header->fld[6]->data);
	visamsg->set_visaroundtripinf(header->fld[7]->data);
	visamsg->set_visabaseiflags(header->fld[8]->data);
	visamsg->set_visamsgstatusflags(header->fld[9]->data);
	visamsg->set_batchnumber(header->fld[10]->data);
	visamsg->set_visareserved(header->fld[11]->data);
	visamsg->set_visauserinfo(header->fld[12]->data);

	if(!message->fld[0])
	{
		printf("Error: No message type\n");
		return 1;
	}

	visamsg->set_isoversion(isomessage::ISO1987);
	visamsg->set_messageclass((isomessage_MsgClass)(message->fld[0]->data[1]-'0'));
	visamsg->set_messagefunction((isomessage_MsgFunction)(message->fld[0]->data[2]-'0'));
	visamsg->set_messageorigin((isomessage_MsgOrigin)(message->fld[0]->data[3]-'0'));

	if(message->fld[2])
		visamsg->set_pan(message->fld[2]->data);

	if(message->fld[3])
	{
		switch(atoi(message->fld[3]->fld[1]->data))
		{
			case 00:
				visamsg->set_transactiontype(isomessage::PURCHASE);
				break;

			case 01:
				visamsg->set_transactiontype(isomessage::CASH);
				break;

			case 03:
				visamsg->set_transactiontype(isomessage::CHECK);
				break;

			case 10:
				visamsg->set_transactiontype(isomessage::ACCNTFUNDING);
				break;

			default:
				printf("Warning: Unknown processing code: '%s'\n", message->fld[3]->fld[1]->data);
		}

		switch(atoi(message->fld[3]->fld[2]->data))
		{
			case 00:
				visamsg->set_accounttypefrom(isomessage::DEFAULT);
				break;

			case 10:
				visamsg->set_accounttypefrom(isomessage::SAVINGS);
				break;

			case 20:
				visamsg->set_accounttypefrom(isomessage::CHECKING);
				break;

			case 30:
				visamsg->set_accounttypefrom(isomessage::CREDIT);
				break;

			case 40:
				visamsg->set_accounttypefrom(isomessage::UNIVERSAL);
				break;

			default:
				printf("Warning: Unknown From account type: '%s'\n", message->fld[3]->fld[2]->data);
		}

		switch(atoi(message->fld[3]->fld[3]->data))
		{
			case 00:
				visamsg->set_accounttypeto(isomessage::DEFAULT);
				break;

			case 10:
				visamsg->set_accounttypeto(isomessage::SAVINGS);
				break;

			case 20:
				visamsg->set_accounttypeto(isomessage::CHECKING);
				break;

			case 30:
				visamsg->set_accounttypeto(isomessage::CREDIT);
				break;

			case 40:
				visamsg->set_accounttypeto(isomessage::UNIVERSAL);
				break;

			default:
				printf("Warning: Unknown To account type: '%s'\n", message->fld[3]->fld[3]->data);
		}
	}

	if(message->fld[4])
		visamsg->set_amounttransaction(atol(message->fld[4]->data));

	if(message->fld[5])
		visamsg->set_amountsettlement(atol(message->fld[5]->data));

	if(message->fld[6])
		visamsg->set_amountbilling(atol(message->fld[6]->data));

	time(&now);
	gmtime_r(&now, &current);

	if(message->fld[7])
	{
		memset(&datetime, 0, sizeof(datetime));
		sscanf(message->fld[7]->data, "%2d%2d%2d%2d%2d", &datetime.tm_mon, &datetime.tm_mday, &datetime.tm_hour, &datetime.tm_min, &datetime.tm_sec);
		datetime.tm_mon--;

		datetime.tm_year=current.tm_year;

		if(datetime.tm_mon>8 && current.tm_mon<3)
			datetime.tm_year--;
		else if(datetime.tm_mon<3 && current.tm_mon>8)
			datetime.tm_year++;

		visamsg->set_transactiondatetime(timegm(&datetime));
	}

	

	return 0;
}

field* translateSwitchToNet(isomessage *visamsg, fldformat *frm)
{
	field *header;
	field *message;
	fldformat *hfrm;
	fldformat *mfrm;
	field *fullmessage=(field*)calloc(1, sizeof(field));

	fullmessage->frm=frm;

	time_t datetime;

	hfrm=frm->fld[0];
	mfrm=frm->fld[1];
	header=add_field(fullmessage, 0);
	message=add_field(fullmessage, 1);

	strcpy(add_field(header, 2)->data, "01");

	if(mfrm->fld[62] && mfrm->fld[62]->fld[0] && mfrm->fld[62]->fld[0]->dataFormat==FRM_BITMAP)
		strcpy(add_field(header, 3)->data, "02");
	else
		strcpy(add_field(header, 3)->data, "01");

	if(isRequest(visamsg))
	{
		strcpy(add_field(header, 5)->data, "000000");
		strcpy(add_field(header, 7)->data, "00000000");
		strcpy(add_field(header, 8)->data, "0000000000000000");
		strcpy(add_field(header, 9)->data, "000000000000000000000000");
		strcpy(add_field(header, 10)->data, "00");
		strcpy(add_field(header, 11)->data, "000000");
		strcpy(add_field(header, 12)->data, "00");
	}
	else
	{
		strncpy(add_field(header, 5)->data, visamsg->sourcestationid().c_str(), 6);
		strncpy(add_field(header, 7)->data, visamsg->visaroundtripinf().c_str(), 8);
		strncpy(add_field(header, 8)->data, visamsg->visabaseiflags().c_str(), 16);
		strncpy(add_field(header, 9)->data, visamsg->visamsgstatusflags().c_str(), 24);
		strncpy(add_field(header, 10)->data, visamsg->visamsgstatusflags().c_str(), 2);
		strncpy(add_field(header, 11)->data, visamsg->visareserved().c_str(), 6);
		strncpy(add_field(header, 12)->data, visamsg->visauserinfo().c_str(), 2);
	}
	
	strcpy(add_field(header, 6)->data, stationid);

	add_field(message, 0)->data[0]='0';
	message->fld[0]->data[1]='0'+visamsg->messageclass();
	message->fld[0]->data[2]='0'+visamsg->messagefunction();
	message->fld[0]->data[3]='0'+visamsg->messageorigin();
	
	if(visamsg->has_pan())
		strcpy(add_field(message, 2)->data, visamsg->pan().c_str());

	if(visamsg->has_responsecode())
		sprintf(add_field(message, 39)->data, "%02d", visamsg->responsecode());

	if(visamsg->has_transactiontype())
	{
		switch(visamsg->transactiontype())
		{
			case isomessage::PURCHASE:
				strcpy(add_field(add_field(message, 3), 1)->data, "00");
				break;

			case isomessage::CASH:
				strcpy(add_field(add_field(message, 3), 1)->data, "01");
				break;

			case isomessage::CHECK:
				strcpy(add_field(add_field(message, 3), 1)->data, "03");
				break;

			case isomessage::ACCNTFUNDING:
				strcpy(add_field(add_field(message, 3), 1)->data, "10");
				break;
		}

		switch(visamsg->accounttypefrom())
		{
			case isomessage::DEFAULT:
				strcpy(add_field(message->fld[3], 2)->data, "00");
				break;

			case isomessage::SAVINGS:
				strcpy(add_field(message->fld[3], 2)->data, "10");
				break;

			case isomessage::CHECKING:
				strcpy(add_field(message->fld[3], 2)->data, "20");
				break;

			case isomessage::CREDIT:
				strcpy(add_field(message->fld[3], 2)->data, "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(add_field(message->fld[3], 2)->data, "40");
				break;

			default:
				printf("Warning: Unknown account type From: '%d'. Using default.\n", visamsg->accounttypefrom());
				strcpy(add_field(message->fld[3], 2)->data, "00");
				break;
		}

		switch(visamsg->accounttypeto())
		{
			case isomessage::DEFAULT:
				strcpy(add_field(message->fld[3], 3)->data, "00");
				break;

			case isomessage::SAVINGS:
				strcpy(add_field(message->fld[3], 3)->data, "10");
				break;

			case isomessage::CHECKING:
				strcpy(add_field(message->fld[3], 3)->data, "20");
				break;

			case isomessage::CREDIT:
				strcpy(add_field(message->fld[3], 3)->data, "30");
				break;

			case isomessage::UNIVERSAL:
				strcpy(add_field(message->fld[3], 3)->data, "40");
				break;

			default:
				printf("Warning: Unknown account type To: '%d'. Using default.\n", visamsg->accounttypeto());
				strcpy(add_field(message->fld[3], 3)->data, "00");
				break;
		}
	}

	if(visamsg->has_amounttransaction())
		snprintf(add_field(message, 4)->data, 13, "%012ld", visamsg->amounttransaction());

	if(visamsg->has_amountsettlement())
		snprintf(add_field(message, 5)->data, 13, "%012ld", visamsg->amountsettlement());

	if(visamsg->has_amountbilling())
		snprintf(add_field(message, 6)->data, 13, "%012ld", visamsg->amountbilling());

	if(visamsg->has_transactiondatetime())
	{
		datetime=visamsg->transactiondatetime();
		printf("UTC Transaction date time: %s\n", asctime(gmtime(&datetime)));
		strftime(add_field(message, 7)->data, 11, "%m%d%H%M%S", gmtime(&datetime));
	}




	return fullmessage;
}

int isNetMgmt(field *message)
{
	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data)
		return 0;

	if(message->fld[1]->fld[0]->data[1]=='8')
		return 1;

	return 0;
}

int isNetRequest(field *message)
{
	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data)
		return 0;

	if(message->fld[1]->fld[0]->data[2]=='0' || message->fld[1]->fld[0]->data[2]=='2')
		return 1;

	return 0;
}

int processNetMgmt(field *message, fldformat *frm)
{
	field *header;
	field *mbody;

	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data || !message->fld[0] || !message->fld[0]->fld || !message->fld[0]->fld[5] || !message->fld[0]->fld[5]->data || !message->fld[0]->fld[6] || !message->fld[0]->fld[6]->data || !frm || !frm->fld || !frm->fld[1])
		return 0;

	header=message->fld[0];
	mbody=message->fld[1];

	strcpy(header->fld[5]->data, header->fld[6]->data);
	strcpy(header->fld[6]->data, stationid);

	if(mbody->fld[0]->data[2]=='0')
		mbody->fld[0]->data[2]='1';

	if(mbody->fld[0]->data[2]=='2')
		mbody->fld[0]->data[2]='3';

	strcpy(add_field(mbody, 39)->data, "00");

	return 1;
}

int declineNetMsg(field *message, fldformat *frm)
{
	char stationid[7];
	field *header;
	field *mbody;

	if(!message || !message->fld || !message->fld[1] || !message->fld[1]->fld || !message->fld[1]->fld[0] || !message->fld[1]->fld[0]->data || !message->fld[0] || !message->fld[0]->fld || !message->fld[0]->fld[5] || !message->fld[0]->fld[5]->data || !message->fld[0]->fld[6] || !message->fld[0]->fld[6]->data || !frm || !frm->fld || !frm->fld[1])
		return 0;

	header=message->fld[0];
	mbody=message->fld[1];

	strncpy(stationid, header->fld[5]->data, 6);
	strncpy(header->fld[5]->data, header->fld[6]->data, 6);
	strncpy(header->fld[6]->data, stationid, 6);
	header->fld[9]->data[16]='1';

	if(mbody->fld[0]->data[2]=='0')
		mbody->fld[0]->data[2]='1';

	if(mbody->fld[0]->data[2]=='2')
		mbody->fld[0]->data[2]='3';

	strcpy(add_field(mbody, 39)->data, "96");
	strcpy(add_field(add_field(mbody, 44), 1)->data, "5");

	remove_field(mbody, 18);
	remove_field(mbody, 22);
	remove_field(mbody, 35);
	remove_field(mbody, 43);
	remove_field(mbody, 60);
	remove_field(mbody, 104);

	return 1;
}
