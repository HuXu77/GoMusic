/*
 * AllAccessHelper.cpp
 *
 *  Created on: Jul 2, 2013
 *      Author: nmmrc18
 */

#include "AllAccessHelper.h"
#include <time.h>
#include "huctx.h"
#include "hugse56.h"
#include "sbreturn.h"
#include <QDebug>

const QString AllAccessHelper::ALPHANUM_LOWERCASE = QString("abcdefghijklmnopqrstuvwxyz0123456789");
const QString AllAccessHelper::AA_KEY = QString("27f7313e-f75d-445a-ac99-56386a5fe879");

const QString AllAccessHelper::S1 = QString("VzeC4H4h+T2f0VI180nVX8x+Mb5HiTtGnKgH52Otj8ZCGDz9jRWyHb6QXK0JskSiOgzQfwTY5xgLLSdUSreaLVMsVVWfxfa8Rw==");
const QString AllAccessHelper::S2 = QString("ZAPnhUkYwQ6y5DdQxWThbvhJHN8msQ1rqJw0ggKdufQjelrKuiGGJI30aswkgCWTDyHkTGK9ynlqTkJ5L4CiGGUabGeo8M6JTQ==");

AllAccessHelper::AllAccessHelper() :
QObject() {
	hu_GlobalCtxCreateDefault(&globalContext);
	hu_RegisterSbg56HMACSHA1(globalContext);
	hu_InitSbg56(globalContext);
}

QString AllAccessHelper::getSalt() {
	unsigned int t = time(NULL);
	srand(t);
	//	https://play.google.com/music/play?u=0&slt=y2tn2l4cd3pe&mjck=Tfgh23n6zyqnrf2fnl77b4t4u7a&sig=z-QjC8jYzQdJWJMw1BSdWkkkfhU.&pt=e																									  Ao1UcKPNGEwUnpEsErc3cEszWO8.
	// https://play.google.com/music/play?u=0&slt=f671rl2w4ba&songid=86aa60b4-a917-3e54-a018-425b85aeebd8&sig=oR8mdl_q8lZswvwIRVts-ffb1oY.&pt=e
	QString salt;
	for (int i = 0; i < 12; i++) {
		int index = rand() % ALPHANUM_LOWERCASE.length();
		QChar cha = ALPHANUM_LOWERCASE.at(index);
		salt.append(cha);
	}
	//salt = "1390856271954";
	return salt;
}

void AllAccessHelper::getSig(QString songIdSalt, unsigned char* message) {
	//QByteArray keybytes = AA_KEY.toUtf8();
	QByteArray songIdSaltBytes = songIdSalt.toUtf8();

	QByteArray s1 = QByteArray::fromBase64(S1.toUtf8());
	QByteArray s2 = QByteArray::fromBase64(S2.toUtf8());
	char* tmp = new char[s1.length()];
	QByteArray keybytes = QByteArray(s1.length(), *tmp);

	for (int i = 0; i < s1.length(); i++) {
		keybytes[i] = s1[i] ^ s2[i];
	}

	unsigned char* data = reinterpret_cast<unsigned char*>(songIdSaltBytes.data());
	unsigned char* key = reinterpret_cast<unsigned char*>(keybytes.data());

	sb_Context context=NULL;

	int success = hu_HMACSHA1Begin((size_t) keybytes.length(), key, NULL, &context, globalContext);
	if (success != SB_SUCCESS) {
		qDebug("GMA: Error in SHABegin");
	}
	//hu_SHA256Begin((size_t) SB_SHA256_DIGEST_LEN, NULL, &sha256Context, sbCtx);

	//qDebug() << "GMA: MAC: BLAH: " << success;

	success = hu_HMACSHA1Hash(context, (size_t) songIdSaltBytes.length(), data, globalContext);
	if (success != SB_SUCCESS) {
		qDebug("GMA: Error in SHAHash");
	}

	//qDebug() << "GMA: MAC: HASH: " << success;

	success = hu_HMACSHA1End(&context, (size_t) SB_HMAC_SHA1_160_TAG_LEN, message, globalContext);

	if (success != SB_SUCCESS) {
		qDebug("GMA: Error in SHAEnd");
	}
	//qDebug() << "GMA: MAC: END: " << success;

	//char * data = songIdSaltBytes.constData();
	//size_t datasize = songIdSaltBytes.length();
}

AllAccessHelper::~AllAccessHelper() {
	hu_GlobalCtxDestroy(&globalContext);
}

